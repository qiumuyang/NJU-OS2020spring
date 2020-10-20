#ifndef __IO_H__
#define __IO_H__

/* \033[xxm
 *   Dark    Light
 * 0 Black   Deep Gray
 * 1         Blue
 * 2         Green
 * 3         Azure
 * 4         Red
 * 5 Purple  Roseo
 * 6 Khaki   Yellow
 * 7 Gray    White
 */
int printf(const char *format, ...);
int scanf(const char *format, ...);
int getline(char *buf, int size);

int open(char *path, int flags);
int write(int fd, uint8_t *buffer, int size, ...);
int read(int fd, uint8_t *buffer, int size, ...);
int lseek(int fd, int offset, int whence);
int close(int fd);
int remove(char *path);

int creat(char *path);
int stat(char *path, FileStat *stat);
int dup(int fd);
int dup2(int oldfd, int newfd);
bool is_dir(char *path);
int chdir(char *path);
int getcwd(char *path, int size);

int relPath(char *rel);

const char *errs[16];
char *typestr[8];

#endif