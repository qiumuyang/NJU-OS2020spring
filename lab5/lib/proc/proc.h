#ifndef __PROC_H__
#define __PROC_H__

#include "types.h"


pid_t fork();
int exec(const char *filename, char * const argv[]);
int sleep(uint32_t time);
int sched_yield();
int exit();
pid_t getpid();
pid_t getppid();
int signal(int signum, sighandler_t handler);
int kill(pid_t pid, int signum);
int waitpid(pid_t pid);

#endif