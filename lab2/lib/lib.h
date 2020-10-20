#ifndef __lib_h__
#define __lib_h__

#define SYS_WRITE 0
#define STD_OUT 0
#define SYS_CLRSCR 1000
#define SYS_TIME 1001

#define MAX_BUFFER_SIZE 512
//#define MAX_BUFFER_SIZE 1

void sleep(unsigned seconds);
void printf(const char *format,...);
void clrscr();

#endif
