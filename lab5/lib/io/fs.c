#include "lib.h"
#include "types.h"

const char *errs[16] = {"Success",
						"No such file or directory",
						"Bad file number",
						"File exists",
						"Not a directory",
						"Is a directory",
						"Too many open files",
						"File too large",
						"Undefined error",
						"Invalid argument",
						"Access denied",
						"Bad file name",
						"Bad file type",
						"No available space left",
						"Directory not empty",
						"Result out of range"};

char *typestr[8] = {"unknown type",
					"regular file",
					"directory",
					"character type",
					"block type",
					"fifo",
					"socket",
					"symbolic link"};

int relPath(char *rel)
{
	if (rel[0] == '/')
		return 0;
	char cwd[256];
	if (getcwd(cwd, 256) >= 0)
	{
		if (cwd[strlen(cwd) - 1] != '/')
			strcat(cwd, "/");
		strcat(cwd, rel);
		strcpy(rel, cwd);
		return 0;
	}
	return -1;
}

int write(int fd, uint8_t *buffer, int size, ...)
{
	int index = 0;
	if (fd == SH_MEM)
	{
		index = *(int *)((void *)&size + 4);
	}
	return syscall(SYS_WRITE, fd, (uint32_t)buffer, size, index, 0);
}

int read(int fd, uint8_t *buffer, int size, ...)
{
	int index = 0;
	if (fd == SH_MEM)
	{
		index = *(int *)((void *)&size + 4);
	}
	return syscall(SYS_READ, fd, (uint32_t)buffer, size, index, 0);
}

int open(char *path, int flags)
{
	char buf[256];
	strcpy(buf, path);
	relPath(buf);
	return syscall(SYS_OPEN, (uint32_t)buf, flags, 0, 0, 0);
}

int creat(char *path)
{
	char buf[256];
	strcpy(buf, path);
	relPath(buf);
	int flags = O_WRITE | O_CREATE | O_TRUNC;
	return syscall(SYS_OPEN, (uint32_t)buf, flags, 0, 0, 0);
}

int lseek(int fd, int offset, int whence)
{
	if (whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END)
		return syscall(SYS_LSEEK, fd, offset, whence, 0, 0);
	return -1;
}

int close(int fd)
{
	return syscall(SYS_CLOSE, fd, 0, 0, 0, 0);
}

int remove(char *path)
{
	char buf[256];
	strcpy(buf, path);
	relPath(buf);
	return syscall(SYS_REMOVE, (uint32_t)buf, 0, 0, 0, 0);
}

int stat(char *path, FileStat *stat)
{
	int fd;
	if (is_dir(path))
	{
		fd = open(path, O_DIRECTORY | O_READ);
	}
	else
		fd = open(path, O_READ);
	if (fd < 0)
	{
		return fd;
	}
	int ret = syscall(SYS_STAT, fd, (uint32_t)stat, 0, 0, 0);
	if (ret == 0)
		close(fd);
	return ret;
}

int dup(int fd)
{
	return syscall(SYS_DUP, fd, 0, 0, 0, 0);
}

int dup2(int oldfd, int newfd)
{
	return syscall(SYS_DUP2, oldfd, newfd, 0, 0, 0);
}

bool is_dir(char *path)
{
	char buf[256];
	strcpy(buf, path);
	relPath(buf);
	int fd = open(buf, O_DIRECTORY | O_READ);
	if (fd >= 0)
	{
		close(fd);
		return true;
	}
	return false;
}

int getcwd(char *path, int size)
{
	return syscall(SYS_GETCWD, (uint32_t)path, size, 0, 0, 0);
}

int chdir(char *path)
{
	char buf[256];
	strcpy(buf, path);
	relPath(buf);
	return syscall(SYS_CHDIR, (uint32_t)buf, 0, 0, 0, 0);
}