#include "x86.h"
#include "device.h"

#define SYS_WRITE 0
#define SYS_FORK 1
#define SYS_EXEC 2
#define SYS_SLEEP 3
#define SYS_EXIT 4
#define SYS_READ 5
#define SYS_SEM 6
#define SYS_GETPID 7
#define SYS_GETPPID 8
#define SYS_GETTIME 9
#define SYS_SCHEDYIELD 10
#define SYS_SIG 11
#define SYS_KILL 12

#define SIG_INT 2
#define SIG_TERM 15
#define SIG_CHLD 17

#define STD_OUT 0
#define STD_IN 1
#define SH_MEM 3

#define SEM_INIT 0
#define SEM_WAIT 1
#define SEM_POST 2
#define SEM_DESTROY 3

extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern Semaphore sem[MAX_SEM_NUM];
extern Device dev[MAX_DEV_NUM];

extern int displayRow;
extern int displayCol;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;
extern uint8_t ctrl;

extern uint32_t ticks;

uint8_t shMem[MAX_SHMEM_SIZE];

void syscallHandle(struct TrapFrame *tf);
void syscallWrite(struct TrapFrame *tf);
void syscallRead(struct TrapFrame *tf);
void syscallFork(struct TrapFrame *tf);
void syscallExec(struct TrapFrame *tf);
void syscallSleep(struct TrapFrame *tf);
void syscallExit(struct TrapFrame *tf);
void syscallSem(struct TrapFrame *tf);
void syscallGetPid(struct TrapFrame *tf);
void syscallGetPPid(struct TrapFrame *tf);
void syscallGetTime(struct TrapFrame *tf);
void syscallSchedYield(struct TrapFrame *tf);

void syscallSignal(struct TrapFrame *tf);
void syscallKill(struct TrapFrame *tf);

void syscallWriteStdOut(struct TrapFrame *tf);
void syscallReadStdIn(struct TrapFrame *tf);
void syscallWriteShMem(struct TrapFrame *tf);
void syscallReadShMem(struct TrapFrame *tf);

void GProtectFaultHandle(struct TrapFrame *tf);

void timerHandle(struct TrapFrame *tf);
void keyboardHandle(struct TrapFrame *tf);

void syscallSemInit(struct TrapFrame *tf);
void syscallSemWait(struct TrapFrame *tf);
void syscallSemPost(struct TrapFrame *tf);
void syscallSemDestroy(struct TrapFrame *tf);

int ptr2pid(struct ListHead *ptr) {
	int id = (ProcessTable*)((uint32_t)ptr - (uint32_t)&(((ProcessTable*)0)->blocked)) - pcb;
	if (id >= 0 && id < MAX_PCB_NUM) return id;
	return -1;
}

void blockon(struct ListHead *blocked, struct ListHead *sem_or_dev) {	
	int pid = ptr2pid(blocked);
	if (pid != -1) pcb[pid].state = STATE_BLOCKED;
	blocked->prev = sem_or_dev->prev;
	blocked->next = sem_or_dev;
	sem_or_dev->prev->next = blocked;
	sem_or_dev->prev = blocked;
}

void wakeup(struct ListHead *Wake) {
	int pid = ptr2pid(Wake);
	if (pid != -1) pcb[pid].state = STATE_RUNNABLE;
	struct ListHead *Prev = Wake->prev;
	struct ListHead *Next = Wake->next;
	for (int i = 0; i < MAX_SEM_NUM; i++) {
		if (Prev == &(sem[i].pcb)) {
			sem[i].value++;
			break;
		}
	}
	Wake->next = Wake;
	Wake->prev = Wake;
	Prev->next = Next;
	Next->prev = Prev;
}

int isblocked(struct ListHead *p) {
	return (p->prev != p) || (p->next != p);
}

int send_signal(int pid, int sig) {
	if (pid < MAX_PCB_NUM && sig < MAX_SIG_NUM && pcb[pid].state != STATE_DEAD && pcb[pid].signal[sig] == 0) {
		pcb[pid].signal[sig] = 1;
		if (pcb[pid].state == STATE_SLEEP) pcb[pid].state = STATE_RUNNABLE;
		return 0;
	}
	return -1;
}

