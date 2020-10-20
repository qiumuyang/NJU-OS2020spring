#include "common.h"
#include "device.h"

int strlen (const char *str) {
	int i = 0;
	if (str == NULL) {
		return 0;
	}
	while (str[i] != 0) {
		i++;
	}
	return i;
}

int strcmp (const char* dst, const char *src) {
	int i = 0;
	if (dst == NULL || src == NULL) 
		return -1;
	for (i = 0; dst[i] && src[i]; i++) {
		if (dst[i] < src[i])
			return -1;
		else if (dst[i] > src[i])
			return 1;
		else
			continue;
	}
	if (dst[i] == 0 && src[i] == 0)
		return 0;
	if (dst[i] == 0)
		return -1;
	return 1;
}

char *strchr (const char *str, int c) {
	if (str == NULL)
		return NULL;
	for (int i = 0; str[i]; i++)
		if (str[i] == c)
			return (char *)(str + i);
	return NULL;
}

char *strrchr (const char *str, int c) {
	if (str == NULL)
		return NULL;
	for (int i = strlen(str) - 1; i >= 0; i--)
		if (str[i] == c)
			return (char *)(str + i);
	return NULL;
}

char *strcpy (char *dst, const char* src) {
	if (src == NULL || dst == NULL)
		return NULL;
	int i = 0;
	for (i = 0; src[i]; i++) {
		dst[i] = src[i];
	}
	dst[i] = '\0';
	return dst;
}

char *strcat (char *dst, const char* src) {
	if (src == NULL || dst == NULL)
		return NULL;
	int i = 0;
	int j = strlen(dst);
	for (i = 0; src[i]; i++) {
		dst[j + i] = src[i];
	}
	dst[i + j] = '\0';
	return dst;
}

char* strtok(char* str, const char* delim) {
	static char* tok = NULL;
	if (tok == NULL) tok = str;
	if (tok == NULL) return NULL;
	char* p = tok;
	char* head = tok;
	int len = strlen(delim);
	while (*p) {
		int i = 0;
		for (i = 0; i < len; i++)
			if (*p == delim[i])
				break;
		if (i != len)
			break;
		p++;
	}
	while (*p) {
		int i = 0;
		for (i = 0; i < len; i++)
			if (*p == delim[i])
				break;
		if (i == len)
			break;
		*p = '\0';
		p++;
	}
	if (*p == '\0') tok = NULL;
	else tok = p;
	return head;
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

void readUsrString(char *ker, char *usr, int sel) {
	char character;
	int i = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(usr + i));
	while (character != 0) {
		ker[i] = character;
		i++;
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(usr + i));
	}
	ker[i] = 0;
}

void readUsr(void *ker, void *usr, int sel, int size) {
	asm volatile("movw %0, %%es"::"m"(sel));
	int data;
	if (size == 1) {
		asm volatile("movb %%es:(%1), %0":"=r"(data):"r"(usr));
		*(uint8_t *)ker = data;
	}
	else if (size == 2) {
		asm volatile("movw %%es:(%1), %0":"=r"(data):"r"(usr));
		*(uint16_t *)ker = data;
	}
	else if (size == 4) {
		asm volatile("movl %%es:(%1), %0":"=r"(data):"r"(usr));
		*(uint32_t *)ker = data;
	}
	else
		assert(0);
}

void writeUsrBytes(uint8_t *usr, uint8_t *ker, int sel, int cnt) {
	asm volatile("movw %0, %%es"::"m"(sel));
	uint8_t data;
	for (int i = 0; i < cnt; i++) {
		data = ker[i];
		asm volatile("movb %0, %%es:(%1)"::"r"(data),"r"(usr + i));
	}
}
/*
* find the first token in string
* set *size as the number of bytes before token
* if not found, set *size as the length of *string
*/
int stringChr (const char *string, char token, int *size) {
    int i = 0;
    if (string == NULL) {
        *size = 0;
        return -1;
    }
    while (string[i] != 0) {
        if (token == string[i]) {
            *size = i;
            return 0;
        }
        else
            i ++;
    }
    *size = i;
    return -1;
}

/*
* find the last token in string
* set *size as the number of bytes before token
* if not found, set *size as the length of *string
*/
int stringChrR (const char *string, char token, int *size) {
    int i = 0;
    if (string == NULL) {
        *size = 0;
        return -1;
    }
    while (string[i] != 0)
        i ++;
    *size = i;
    while (i > -1) {
        if (token == string[i]) {
            *size = i;
            return 0;
        }
        else
            i --;
    }
    return -1;
}

int stringLen (const char *string) {
    int i = 0;
    if (string == NULL)
        return 0;
    while (string[i] != 0)
        i ++;
    return i;
}

int stringCmp (const char *srcString, const char *destString, int size) { // compare first 'size' bytes
    int i = 0;
    if (srcString == NULL || destString == NULL)
        return -1;
    while (i != size) {
        if (srcString[i] != destString[i])
            return -1;
        else if (srcString[i] == 0)
            return 0;
        else
            i ++;
    }
	if (srcString[i]) return -1;
    return 0;
}

int stringCpy (const char *srcString, char *destString, int size) {
    int i = 0;
    if (srcString == NULL || destString == NULL)
        return -1;
    while (i != size) {
        if (srcString[i] != 0) {
            destString[i] = srcString[i];
            i++;
        }
        else
            break;
    }
    destString[i] = 0;
    return 0;
}

int setBuffer (uint8_t *buffer, int size, uint8_t value) {
    int i = 0;
    if (buffer == NULL)
        return -1;
    for (i = 0; i < size ; i ++)
        buffer[i] = value;
    return 0;
}
