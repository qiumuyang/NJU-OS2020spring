#ifndef __sem_h__
#define __sem_h__

#define SEM_INIT 0
#define SEM_WAIT 1
#define SEM_POST 2
#define SEM_DESTROY 3

void syscallSem(struct TrapFrame *tf);

void syscallSemInit(struct TrapFrame *tf);
void syscallSemWait(struct TrapFrame *tf);
void syscallSemPost(struct TrapFrame *tf);
void syscallSemDestroy(struct TrapFrame *tf);

void blockon(struct ListHead *blocked, struct ListHead *sem_or_dev);
void wakeup(struct ListHead *Wake);

int semDestroy(int id);

#endif