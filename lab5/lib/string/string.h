#ifndef __STRING_H__
#define __STRING_H__

int strlen(const char *str);
int strcmp(const char* dst, const char *src);
int strncmp(const char* dst, const char *src, int n);
char *strchr(const char *str, int c);
char *strrchr(const char *str, int c);
char *strcpy(char *dst, const char* src);
char *strncpy(char *dst, const char* src, uint32_t n);
char *strcat(char *dst, const char* src);
char* strtok(char* str, const char* delim);


#endif