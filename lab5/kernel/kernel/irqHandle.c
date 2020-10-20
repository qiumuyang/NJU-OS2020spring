#include "x86.h"
#include "device.h"
#include "sem.h"
#include "fs.h"
#include "mm.h"

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
#define SYS_OPEN 13
#define SYS_LSEEK 14
#define SYS_CLOSE 15
#define SYS_REMOVE 16
#define SYS_STAT 17
#define SYS_DUP 18
#define SYS_DUP2 19
#define SYS_MALLOC 20
#define SYS_FREE 21
#define SYS_GETCWD 22
#define SYS_CHDIR 23
#define SYS_WAITPID 24
#define SYS_CLR 25

#define SIG_INT 2
#define SIG_TERM 15
#define SIG_CHLD 17

extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern File file[MAX_FILE_NUM];
extern int rootDirInode;

extern Semaphore sem[MAX_SEM_NUM];
extern Device dev[MAX_DEV_NUM];

extern int displayRow;
extern int displayCol;
extern int baseRow;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;
extern uint8_t ctrl;

extern uint32_t ticks;

uint8_t shMem[MAX_SHMEM_SIZE];

const int keyboard_echo = 1;

void syscallHandle(struct TrapFrame *tf);

void syscallWrite(struct TrapFrame *tf);
void syscallRead(struct TrapFrame *tf);
void syscallOpen(struct TrapFrame *tf);
void syscallLseek(struct TrapFrame *tf);
void syscallClose(struct TrapFrame *tf);
void syscallRemove(struct TrapFrame *tf);
void syscallStat(struct TrapFrame *tf);
void syscallDup(struct TrapFrame *tf);
void syscallDup2(struct TrapFrame *tf);
void syscallGetcwd(struct TrapFrame *tf);
void syscallChdir(struct TrapFrame *tf);

void syscallFork(struct TrapFrame *tf);
void syscallExec(struct TrapFrame *tf);
void syscallSleep(struct TrapFrame *tf);
void syscallExit(struct TrapFrame *tf);
void syscallGetPid(struct TrapFrame *tf);
void syscallGetPPid(struct TrapFrame *tf);
void syscallSchedYield(struct TrapFrame *tf);
void syscallWaitpid(struct TrapFrame *tf);

void syscallGetTime(struct TrapFrame *tf);

void syscallSignal(struct TrapFrame *tf);
void syscallKill(struct TrapFrame *tf);

void syscallFree(struct TrapFrame *tf);
void syscallMalloc(struct TrapFrame *tf);

void syscallWriteStdOut(struct TrapFrame *tf);
void syscallReadStdIn(struct TrapFrame *tf);
void syscallWriteShMem(struct TrapFrame *tf);
void syscallReadShMem(struct TrapFrame *tf);
void syscallClear(struct TrapFrame *tf);

void GProtectFaultHandle(struct TrapFrame *tf);

void timerHandle(struct TrapFrame *tf);
void keyboardHandle(struct TrapFrame *tf);

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