void do_signal(struct TrapFrame *tf) {
	for (int i = 0; i < MAX_SIG_NUM; i++) {
		if (pcb[current].signal[i]) {
			pcb[current].signal[i] = 0;
			if (pcb[current].handler[i] != -1) {
				pcb[current].regs.esp -= 4;
				int sel = tf->ss;
				asm volatile("movw %0, %%es"::"m"(sel));
				asm volatile("movl %0, %%es:(%1)"::"r"(pcb[current].regs.eip),"r"(pcb[current].regs.esp));	// old-eip is pushed for later return
				pcb[current].regs.eip = (uint32_t)pcb[current].handler[i];
				break;
			}
			else continue;
		}
	}
}

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

	do_signal(tf);

	pcb[current].stackTop = tmpStackTop;
}

void syscallHandle(struct TrapFrame *tf) {
	switch(tf->eax) { // syscall number
		case SYS_WRITE:
			syscallWrite(tf);
			break; // for SYS_WRITE
		case SYS_READ:
			syscallRead(tf);
			break; // for SYS_READ
		case SYS_FORK:
			syscallFork(tf);
			break; // for SYS_FORK
		case SYS_EXEC:
			syscallExec(tf);
			break; // for SYS_EXEC
		case SYS_SLEEP:
			syscallSleep(tf);
			break; // for SYS_SLEEP
		case SYS_SCHEDYIELD:
			syscallSchedYield(tf);
			break; // for SYS_SCHEDYIELD
		case SYS_EXIT:
			syscallExit(tf);
			break; // for SYS_EXIT
		case SYS_SEM:
			syscallSem(tf);
			break; // for SYS_SEM
		case SYS_GETPID:
			syscallGetPid(tf);
			break; // for SYS_GETPID
		case SYS_GETPPID:
			syscallGetPPid(tf);
			break; // for SYS_GETPID
		case SYS_GETTIME:
			syscallGetTime(tf);
			break; // for SYS_GETTIME
		case SYS_SIG:
			syscallSignal(tf);
			break;
		case SYS_KILL:
			syscallKill(tf);
			break;
		default:break;
	}
}

