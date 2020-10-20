#ifndef __lib_h__
#define __lib_h__

#include "types.h"
#include "macro.h"
#include "string/string.h"
#include "io/io.h"
#include "proc/proc.h"


int32_t syscall(int num, uint32_t a1,uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5);

char *itoa(int i, char *buf);
int atoi(const char *str);

void *memcpy(void *dst, const void *src, int n);
void *memset(void *dst, uint8_t c, int n);

void *malloc(int size);
void free(void *ptr);

int sem_init(sem_t *sem, uint32_t value);
int sem_wait(sem_t *sem);
int sem_post(sem_t *sem);
int sem_destroy(sem_t *sem);

uint32_t time();

uint32_t rand();
void srand(uint32_t seed);

void clear();

#endif