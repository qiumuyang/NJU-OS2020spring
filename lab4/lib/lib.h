#ifndef __lib_h__
#define __lib_h__

#include "types.h"

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

#define MAX_BUFFER_SIZE 256

#define RAND_MAX 32767

int itoa(int i, char *buf);

int strcpy(char *dst, char *src);

int printf(const char *format,...);

int scanf(const char *format,...);

pid_t fork();

int exec(const char *filename, char * const argv[]);

int sleep(uint32_t time);

int sched_yield();

int exit();

int write(int fd, uint8_t *buffer, int size, ...);

int read(int fd, uint8_t *buffer, int size, ...);

int sem_init(sem_t *sem, uint32_t value);

int sem_wait(sem_t *sem);

int sem_post(sem_t *sem);

int sem_destroy(sem_t *sem);

pid_t getpid();

pid_t getppid();

uint32_t time();

uint32_t rand();

void srand(uint32_t seed);

int signal(int signum, sighandler_t handler);

int kill(pid_t pid, int signum);

#endif