void timerHandle(struct TrapFrame *tf) {
	ticks++;
	uint32_t tmpStackTop;
	int i = (current + 1) % MAX_PCB_NUM;
	while (i != current) {
		if (pcb[i].state == STATE_SLEEP) {
			pcb[i].sleepTime--;
			if (pcb[i].sleepTime == 0) {
				pcb[i].state = STATE_RUNNABLE;
			}
		}
		i = (i + 1) % MAX_PCB_NUM;
	}
	if (pcb[current].state == STATE_RUNNING &&
			pcb[current].timeCount < MAX_TIME_COUNT &&
			!isblocked(&(pcb[current].blocked))) {
		pcb[current].timeCount++;
		return;
	}
	else {
		if (pcb[current].state == STATE_RUNNING) {
			pcb[current].state = STATE_RUNNABLE;
			pcb[current].timeCount = 0;
		}
		i = (current + 1) % MAX_PCB_NUM;
		while (i != current) {
			if (i != 0 && pcb[i].state == STATE_RUNNABLE && !isblocked(&(pcb[i].blocked))) {
				break;
			}
			i = (i + 1) % MAX_PCB_NUM;
		}
		if (pcb[i].state != STATE_RUNNABLE || isblocked(&(pcb[i].blocked))) {
			i = 0;
		}
		if (current == i) return;
		current = i;
		//putString("Switch to ");
		//putInt(current, '\n');
		pcb[current].state = STATE_RUNNING;
		pcb[current].timeCount = 1;		
		tmpStackTop = pcb[current].stackTop;
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
	return;
}

void keyboardHandle(struct TrapFrame *tf) {
	// TODO in lab4
	uint32_t keyCode = getKeyCode();
	if (keyCode == 0)
		return;
	char keyChar = getChar(keyCode);
	if (keyChar == 0)
		return;
	if (ctrl) putChar('^');
	if (ctrl && keyChar >= 'a' && keyChar <= 'z')
		keyChar += 'A' - 'a';
	putChar(keyChar);
	if (!ctrl) {
		keyBuffer[bufferTail] = keyChar;
		bufferTail = (bufferTail + 1) % MAX_KEYBUFFER_SIZE;
	}
	if (dev[STD_IN].state == 1 && dev[STD_IN].value < 0) {
		dev[STD_IN].value = 1;
		wakeup(dev[STD_IN].pcb.next);
	}
	if (ctrl) {
		switch (keyChar) {
			case 'C':
				for (int i = 1; i < MAX_SIG_NUM; i++) {
					send_signal(i, SIG_INT);
				}
				break;
			case 'L':
				clearScreen();
				displayRow = 0;
				displayCol = 0;
				updateCursor(0, 0);
			default:
				break;
		}
	}
	return;
}

void syscallWrite(struct TrapFrame *tf) {
	switch(tf->ecx) { // file descriptor
		case STD_OUT:
			if (dev[STD_OUT].state == 1) {
				syscallWriteStdOut(tf);
			}
			break; // for STD_OUT
		case SH_MEM:
			if (dev[SH_MEM].state == 1) {
				syscallWriteShMem(tf);
			}
			break; // for SH_MEM
		default:break;
	}
}

void syscallWriteStdOut(struct TrapFrame *tf) {
	int sel = tf->ds; //TODO segment selector for user data, need further modification
	char *str = (char *)tf->edx;
	int size = tf->ebx;
	int font = tf->esi;
	int bg = tf->edi;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	if (font < 0 || font >= 16) font = 0xc;
	if (bg < 0 || bg >= 16) bg = 0x0;
	uint32_t color = (bg << 4) | font;
	
	for (i = 0; i < size; i++) {
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
		else if (character == '\t') {
			do {
				displayCol++;
			} while (displayCol % 4 != 0);
			if (displayCol >= 80) {
				displayRow++;
				displayCol = 0;
				if (displayRow == 25){
					displayRow = 24;
					displayCol = 0;
					scrollScreen();
				}
			}
		}
		else {
			data = character | (color << 8);
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
	//TODO take care of return value
}

void syscallWriteShMem(struct TrapFrame *tf) {
	// TODO in lab4
	int sel = tf->ds;
	char *buffer = (char *)tf->edx;
	int size = tf->ebx;
	int index = tf->esi;
	int i = 0;
	char character;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (; i < size; i++) {
		if (i + index >= MAX_SHMEM_SIZE) {
			break;
		}
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(buffer + i));
		shMem[i + index] = character;
	}
	tf->eax = i;
	return;
}

void syscallRead(struct TrapFrame *tf) {
	switch(tf->ecx) {
		case STD_IN:
			if (dev[STD_IN].state == 1) {
				syscallReadStdIn(tf);
			}
			break;
		case SH_MEM:
			if (dev[SH_MEM].state == 1) {
				syscallReadShMem(tf);
			}
			break;
		default:
			break;
	}
}

void syscallReadStdIn(struct TrapFrame *tf) {
	// TODO in lab4
	if (dev[STD_IN].value > 0) {
		dev[STD_IN].value = 0;
		int sel = tf->ds;
		int i = 0;
		char *str = (char *)tf->edx;
		asm volatile("movw %0, %%es"::"m"(sel));
		for (i = 0; bufferHead != bufferTail; bufferHead = (bufferHead + 1) % MAX_KEYBUFFER_SIZE, i++) {
			char character = keyBuffer[bufferHead];
			asm volatile("movb %0, %%es:(%1)"::"r"(character),"r"(str + i));
		}
		asm volatile("movb %0, %%es:(%1)"::"r"(0),"r"(str + i)); // end with '\0' for scanf
		tf->eax = i;
	}
	else if (dev[STD_IN].value == 0) {
		blockon(&(pcb[current].blocked), &(dev[STD_IN].pcb));
		dev[STD_IN].value--;
		tf->eax = 0;
		asm volatile("int $0x20");
	}
	else if (dev[STD_IN].value < 0) {
		tf->eax = -1;
	}
	return;
}

void syscallReadShMem(struct TrapFrame *tf) {
	// TODO in lab4
	int sel = tf->ds;
	char *buffer = (char *)tf->edx;
	int size = tf->ebx;
	int index = tf->esi;
	int i;
	asm volatile("movw %0, %%es"::"m"(sel));
	char character;
	for (i = 0; i < size; i++) {
		if (i + index >= MAX_SHMEM_SIZE) {
			break;
		}
		character = shMem[i + index];
		asm volatile("movb %0, %%es:(%1)"::"r"(character),"r"(buffer + i));
	}
	tf->eax = i;
	return;
}

void syscallFork(struct TrapFrame *tf) {
	int i, j;
	for (i = 0; i < MAX_PCB_NUM; i++) {
		if (pcb[i].state == STATE_DEAD) {
			break;
		}
	}
	if (i != MAX_PCB_NUM) {
		pcb[i].state = STATE_PREPARING;

		enableInterrupt();
		for (j = 0; j < 0x100000; j++) {
			*(uint8_t *)(j + (i + 1) * 0x100000) = *(uint8_t *)(j + (current + 1) * 0x100000);
		}
		disableInterrupt();
		
		pcb[i].stackTop = (uint32_t)&(pcb[i].stackTop) -
			((uint32_t)&(pcb[current].stackTop) - pcb[current].stackTop);
		pcb[i].prevStackTop = (uint32_t)&(pcb[i].stackTop) -
			((uint32_t)&(pcb[current].stackTop) - pcb[current].prevStackTop);
		pcb[i].state = STATE_RUNNABLE;
		pcb[i].timeCount = pcb[current].timeCount;
		pcb[i].sleepTime = pcb[current].sleepTime;
		pcb[i].pid = i;
		pcb[i].ppid = current;
		for (j = 0; j < MAX_SIG_NUM; j++) {
			pcb[i].handler[j] = pcb[current].handler[j];
		}
		pcb[i].regs.ss = USEL(2 + i * 2);
		pcb[i].regs.esp = pcb[current].regs.esp;
		pcb[i].regs.eflags = pcb[current].regs.eflags;
		pcb[i].regs.cs = USEL(1 + i * 2);
		pcb[i].regs.eip = pcb[current].regs.eip;
		pcb[i].regs.eax = pcb[current].regs.eax;
		pcb[i].regs.ecx = pcb[current].regs.ecx;
		pcb[i].regs.edx = pcb[current].regs.edx;
		pcb[i].regs.ebx = pcb[current].regs.ebx;
		pcb[i].regs.xxx = pcb[current].regs.xxx;
		pcb[i].regs.ebp = pcb[current].regs.ebp;
		pcb[i].regs.esi = pcb[current].regs.esi;
		pcb[i].regs.edi = pcb[current].regs.edi;
		pcb[i].regs.ds = USEL(2 + i * 2);
		pcb[i].regs.es = pcb[current].regs.es;
		pcb[i].regs.fs = pcb[current].regs.fs;
		pcb[i].regs.gs = pcb[current].regs.gs;
		/*XXX set return value */
		pcb[i].regs.eax = 0;
		pcb[current].regs.eax = i;
	}
	else {
		pcb[current].regs.eax = -1;
	}
	return;
}

void syscallExec(struct TrapFrame *tf) {
	int sel = tf->ds;
	char *str = (char *)tf->ecx;
	char tmp[128];
	int i = 0;
	char character = 0;
	int ret = 0;
	uint32_t entry = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	while (character != 0) {
		tmp[i] = character;
		i++;
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	}
	tmp[i] = 0;

	ret = loadElf(tmp, (current + 1) * 0x100000, &entry);
	if (ret == -1) {
		tf->eax = -1;
		return;
	}
	for (int j = 0; j < MAX_SIG_NUM; j++) {
		pcb[current].handler[j] = -1;
	}
	stringCpy(tmp, pcb[current].name, 32);
	tf->eip = entry;
	return;
}

void syscallSleep(struct TrapFrame *tf) {
	if (tf->ecx == 0) {
		return;
	}
	else {
		pcb[current].state = STATE_SLEEP;
		pcb[current].sleepTime = tf->ecx;
		asm volatile("int $0x20");
		return;
	}
	return;
}

void syscallSchedYield(struct TrapFrame *tf) {
	if (pcb[current].state == STATE_RUNNING && !isblocked(&(pcb[current].blocked))) {
		pcb[current].timeCount = MAX_TIME_COUNT;
		tf->eax = 0;
		asm volatile("int $0x20");
		return;
	}
	tf->eax = -1;
	return;
}

void syscallExit(struct TrapFrame *tf) {
	send_signal(pcb[current].ppid, SIG_CHLD);
	pcb[current].ppid = 0;
	for (int i = 0; i < MAX_PCB_NUM; i++) {
		if (pcb[i].ppid == pcb[current].pid) {
			pcb[i].ppid = 1;
		}
	}
	for (int i = 0; i < MAX_SIG_NUM; i++) {
		pcb[current].handler[i] = -1;
		pcb[current].signal[i] = 0;
	}
	wakeup(&(pcb[current].blocked));
	
	pcb[current].state = STATE_DEAD;
	
	bufferHead = bufferTail; // clear input buffer
	asm volatile("int $0x20");
	return;
}

void syscallSignal(struct TrapFrame *tf) {
	int signum = tf->ecx;
	int handler = tf->edx;
	if (signum < MAX_SIG_NUM) {
		pcb[current].handler[signum] = handler;
		tf->eax = 0;
		return;
	}
	tf->eax = -1;
	return;
}

void syscallKill(struct TrapFrame *tf) {
	int pid = tf->ecx;
	int signum = tf->edx;
	if (signum < MAX_SIG_NUM) {
		if (pid == -1) {
			int success = -1;
			for (int i = 2; i < MAX_PCB_NUM; i++) {
				if (i != pcb[current].pid) {
					if (send_signal(i, signum) != -1) {
						success = 0;
					}
				}
			}
			tf->eax = success;
			return;
		}
		else {
			tf->eax = send_signal(pid, signum);
			return;
		}
	}
	tf->eax = -1;
	return;
}

void syscallSem(struct TrapFrame *tf) {
	switch(tf->ecx) {
		case SEM_INIT:
			syscallSemInit(tf);
			break;
		case SEM_WAIT:
			syscallSemWait(tf);
			break;
		case SEM_POST:
			syscallSemPost(tf);
			break;
		case SEM_DESTROY:
			syscallSemDestroy(tf);
			break;
		default:break;
	}
}

void syscallSemInit(struct TrapFrame *tf) {
	// TODO in lab4
	int value = tf->edx;
	int i = 0;
	while (i < MAX_SEM_NUM) {
		if (sem[i].state == 0) break;
		else i++;
	}
	if (i == MAX_SEM_NUM) {
		tf->eax = -1;
		return;
	}
	sem[i].value = value;
	sem[i].state = 1;
	tf->eax = (uint32_t)&(sem[i]);
	return;
}

void syscallSemWait(struct TrapFrame *tf) {
	// TODO in lab4
	Semaphore *s = (Semaphore *)(tf->edx);
	if (s->state == 0) {
		tf->eax = -1;
		return;
	}
	s->value--;
	tf->eax = 0;
	if (s->value < 0) {
		blockon(&(pcb[current].blocked), &(s->pcb));
		asm volatile("int $0x20");
	}
	return;
}

void syscallSemPost(struct TrapFrame *tf) {
	// TODO in lab4
	Semaphore *s = (Semaphore *)(tf->edx);
	if (s->state == 0) {
		tf->eax = -1;
		return;
	}
	if (s->value <= 0) {
		wakeup(s->pcb.next);
	}
	tf->eax = 0;
	return;
}

void syscallSemDestroy(struct TrapFrame *tf) {
	// TODO in lab4
	Semaphore *s = (Semaphore *)(tf->edx);
	if (s->state == 0) {
		tf->eax = -1;
	}
	else {
		struct ListHead *p = s->pcb.next;
		while (p != &(s->pcb)) {
			struct ListHead *tmp = p;
			p = p->next;
			wakeup(tmp);
		}
		s->state = 0;
		s->value = 0;
		s->pcb.next = &(s->pcb);
		s->pcb.prev = &(s->pcb);
		tf->eax = 0;
	}
	return;
}

void syscallGetPid(struct TrapFrame *tf) {
	pcb[current].regs.eax = pcb[current].pid;
	return;
}

void syscallGetPPid(struct TrapFrame *tf) {
	pcb[current].regs.eax = pcb[current].ppid;
	return;
}

void syscallGetTime(struct TrapFrame *tf) {
	pcb[current].regs.eax = ticks;
	return;
}

void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}