void do_exit(int pid) {
	send_signal(pcb[pid].ppid, SIG_CHLD);
	pcb[pid].ppid = 0;
	for (int i = 0; i < MAX_PCB_NUM; i++) {
		if (pcb[i].ppid == pcb[pid].pid) {
			pcb[i].ppid = 1;
		}
	}
	for (int i = 0; i < MAX_SIG_NUM; i++) {
		pcb[pid].handler[i] = -1;
		pcb[pid].signal[i] = 0;
	}

	for (int i = 0; i < MAX_FILE_NUM; i++) {
		if (pcb[pid].fd[i] != -1) {
			fs_close(pcb[pid].fd[i]);
			pcb[pid].fd[i] = -1;
		}
	}
	pcb[pid].fd[STD_OUT] = STD_OUT;
	pcb[pid].fd[STD_IN] = STD_IN;
	pcb[pid].fd[STD_ERR] = STD_ERR;
	pcb[pid].fd[SH_MEM] = SH_MEM;

	for (int i = 0; i < MAX_SEM_NUM; i++) {
		if (sem[i].state == 1 && sem[i].creator == pid) {
			semDestroy(i);
		}
	}
	struct ListHead *p = pcb[pid].pcb.next;
	while (p != &(pcb[pid].pcb)) {
		struct ListHead *tmp = p;
		p = p->next;
		wakeup(tmp);
	}
	wakeup(&(pcb[pid].blocked));
	
	pcb[pid].state = STATE_DEAD;
	
	bufferHead = bufferTail; // clear input buffer
	asm volatile("int $0x20");
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
			else if ((i == SIG_INT || i == SIG_TERM) && current != 0 && current != 1) {
				do_exit(current);
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
		case SYS_OPEN:
			syscallOpen(tf);
			break; // for SYS_OPEN
		case SYS_LSEEK:
			syscallLseek(tf);
			break; // for SYS_SEEK
		case SYS_CLOSE:
			syscallClose(tf);
			break; // for SYS_CLOSE
		case SYS_REMOVE:
			syscallRemove(tf);
			break; // for SYS_REMOVE
		case SYS_STAT:
			syscallStat(tf);
			break;
		case SYS_DUP:
			syscallDup(tf);
			break;
		case SYS_DUP2:
			syscallDup2(tf);
			break;
		case SYS_MALLOC:
			syscallMalloc(tf);
			break;
		case SYS_FREE:
			syscallFree(tf);
			break;
		case SYS_CHDIR:
			syscallChdir(tf);
			break;
		case SYS_GETCWD:
			syscallGetcwd(tf);
			break;
		case SYS_WAITPID:
			syscallWaitpid(tf);
			break;
		case SYS_CLR:
			syscallClear(tf);
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
	if (keyCode == 0xe) {
		if (dev[STD_IN].state == 1 && dev[STD_IN].value < 0) {
			if (bufferTail != bufferHead) {
				if (keyboard_echo) {
					putChar('\b');
					putChar(' ');
					putChar('\b');
				}
				if (bufferTail > 0)
					bufferTail --;
				else
					bufferTail = MAX_KEYBUFFER_SIZE - 1;
				eraseVga();
			}
		}
	}
	else if (keyCode == 0x68) {
		//putString("↑");
		if (displayRow > 24 && baseRow > 0) {
			baseRow--;
			updateScreen();
		}
	}
	else if (keyCode == 0x70) {
		//putString("↓");
		if (baseRow < displayRow - 24) {
			baseRow++;
			updateScreen();
		}
	}
	else if (keyCode == 0x6b) {
		//putString("←");
	}
	else if (keyCode == 0x6d) {
		//putString("→");
	}
	if (keyCode == 0)
		return;
	char keyChar = getChar(keyCode);
	if (keyChar == 0)
		return;
	if (ctrl && keyboard_echo) putChar('^');
	if (ctrl && keyChar >= 'a' && keyChar <= 'z')
		keyChar += 'A' - 'a';
	if (keyboard_echo) putChar(keyChar);
	if (dev[STD_IN].state == 1 && dev[STD_IN].value < 0) {
		if (ctrl) writeVga('^' | 0xf00);
		writeVga(keyChar | 0xf00);
	}
	if (!ctrl) {
		keyBuffer[bufferTail] = keyChar;
		bufferTail = (bufferTail + 1) % MAX_KEYBUFFER_SIZE;
	}
	if (keyChar == '\n') {
		if (dev[STD_IN].state == 1 && dev[STD_IN].value < 0) {
			dev[STD_IN].value = 1;
			wakeup(dev[STD_IN].pcb.next);
		}
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
			default:
				break;
		}
	}
	return;
}

void syscallClear(struct TrapFrame *tf) {
	clearScreen();
}

void syscallGetcwd(struct TrapFrame *tf) {
	int sel = tf->ds;
	char *str = (char *)tf->ecx;
	int size = tf->edx;
	char tmp[128] = {0};

	int ret = fs_getDirPath(pcb[current].cwd, tmp);
	if (ret < 0) {
		tf->eax = ret;
		return;
	}
	if (strlen(tmp) + 1 > size) {
		tf->eax = ERANGE;
		return;
	}
	writeUsrBytes((uint8_t *)str, (uint8_t *)tmp, sel, strlen(tmp) + 1);
	tf->eax = 0;
}

void syscallChdir(struct TrapFrame *tf) {
	char tmp[128] = {0};
	readUsrString(tmp, (char *)tf->ecx, tf->ds);
	int ret = fs_getDirInode(tmp);
	if (ret < 0) {
		tf->eax = ret;
		return;
	}
	pcb[current].cwd = ret;
	tf->eax = 0;
}

void syscallFree(struct TrapFrame *tf) {
	kfree((void *)tf->ecx, (current + 1)*0x100000);
}

void syscallMalloc(struct TrapFrame *tf) {
	tf->eax = (uint32_t)kalloc(tf->ecx, (current + 1)*0x100000);
}

void syscallDup(struct TrapFrame *tf) {
	int i = tf->ecx;
	if (i < 0 || i >= MAX_FILE_NUM) {
		tf->eax = EBADF;
		return;
	}
	int fd = pcb[current].fd[i];
	if (fd < 0 || fd >= MAX_FILE_NUM) {
		tf->eax = EBADF;
		return;
	}
	for (i = 0; i < MAX_FILE_NUM; i++) {
		if (pcb[current].fd[i] == -1)
			break;
	}
	if (i == MAX_FILE_NUM) {
		tf->eax = EMFILE;
		return;
	}
	fs_fork(fd);
	pcb[current].fd[i] = fd;
	tf->eax = i;
}

