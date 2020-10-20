#ifndef __UTILS_H__
#define __UTILS_H__

int strlen(const char *str);

int strcmp(const char* dst, const char *src);

char *strchr(const char *str, int c);

char *strrchr(const char *str, int c);

char *strcpy(char *dst, const char* src);

char *strcat(char *dst, const char* src);

char* strtok(char* str, const char* delim);

void *memcpy(void *dst, const void *src, int n);

void *memset(void *dst, uint8_t c, int n);

void readUsrString(char *ker, char *usr, int sel);

void readUsr(void *ker, void *usr, int sel, int size);

void writeUsrBytes(uint8_t *usr, uint8_t *ker, int sel, int cnt);

int stringChr(const char *string, char token, int *size);

int stringChrR (const char *string, char token, int *size);

int stringLen(const char *string);

int stringCmp(const char *srcString, const char *destString, int size);

int stringCpy (const char *srcString, char *destString, int size);

int setBuffer (uint8_t *buffer, int size, uint8_t value);

#endif
