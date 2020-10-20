#include "x86.h"
#include "device.h"
#include "fs.h"
#include "mm.h"


const int do_heap_log = 0;

void initHeap(int physAddr) {
    Node_t *nodes = (Node_t *)(physAddr + HEAP_START) - MAX_HEAP_NODE;
    for (int i = 1; i < MAX_HEAP_NODE; i++) {
        nodes[i].state = 0;
        nodes[i].size = 0;
        nodes[i].pivot = 0;
        nodes[i].next = NULL;
        nodes[i].prev = NULL;
    }
    nodes[1].state = 1;
    nodes[1].size = MAX_HEAP_SIZE;
    nodes[1].pivot = HEAP_START;
    nodes[1].prev = &nodes[0];
    nodes[0].pivot = 0;
    nodes[0].next = &nodes[1];
}

static void mergeNodes(int physAddr) {
    Node_t *nodes = (Node_t *)(physAddr + HEAP_START) - MAX_HEAP_NODE;
    for (Node_t *p = nodes->next; p;) {
        assert(p->state == 1);
        if (p->next && p->next->pivot == p->pivot + p->size) {
            Node_t *check = p->next;
            while (check && check->pivot == p->pivot + p->size) {
                p->size += check->size;
                Node_t *del = check;
                check = check->next;
                del->state = 0;
                del->size = 0;
                del->pivot = 0;
                if (del->next)
                    del->next->prev = p;
                p->next = del->next;
                del->next = NULL;
                del->prev = NULL;
            }
            p = check;
        }
        else
            p = p->next;
    }
}

static void showNodes(int physAddr) {
    Node_t *nodes = (Node_t *)(physAddr + HEAP_START) - MAX_HEAP_NODE;
    putString("-------------Nodes-------------\n");
    putString("Free:\n");
    for (Node_t *p = nodes->next; p; p = p->next) {
        putString("Node");
        putInt(p - nodes, ' ');
        putString("start ");
        putHex(p->pivot + physAddr, ' ');
        putString("size ");
        putHex(p->size, '\n');
    }
    putString("Occupy:\n");
    for (int i = 1; i < MAX_HEAP_NODE; i++) {
        if (nodes[i].state == 2) {
            putString("Node");
            putInt(i, ' ');
            putString("start ");
            putHex(nodes[i].pivot + physAddr, ' ');
            putString("size ");
            putHex(nodes[i].size, '\n');
        }
    }
    putString("-------------------------------\n");
}

void *kalloc(int size, int physAddr) {
    if (size <= 0)
        return NULL;
    if (size < 8)
        size = 8;
    int aligned = pow2(size);
    Node_t *nodes = (Node_t *)(physAddr + HEAP_START) - MAX_HEAP_NODE;
    if (do_heap_log) {
        putString("Alloc from ");
        putHex((int)nodes, '\n');
    }
    for (Node_t *p = nodes->next; p; p = p->next) {
        assert(p->state == 1);
        if (p->size < aligned)
            continue;
        uint32_t mm_begin = mult4(p->pivot);
        uint32_t mm_end = mm_begin + aligned;
        uint32_t actual_size = mm_end - p->pivot;
        if (p->size < actual_size)
            continue;
        int i;
        for (i = 1; i < MAX_HEAP_NODE; i++) {
            if (nodes[i].state == 0)
                break;
        }
        if (i == MAX_HEAP_NODE)
            return NULL;
        nodes[i].state = 2;
        nodes[i].size = actual_size;
        nodes[i].pivot = p->pivot;
        nodes[i].next = NULL;
        nodes[i].prev = NULL;
        p->size -= actual_size;
        p->pivot = mm_end;
        if (p->size == 0) {
            p->prev->next = p->next;
            if (p->next)
                p->next->prev = p->prev;
            p->state = 0;
        }
        if (do_heap_log)
            showNodes(physAddr);
        memset((void *)(mm_begin + physAddr), 0, aligned);
        return (void *)mm_begin;
    }
    if (do_heap_log)
        showNodes(physAddr);
    return NULL;
}

void kfree(void *ptr, int physAddr) {
    Node_t *nodes = (Node_t *)(physAddr + HEAP_START) - MAX_HEAP_NODE;
    if (do_heap_log) {
        putString("Free from ");
        putHex((int)nodes, '\n');
    }
    int i;
    for (i = 1; i < MAX_HEAP_NODE; i++) {
        if (nodes[i].state == 2 && mult4(nodes[i].pivot) == (uint32_t)ptr)
            break;
    }
    if (i == MAX_HEAP_NODE)
        return;
    nodes[i].state = 1;
    uint32_t pivot = nodes[i].pivot;
    for (Node_t *p = nodes->next; p; p = p->next) {
        if (p->prev->pivot <= pivot && pivot <= p->pivot) {
            p->prev->next = &nodes[i];
            nodes[i].prev = p->prev;
            nodes[i].next = p;
            p->prev = &nodes[i];
            break;
        }
        else if (p->next == NULL) {
            p->next = &nodes[i];
            nodes[i].next = NULL;
            nodes[i].prev = p;
        }
    }
    mergeNodes(physAddr);
    if (do_heap_log)
        showNodes(physAddr);
}