void syscallDup2(struct TrapFrame *tf) {
	int i = tf->ecx;
	int j = tf->edx;
	if (i < 0 || i >= MAX_FILE_NUM || j < 0 || j >= MAX_FILE_NUM) {
		tf->eax = EBADF;
		return;
	}
	int oldfd = pcb[current].fd[i];
	int newfd = pcb[current].fd[j];
	if (oldfd < 0 || oldfd >= MAX_FILE_NUM || oldfd == -1) {
		tf->eax = EBADF;
		return;
	}
	fs_close(newfd);
	fs_fork(oldfd);
	pcb[current].fd[j] = oldfd;
	tf->eax = j;
}

void syscallStat(struct TrapFrame *tf) {
	int sel = tf->ds;
	int i = tf->ecx;
	if (i < 0 || i >= MAX_FILE_NUM) {
		tf->eax = EBADF;
		return;
	}
	int fd = pcb[current].fd[i];
	uint8_t *dst = (uint8_t *)(tf->edx);
	FileStat stat;
	int ret = fs_stat(fd, &stat);
	if (ret < 0) {
		tf->eax = ret;
		return;
	}
	uint8_t *p = (uint8_t *)&stat;
	uint8_t data;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (int i = 0; i < sizeof(FileStat); i++) {
		data = p[i];
		asm volatile("movb %0, %%es:(%1)"::"r"(data),"r"(dst + i));
	}
	tf->eax = 0;
}

void syscallOpen(struct TrapFrame *tf) {
	int sel = tf->ds;
	char *str = (char *)tf->ecx;
	int flags = tf->edx;
	int i = 0;
	char character;
	char tmp[256];

	asm volatile("movw %0, %%es"::"m"(sel));
	asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	while (character != 0) {
		tmp[i] = character;
		i++;
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	}
	tmp[i] = 0;

	for (i = 0; i < MAX_FILE_NUM; i++) {
		if (pcb[current].fd[i] == -1)
			break;
	}
	if (i == MAX_FILE_NUM) {
		tf->eax = EMFILE;
		return;
	}
	int ret = fs_open(tmp, flags);
	if (ret >= 0) {
		pcb[current].fd[i] = ret;
		tf->eax = i;
	}
	else
		tf->eax = ret;
}

void syscallLseek(struct TrapFrame *tf) {
	int i = tf->ecx;
	if (i < 0 || i >= MAX_FILE_NUM) {
		tf->eax = EBADF;
		return;
	}
	int fd = pcb[current].fd[i];
	int offset = tf->edx;
	int whence = tf->ebx;
	tf->eax = fs_lseek(fd, offset, whence);
}

void syscallClose(struct TrapFrame *tf) {
	int i = tf->ecx;
	if (i < 0 || i >= MAX_FILE_NUM) {
		tf->eax = EBADF;
		return;
	}
	int fd = pcb[current].fd[i];
	if (fd < 4) { 	// STD_IN STD_OUT etc. cannot be closed
		tf->eax = EBADF;
		return;
	}
	pcb[current].fd[i] = -1;
	tf->eax = fs_close(fd);
}

void syscallRemove(struct TrapFrame *tf) {
	int sel = tf->ds;
	char *str = (char *)tf->ecx;
	int i = 0;
	char character;
	char tmp[256];

	asm volatile("movw %0, %%es"::"m"(sel));
	asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	while (character != 0) {
		tmp[i] = character;
		i++;
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	}
	tmp[i] = 0;

	tf->eax = fs_remove(tmp);
}

void syscallWrite(struct TrapFrame *tf) {
	int i = tf->ecx;
	if (i < 0 || i >= MAX_FILE_NUM) {
		tf->eax = EBADF;
		return;
	}
	int fd = pcb[current].fd[i];
	switch(fd) { // file descriptor
		case STD_OUT:
		case STD_ERR:
			if (dev[STD_OUT].state == 1) {
				syscallWriteStdOut(tf);
			}
			break; // for STD_OUT
		case SH_MEM:
			if (dev[SH_MEM].state == 1) {
				syscallWriteShMem(tf);
			}
			break; // for SH_MEM
		case STD_IN:
			break;
		default:
			tf->eax = fs_write(fd, (uint8_t *)tf->edx, tf->ebx, tf->ds);
	}
}

void syscallWriteStdOut(struct TrapFrame *tf) {
	int sel = tf->ds; //TODO segment selector for user data, need further modification
	char *str = (char *)tf->edx;
	int size = tf->ebx;
	int font = tf->esi;
	int bg = tf->edi;
	int i = 0;
	//int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	if (font < 0 || font >= 16) font = 0xc;
	if (bg < 0 || bg >= 16) bg = 0x0;
	if (bg == 0 && font == 0) font = 0xc;
	uint32_t color = (bg << 4) | font;
	
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
		data = character | (color << 8);
		writeVga(data);
	}
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
}

