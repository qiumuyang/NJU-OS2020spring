#include "lib.h"
#include "types.h"

static uint32_t next;

uint32_t rand() {
    next = next * 1103515245 + 12345;
    return (uint32_t)((next/65536) % RAND_MAX);
}

void srand(uint32_t seed) {
    next = seed;
}

char *itoa(int i, char *buf) {
	char tmp[15];
	tmp[14] = '\0';
	char *p = tmp + 13;
	int sign = 0;
	if (i == 0x80000000) {
		strcpy(buf, "-2147483648");
		return buf;
	}
	else if (i < 0) {
		sign = 1;
		i *= -1;
	}
	do {
		*--p = '0' + i % 10;
	} while (i /= 10);
	if (sign) {
		*--p = '-';
	}
	strcpy(buf, p);
	return buf;
}

int atoi(const char *str) {
	int sign = 0;
	int ret = 0;
	if (strcmp(str, "-2147483648") == 0)
		return 0x80000000;
	if (str == NULL)
		return 0;
	if (str[0] == '-') {
		sign = 1;
		str++;
	}
	while (*str) {
		if (*str < '0' || *str > '9')
			break;
		ret = ret * 10 + *str - '0';
		str++;
	}
	if (sign) ret *= -1;
	return ret;
}


void *memcpy (void *dst, const void *src, int n) {
	if (src == NULL || dst == NULL)
		return NULL;
	for (int i = 0; i < n; i++) {
		((uint8_t *)dst)[i] = ((uint8_t *)src)[i];
	}
	return dst;
}

void *memset (void *dst, uint8_t c, int n) {
	if (dst == NULL)
		return NULL;
	for (int i = 0; i < n; i++) {
		((uint8_t *)dst)[i] = c;
	}
	return dst;
}

uint32_t time() {
	return syscall(SYS_GETTIME, 0, 0, 0, 0, 0);
}

void *malloc(int size) {
	return (void *)syscall(SYS_MALLOC, size, 0, 0, 0, 0);
}

void free(void *ptr) {
	if (ptr != NULL)
		syscall(SYS_FREE, (uint32_t)ptr, 0, 0, 0, 0);
}

void clear() {
	syscall(SYS_CLR, 0, 0, 0, 0, 0);
}