#include "x86.h"
#include "device.h"
#define va_to_pa(va) (va + (current + 1) * 0x100000)
#define pa_to_va(pa) (pa - (current + 1) * 0x100000)
#define ARGVBUFFER 0x1000

extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern int displayRow;
extern int displayCol;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;

void syscallHandle(struct TrapFrame *tf);
void syscallWrite(struct TrapFrame *tf);
void syscallPrint(struct TrapFrame *tf);
void syscallFork(struct TrapFrame *tf);
void syscallExec(struct TrapFrame *tf);
void syscallSleep(struct TrapFrame *tf);
void syscallExit(struct TrapFrame *tf);

void GProtectFaultHandle(struct TrapFrame *tf);

void timerHandle(struct TrapFrame *tf);
void keyboardHandle(struct TrapFrame *tf);

void irqHandle(struct TrapFrame *tf) { // pointer tf = esp
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));

	uint32_t tmpStackTop = pcb[current].stackTop;
	pcb[current].prevStackTop = pcb[current].stackTop;
	pcb[current].stackTop = (uint32_t)tf;

	switch(tf->irq) {
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(tf); // return
			break;
		case 0x20:
			timerHandle(tf);         // return or iret
			break;
		case 0x21:
			keyboardHandle(tf);      // return
			break;
		case 0x80:
			syscallHandle(tf);       // return
			break;
		default:assert(0);
	}

	pcb[current].stackTop = tmpStackTop;
}

void syscallHandle(struct TrapFrame *tf) {
	switch(tf->eax) { // syscall number
		case 0:
			syscallWrite(tf);
			break; // for SYS_WRITE
		case 1:
			syscallFork(tf);
			break; // for SYS_FORK
		case 2:
			syscallExec(tf);
			break; // for SYS_EXEC
		case 3:
			syscallSleep(tf);
			break; // for SYS_SLEEP
		case 4:
			syscallExit(tf);
			break; // for SYS_EXIT
		default:break;
	}
}

void timerHandle(struct TrapFrame *tf) {
	// TODO in lab3
	// sleep time elapse
	for (int i = 0; i < MAX_PCB_NUM; i++) {
		if (pcb[i].state == STATE_BLOCKED) {
			pcb[i].sleepTime--;
			if (pcb[i].sleepTime <= 0)
				pcb[i].state = STATE_RUNNABLE;
		}
	}
	// current process running timeslice
	if (pcb[current].state == STATE_RUNNING)
		pcb[current].timeCount++;
	// situation where a process-switch is needed
	if (pcb[current].timeCount >= MAX_TIME_COUNT || pcb[current].state != STATE_RUNNING) {
		// current process timeslice runs out
		if (pcb[current].state == STATE_RUNNING) {
			pcb[current].timeCount = 0;
			pcb[current].state = STATE_RUNNABLE;
		}
		int nextPid = -1;
		// find a RUNNABLE process (will go back to itself if others are all not RUNNABLE)
		for (int i = 1; i <= MAX_PCB_NUM; i++) {
			int pid = (current + i) % MAX_PCB_NUM;
			if (pid && pcb[pid].state == STATE_RUNNABLE) { // IDLE process is the last choice
				nextPid = pid;
				break;
			}
		}
		// all except IDLE process are not RUNNABLE
		if (nextPid == -1) nextPid = 0;
		// no switch happens (others all not RUNNABLE)
		if (nextPid == current) return;
		// switch process
		current = nextPid;
		pcb[current].state = STATE_RUNNING;
		uint32_t tmpStackTop = pcb[current].stackTop;
		pcb[current].stackTop = pcb[current].prevStackTop;
		tss.esp0 = (uint32_t)&(pcb[current].stackTop);
		asm volatile("movl %0, %%esp"::"m"(tmpStackTop)); // switch kernel stack
		asm volatile("popl %gs");
		asm volatile("popl %fs");
		asm volatile("popl %es");
		asm volatile("popl %ds");
		asm volatile("popal");
		asm volatile("addl $8, %esp");
		asm volatile("iret");
	}
}

void keyboardHandle(struct TrapFrame *tf) {
	uint32_t keyCode = getKeyCode();
	if (keyCode == 0)
		return;
	putChar(getChar(keyCode));
	keyBuffer[bufferTail] = keyCode;
	bufferTail = (bufferTail + 1) % MAX_KEYBUFFER_SIZE;
	return;
}

void syscallWrite(struct TrapFrame *tf) {
	switch(tf->ecx) { // file descriptor
		case 0:
			syscallPrint(tf);
			break; // for STD_OUT
		default:break;
	}
}

void syscallPrint(struct TrapFrame *tf) {
	// enableInterrupt();  // for critical section test
	int sel = tf->ds; //TODO segment selector for user data, need further modification
	char *str = (char *)tf->edx;
	int size = tf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		// asm volatile("int $0x20"); // for critical section test
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
		if (character == '\n') {
			displayRow++;
			displayCol = 0;
			if (displayRow == 25){
				displayRow = 24;
				displayCol = 0;
				scrollScreen();
			}
		}
		else {
			data = character | (0x0c << 8);
			pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
			displayCol++;
			if (displayCol == 80){
				displayRow++;
				displayCol = 0;
				if (displayRow == 25){
					displayRow = 24;
					displayCol = 0;
					scrollScreen();
				}
			}
		}
	}
	updateCursor(displayRow, displayCol);
	// disableInterrupt(); // for critical section test
	//TODO take care of return value
}