void syscallRead(struct TrapFrame *tf) {
	int i = tf->ecx;
	if (i < 0 || i >= MAX_FILE_NUM) {
		tf->eax = EBADF;
		return;
	}
	int fd = pcb[current].fd[i];
	switch(fd) {
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
		case STD_OUT:
		case STD_ERR:
			break;
		default:
			tf->eax = fs_read(fd, (uint8_t *)tf->edx, tf->ebx, tf->ds);
	}
}

void syscallReadStdIn(struct TrapFrame *tf) {
	// TODO in lab4
	int size = tf->ebx;
	if (dev[STD_IN].value > 0) {
		dev[STD_IN].value = 0;
		int sel = tf->ds;
		int i = 0;
		char *str = (char *)tf->edx;
		asm volatile("movw %0, %%es"::"m"(sel));
		for (i = 0; bufferHead != bufferTail; bufferHead = (bufferHead + 1) % MAX_KEYBUFFER_SIZE, i++) {
			if (i == size) break;
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
		for (j = 0; j < MAX_FILE_NUM; j++) {
			pcb[i].fd[j] = pcb[current].fd[j];
			if (pcb[i].fd[j] != -1)
				fs_fork(pcb[i].fd[j]);
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
		pcb[i].cwd = pcb[current].cwd;
		initHeap((i+1)*0x100000); // ???
		pcb[current].regs.eax = i;
	}
	else {
		pcb[current].regs.eax = -1;
	}
	return;
}

void syscallExec(struct TrapFrame *tf) {
	int sel = tf->ds;
	int stack = tf->ss;
	char *str = (char *)tf->ecx;
	char **argv = (char **)tf->edx;

	char tmp[128];
	int ret = 0;
	uint32_t entry = 0;
	readUsrString(tmp, str, sel);
	
	int esp = 0x100000;
	int ptr[MAX_ARGV_NUM];
	int argc = 0;
	char argtmp[MAX_ARGV_NUM][128];
	int len = 0;

	if (argv != NULL) {
		// copy all argv into kernel
		char *arg_item;
		readUsr((void *)&arg_item, (void *)(argv+argc), sel, sizeof(char *));
		while (arg_item != NULL) {
			readUsrString(argtmp[argc], arg_item, sel);
			argc++;
			if (argc >= MAX_ARGV_NUM)
				break;
			readUsr((void *)&arg_item, (void *)(argv+argc), sel, sizeof(char *));
		}

		// write argv into process's stack
		for (int i = 0; i < argc; i++) {
			len = strlen(argtmp[i]) + 1;
			esp -= len;
			while (esp % 4 != 0) esp--;
			writeUsrBytes((uint8_t *)esp, (uint8_t *)argtmp[i], stack, len);
			ptr[i] = esp;
		}

		// write each argv's beginning pos into stack
		for (int i = argc - 1; i >= 0; i--) {
			esp -= 4;
			writeUsrBytes((uint8_t *)esp, (uint8_t *)&(ptr[i]), stack, 4);
		}

		// push argv, argc
		int argv_addr = esp;
		esp -= 4;
		writeUsrBytes((uint8_t *)esp, (uint8_t *)&argv_addr, stack, 4);
		esp -= 4;
		writeUsrBytes((uint8_t *)esp, (uint8_t *)&argc, stack, 4);
		esp -= 4;
	}
	initHeap((current + 1) * 0x100000);
	ret = loadElf(tmp, (current + 1) * 0x100000, &entry);
	if (ret == -1) {
		tf->eax = -1;
		return;
	}
	for (int j = 0; j < MAX_SIG_NUM; j++) {
		pcb[current].handler[j] = -1;
	}
	char *filename = strrchr(tmp, '/');
	if (filename != NULL)
		stringCpy(filename, pcb[current].name, 32);
	else
		stringCpy(tmp, pcb[current].name, 32);

	tf->eip = entry;
	tf->esp = esp;
}

void syscallSleep(struct TrapFrame *tf) {
	if (tf->ecx < 0) {
		tf->eax = -1;
		return;
	}
	else if (tf->ecx == 0){
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
	}
	tf->eax = -1;
	return;
}

void syscallExit(struct TrapFrame *tf) {
	do_exit(current);
	return;
}

void syscallWaitpid(struct TrapFrame *tf) {
	int pid = tf->ecx;
	if (pid < 0 || pid >= MAX_PCB_NUM || pcb[pid].state == STATE_DEAD || pid == current) {
		tf->eax = -1;
		return;
	}
	blockon(&(pcb[current].blocked), &(pcb[pid].pcb));
	tf->eax = 0;
	asm volatile("int $0x20");
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
