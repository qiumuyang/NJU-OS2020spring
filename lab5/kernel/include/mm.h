#ifndef __mm_h__
#define __mm_h__

#include "x86.h"

// code data bss    0x0 - 0x40000
// heap nodes   0x40000 - 0x60000
// heap content 0x60000 - 0xc0000
// stack        0xc0000 - 0xfffff

#define MAX_HEAP_SIZE 0x60000
#define HEAP_START 0x60000
#define MAX_HEAP_NODE 4096

// allow malloc 4096 times at most for each process

typedef struct Node {
    uint32_t state; // 0: not used 1: avail 2: occupied
    uint32_t size;
    uint32_t pivot;
    struct Node* next;
    struct Node* prev;
} Node_t;

static inline int pow2(int size) {
    int n = size - 1;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return (n < 0) ? 1 : n + 1;
}

static inline int mult4(int i) {
    int k = !!(i & 0x3);
    return (i & ~0x3) + k * 4;
}

void initHeap(int physAddr);
void *kalloc(int size, int physAddr);
void kfree(void *ptr, int physAddr);

#endif