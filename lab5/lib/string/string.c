#include "lib.h"
#include "types.h"


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

int strncmp (const char* dst, const char *src, int n) {
	int i = 0;
	if (dst == NULL || src == NULL) 
		return -1;
	while (dst[i] && src[i]) {
		if (dst[i] < src[i])
			return -1;
		else if (dst[i] > src[i])
			return 1;
		i++;
		if (i == n)
			return 0;			
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

char *strncpy (char *dst, const char* src, uint32_t n) {
	if (src == NULL || dst == NULL)
		return NULL;
	int i = 0;
	while (src[i] && i < n) {
		dst[i] = src[i];
		i++;
	}
	if (!src[i]) {
		while (i < n) {
			dst[i] = '\0';
			i++;
		}
	}
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