void copyData(int dstPid, int srcPid) {
	enableInterrupt();
	uint8_t *dstptr = (uint8_t *)(0x100000 * (dstPid+1));
	uint8_t *srcptr = (uint8_t *)(0x100000 * (srcPid+1));
	for (int i = 0; i < 0x100000; i++) {
		*dstptr = *srcptr; srcptr++; dstptr++;
		asm volatile("int $0x20");
	}
	disableInterrupt();
}

void syscallFork(struct TrapFrame *tf) {
	// TODO in lab3
	int child = 0;
	while (child < MAX_PCB_NUM) {
		if (pcb[child].state == STATE_DEAD) break;
		child++;
	}
	if (child == MAX_PCB_NUM) {
		tf->eax = -1; return;
	}
	tf->eax = child;
	pcb[child].state = -1;	// mark this pcb occupied
	copyData(child, current);
	for (int i = 0; i < MAX_STACK_SIZE; i++)
		pcb[child].stack[i] = pcb[current].stack[i];
	pcb[child].regs.eax = 0;
	pcb[child].regs.ecx = pcb[current].regs.ecx;
	pcb[child].regs.edx = pcb[current].regs.edx;
	pcb[child].regs.ebx = pcb[current].regs.ebx;
	pcb[child].regs.xxx = pcb[current].regs.xxx;
	pcb[child].regs.ebp = pcb[current].regs.ebp;
	pcb[child].regs.esi = pcb[current].regs.esi;
	pcb[child].regs.edi = pcb[current].regs.edi;
	pcb[child].regs.eip = pcb[current].regs.eip;
	pcb[child].regs.esp = pcb[current].regs.esp;
	pcb[child].regs.cs = USEL(1 + child * 2);
	pcb[child].regs.ds = USEL(2 + child * 2);
	pcb[child].regs.es = USEL(2 + child * 2);
	pcb[child].regs.fs = USEL(2 + child * 2);
	pcb[child].regs.gs = USEL(2 + child * 2);
	pcb[child].regs.ss = USEL(2 + child * 2);
	pcb[child].regs.eflags = pcb[current].regs.eflags;
	pcb[child].regs.irq = pcb[current].regs.irq;
	pcb[child].regs.error = pcb[current].regs.error;
	pcb[child].stackTop = (uint32_t)&(pcb[child].regs);
	pcb[child].prevStackTop = (uint32_t)&(pcb[child].stackTop);
	pcb[child].state = STATE_RUNNABLE;
	pcb[child].timeCount = 0;
	pcb[child].sleepTime = 0;
	pcb[child].pid = child;
}

void memcpy(void *dst, void *src, uint32_t size) {
	uint8_t *dstptr = dst, *srcptr = src;
	while (size--) { *dstptr = *srcptr; dstptr++; srcptr++; }
}

int strlen(char *str) {
	int len = 0; while (*(str++)) len++; return len;
}

uint32_t copy_argv(char **argv, int *argc) {
	if (!argv) return 0;
	char **p = argv;
	*argc = 0;
	while (*p) {
		*p = va_to_pa(*p);
		(*argc)++;
		p++;
	}
	uint32_t index = ARGVBUFFER;
	uint32_t data = (*argc + 1)* 4 + index;
	uint32_t size = (*argc + 1)* 4;
	p = argv;
	while (*p) {
		char *str = *p;
		memcpy((void *)data, str, strlen(str) + 1);
		size += strlen(str) + 1;
		*(uint32_t *)index = data;
		index += 4;
		data += strlen(str) + 1;
		p++;
	}
	*(uint32_t *)index = 0;
	return size;
}
void trans_addr(uint32_t bias) {
	uint32_t *index = (uint32_t *)ARGVBUFFER;
	for (int i = 0; index[i]; i++)
		index[i] = pa_to_va(index[i]+bias-ARGVBUFFER);
}

void syscallExec(struct TrapFrame *tf) {
	// TODO in lab3
	// hint: ret = loadElf(tmp, (current + 1) * 0x100000, &entry);
	uint32_t entry = 0;
	// 0x10 - 0x1000 for filepath
	char *tmp = (char *)10;
	char *filepath = (char *)va_to_pa(tf->ecx);
	memcpy(tmp, filepath, strlen(filepath) + 1);
	
	// 0x1000 - 0x10000 for argvbuff
	void *argvbuff = (void *)ARGVBUFFER;
	char **argv = (char **)(tf->edx?va_to_pa(tf->edx): 0);
	int argc = 0;
	uint32_t size = copy_argv(argv, &argc);	
	
	int ret = loadElf(tmp, va_to_pa(0), &entry);	// ret is paddr of the last loaded segment
	if (ret == -1)
		tf->eax = -1;
	else {
		if (argv) {
			trans_addr(ret);
			memcpy((void *)ret, (void *)argvbuff, size);
			*(uint32_t *)va_to_pa(tf->esp) = pa_to_va(ret);
			tf->esp -= 4;
			*(uint32_t *)va_to_pa(tf->esp) = argc;
			tf->esp -= 4;
		}
		tf->eax = 0;
		tf->eip = entry;
	}
}

void syscallSleep(struct TrapFrame *tf) {
	// TODO in lab3
	if (tf->ecx >= 0) {
		pcb[current].state = STATE_BLOCKED;
		pcb[current].sleepTime = tf->ecx;
		tf->eax = 0;
		asm volatile("int $0x20");
	}
	else
		tf->eax = -1;
}

void syscallExit(struct TrapFrame *tf) {
	// TODO in lab3
	pcb[current].state = STATE_DEAD;
	asm volatile("int $0x20");
}

void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}
