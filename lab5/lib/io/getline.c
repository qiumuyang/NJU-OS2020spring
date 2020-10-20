#include "lib.h"
#include "types.h"


int getline(char *buf, int size) {
    int i = 0;
    int idx = 0;
    int ret;
    char buffer[MAX_BUFFER_SIZE];
    while (i < size) {
        do {
			ret = syscall(SYS_READ, STD_IN, (uint32_t)buffer, (uint32_t)MAX_BUFFER_SIZE, 0, 0);
		} while(ret == 0 || ret == -1);
        idx = 0;
        while (buffer[idx]) {
            buf[i] = buffer[idx];
            i++;
            idx++;
            if (i == size)
                break;
        }
        if (buf[i-1] == '\n')
            break;
    }
    buf[i-1] = '\0';
    return i;
}