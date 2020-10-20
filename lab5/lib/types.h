#ifndef __TYPES_H__
#define __TYPES_H__

#define true 1
#define false 0

typedef unsigned int   uint32_t;
typedef          int   int32_t;
typedef unsigned short uint16_t;
typedef          short int16_t;
typedef unsigned char  uint8_t;
typedef          char  int8_t;
typedef unsigned char  boolean;
typedef unsigned char  bool;

typedef uint32_t size_t;
typedef int32_t pid_t;
typedef int32_t sem_t;
typedef void (*sighandler_t)(int);

struct FileStat {
	int type;
	int inode;
	int blocks;
	int links;
	int size;
	char name[64];
};
typedef struct FileStat FileStat;

#endif
