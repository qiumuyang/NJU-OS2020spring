#include "x86.h"
#include "device.h"
#include "fs.h"
#define PT_LOAD 1

SegDesc gdt[NR_SEGMENTS];       // the new GDT, NR_SEGMENTS=7, defined in x86/memory.h
TSS tss;

SuperBlock sBlock;

ProcessTable pcb[MAX_PCB_NUM]; // pcb
int current; // current process

/*
MACRO
SEG(type, base, lim, dpl) (SegDesc) {...};
SEG_KCODE=1
SEG_KDATA=2
SEG_UCODE=3
SEG_UDATA=4
SEG_TSS=5
DPL_KERN=0
DPL_USER=3
KSEL(desc) (((desc)<<3) | DPL_KERN)
USEL(desc) (((desc)<<3) | DPL_UERN)
asm [volatile] (AssemblerTemplate : OutputOperands [ : InputOperands [ : Clobbers ] ])
asm [volatile] (AssemblerTemplate : : InputOperands : Clobbers : GotoLabels)
*/
void initSeg() { // setup kernel segements
	int i = 0;
	gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0,       0xffffffff, DPL_KERN);
	gdt[SEG_KDATA] = SEG(STA_W,         0,       0xffffffff, DPL_KERN);

	for (i = 1; i < MAX_PCB_NUM; i++) {
		gdt[1 + i * 2] = SEG(STA_X | STA_R, (i + 1) * 0x100000, 0x00100000, DPL_USER);
		gdt[2 + i * 2] = SEG(STA_W,         (i + 1) * 0x100000, 0x00100000, DPL_USER);
	}
	
	gdt[SEG_TSS] = SEG16(STS_T32A,      &tss, sizeof(TSS) - 1, DPL_KERN);
	gdt[SEG_TSS].s = 0;

	setGdt(gdt, sizeof(gdt)); // gdt is set in bootloader, here reset gdt in kernel

	/*
	 * 初始化TSS
	 */
	tss.ss0 = KSEL(SEG_KDATA);
	asm volatile("ltr %%ax":: "a" (KSEL(SEG_TSS)));

	/*设置正确的段寄存器*/
	asm volatile("movw %%ax,%%ds":: "a" (KSEL(SEG_KDATA)));
	asm volatile("movw %%ax,%%ss":: "a" (KSEL(SEG_KDATA)));

	lLdt(0);
	
}

void initFS () {
	readSuperBlock(&sBlock);
}

int loadElf(const char *filename, uint32_t physAddr, uint32_t *entry) {
	// TODO in lab3
	Inode inode;
	int inodeOffset = 0;
	uint8_t *buf = (uint8_t *)0x10000;	// keep 0x0 to 0x10000 for argv&filename temp 
	if (readInode(&sBlock, &inode, &inodeOffset, filename) == -1) return -1;
	for (int i = 0; i < inode.blockCount; i++)
		if (readBlock(&sBlock, &inode, i, (uint8_t *)(buf + i * sBlock.blockSize)) == -1) return -1;
	
	struct ELFHeader *elf;
	struct ProgramHeader *ph, *eph;
	int maxPaddr = 0;
	elf = (void *)buf;
	ph = (void *)elf + elf->phoff;
	eph = ph + elf->phnum;
	for (; ph < eph; ph++) {
		if (ph->type == PT_LOAD) {
			uint32_t paddr = ph->paddr + physAddr;
			for (int i = 0; i < ph->filesz; i++)
				*(uint8_t *)(paddr + i) = buf[ph->off + i];
			for (int i = ph->filesz; i < ph->memsz; i++)
				*(uint8_t *)(paddr + i) = 0;
			if (paddr + ph->memsz > maxPaddr && paddr < 0x8048000) maxPaddr = paddr + ph->memsz;
		}
	}
	*entry = elf->entry;
	return maxPaddr;
}

/*
kernel is loaded to location 0x100000, i.e., 1MB
size of kernel is not greater than 200*512 bytes, i.e., 100KB
user program is loaded to location 0x200000, i.e., 2MB
size of user program is not greater than 200*512 bytes, i.e., 100KB
*/

uint32_t loadUMain(void) {
	int i = 0;
	int phoff = 0x34; // program header offset
	int offset = 0x1000; // .text section offset
	uint32_t elf = 0x200000; // physical memory addr to load
	uint32_t uMainEntry = 0x200000;
	Inode inode;
	int inodeOffset = 0;

	readInode(&sBlock, &inode, &inodeOffset, "/boot/initrd");

	for (i = 0; i < inode.blockCount; i++) {
		readBlock(&sBlock, &inode, i, (uint8_t *)(elf + i * sBlock.blockSize));
	}
	
	uMainEntry = ((struct ELFHeader *)elf)->entry; // entry address of the program
	phoff = ((struct ELFHeader *)elf)->phoff;
	offset = ((struct ProgramHeader *)(elf + phoff))->off;

	for (i = 0; i < 200 * 512; i++) {
		*(uint8_t *)(elf + i) = *(uint8_t *)(elf + i + offset);
	}

	// enterUserSpace(uMainEntry);
	return uMainEntry;
}

void initProc() {
	int i = 0;
	for (i = 0; i < MAX_PCB_NUM; i++) {
		pcb[i].state = STATE_DEAD;
	}
	// kernel process
	pcb[0].stackTop = (uint32_t)&(pcb[0].stackTop);
	pcb[0].prevStackTop = (uint32_t)&(pcb[0].stackTop);
	pcb[0].state = STATE_RUNNING;
	pcb[0].timeCount = MAX_TIME_COUNT;
	pcb[0].sleepTime = 0;
	pcb[0].pid = 0;
	// user process
	pcb[1].stackTop = (uint32_t)&(pcb[1].regs);
	pcb[1].prevStackTop = (uint32_t)&(pcb[1].stackTop);
	pcb[1].state = STATE_RUNNABLE;
	pcb[1].timeCount = 0;
	pcb[1].sleepTime = 0;
	pcb[1].pid = 1;
	pcb[1].regs.ss = USEL(4);
	pcb[1].regs.esp = 0x100000;
	asm volatile("pushfl");
	asm volatile("popl %0":"=r"(pcb[1].regs.eflags));
	pcb[1].regs.eflags = pcb[1].regs.eflags | 0x200;
	pcb[1].regs.cs = USEL(3);
	pcb[1].regs.eip = loadUMain();
	pcb[1].regs.ds = USEL(4);
	pcb[1].regs.es = USEL(4);
	pcb[1].regs.fs = USEL(4);
	pcb[1].regs.gs = USEL(4);

	current = 0;
	asm volatile("movl %0, %%esp"::"m"(pcb[0].stackTop));
	enableInterrupt();
	asm volatile("int $0x20");
	while(1) {
		waitForInterrupt();
	}
}