#ifndef __MACRO_H__
#define __MACRO_H__

#define NULL 0

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

#define STD_OUT 0
#define STD_IN 1
#define SH_MEM 3

#define SEM_INIT 0
#define SEM_WAIT 1
#define SEM_POST 2
#define SEM_DESTROY 3

#define MAX_BUFFER_SIZE 256

#define RAND_MAX 32767

#define O_WRITE 0x01
#define O_READ 0x02
#define O_CREATE 0x04
#define O_DIRECTORY 0x08
#define O_TRUNC 0x10

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define UNKNOWN_TYPE 0
#define REGULAR_TYPE 1
#define DIRECTORY_TYPE 2
#define CHARACTER_TYPE 3
#define BLOCK_TYPE 4
#define FIFO_TYPE 5
#define SOCKET_TYPE 6
#define SYMBOLIC_TYPE 7

#define ENOENT -1
#define EBADF -2
#define EEXIST -3
#define ENOTDIR -4
#define EISDIR -5
#define EMFILE -6
#define EFBIG -7
#define EUNDEF -8
#define EINVAL -9
#define EACCES -10
#define EBADN -11
#define EBADT -12
#define ENOSPC -13
#define ENOTEMPTY -14
#define ERANGE -15

#endif