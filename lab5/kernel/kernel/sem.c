#include "x86.h"
#include "device.h"
#include "sem.h"

extern int current;
extern ProcessTable pcb[MAX_PCB_NUM];

extern Semaphore sem[MAX_SEM_NUM];

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
	sem[i].creator = current;
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

int semDestroy(int id) {
	Semaphore *s = sem + id;
	if (s->state == 0) {
		return -1;
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
		s->creator = 0;
		s->pcb.next = &(s->pcb);
		s->pcb.prev = &(s->pcb);
		return 0;
	}
}

void syscallSemDestroy(struct TrapFrame *tf) {
	// TODO in lab4
	Semaphore *s = (Semaphore *)(tf->edx);
	int id = s - sem;
	if (id < 0 || id >= MAX_SEM_NUM)
		tf->eax = -1;
	tf->eax = semDestroy(id);
	return;
}
