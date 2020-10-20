#include "lib.h"
#include "types.h"


pid_t fork()
{
    return syscall(SYS_FORK, 0, 0, 0, 0, 0);
}

int exec(const char *filename, char *const argv[])
{
    char buf[256];
    strcpy(buf, filename);
    relPath(buf);
    return syscall(SYS_EXEC, (uint32_t)buf, (uint32_t)argv, 0, 0, 0);
}

int sleep(uint32_t time)
{
    return syscall(SYS_SLEEP, (uint32_t)time, 0, 0, 0, 0);
}

int sched_yield()
{
    return syscall(SYS_SCHEDYIELD, 0, 0, 0, 0, 0);
}

int signal(int signum, sighandler_t handler)
{
    return syscall(SYS_SIG, signum, (uint32_t)handler, 0, 0, 0);
}

int kill(pid_t pid, int signum)
{
    return syscall(SYS_KILL, pid, signum, 0, 0, 0);
}

int exit()
{
    return syscall(SYS_EXIT, 0, 0, 0, 0, 0);
}

pid_t getpid()
{
    return syscall(SYS_GETPID, 0, 0, 0, 0, 0);
}

pid_t getppid()
{
    return syscall(SYS_GETPPID, 0, 0, 0, 0, 0);
}

int waitpid(pid_t pid)
{
    return syscall(SYS_WAITPID, pid, 0, 0, 0, 0);
}