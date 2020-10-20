#include "x86.h"
#include "device.h"

extern int displayRow;
extern int displayCol;

uint32_t time = 0;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;

void syscallHandle(struct TrapFrame *tf);
void syscallWrite(struct TrapFrame *tf);
void syscallPrint(struct TrapFrame *tf);
void syscallClr(struct TrapFrame *tf);
void syscallTime(struct TrapFrame *tf);

void GProtectFaultHandle(struct TrapFrame *tf);

void timerHandle(struct TrapFrame *tf);
void keyboardHandle(struct TrapFrame *tf);

void irqHandle(struct TrapFrame *tf) { // pointer tf = esp
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));
	//asm volatile("movw %%ax, %%es"::"a"(KSEL(SEG_KDATA)));
	//asm volatile("movw %%ax, %%fs"::"a"(KSEL(SEG_KDATA)));
	//asm volatile("movw %%ax, %%gs"::"a"(KSEL(SEG_KDATA)));
	switch(tf->irq) {
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(tf);
			break;
		case 0x20:
			timerHandle(tf);
			break;
		case 0x21:
			keyboardHandle(tf);
			break;
		case 0x80:
			syscallHandle(tf);
			break;
		default:assert(0);
	}
}

void syscallHandle(struct TrapFrame *tf) {
	switch(tf->eax) { // syscall number
		case 0:
			syscallWrite(tf);
			break; // for SYS_WRITE
		case 1000:
			syscallClr(tf);
			break;
		case 1001:
			syscallTime(tf);
			break;
		default:break;
	}
}

void timerHandle(struct TrapFrame *tf) {
	time++;
	return;
}

void keyboardHandle(struct TrapFrame *tf) {
	// TODO in lab2
	int	code = getKeyCode();
	char character = getChar(code);
	if (character != 0)
	{
		putChar(character);	// put char to serial (stdio)
		/*					// put char to display (can be used to test syscallPrint)
		uint16_t data = character | (0x0c << 8);
		int pos = (80 * displayRow + displayCol)*2;
		if (character != '\n' && character != '\r') 
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
			
		displayCol++;
		if (displayCol == 80 || character == '\n' || character == '\r') {
			displayCol = 0;
			if (displayRow < 24)
				displayRow++;
			else
				scrollScreen();
		}
		updateCursor(displayRow, displayCol);
		*/
	}
}

void syscallClr(struct TrapFrame *tf) {
	clearScreen();
	displayRow = 0;
	displayCol = 0;
	updateCursor(0, 0);
}

void syscallTime(struct TrapFrame *tf) {
	tf->eax = time*10;	// ms
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
	int sel = USEL(SEG_UDATA); //TODO segment selector for user data, need further modification
	char *str = (char*)tf->edx;
	int size = tf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str+i));
		// TODO in lab2
		data = character | (0x0c << 8);
		pos = (80 * displayRow + displayCol)*2;
		if (character != '\n' && character != '\b') 
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos+0xb8000));
			
		if (character != '\b') displayCol++;
		else {
			if (displayCol > 0) displayCol--;
		}
		if (displayCol == 80 || character == '\n') {
			displayCol = 0;
			if (displayRow < 24)
				displayRow++;
			else
				scrollScreen();
		}
	}
	updateCursor(displayRow, displayCol);
}

void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}
