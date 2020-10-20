#ifndef __FS_H__
#define __FS_H__

#include "fs/minix.h"


int fs_open(char *path, int flags);
int fs_read(int fd, uint8_t* buffer, int size, int sel);
int fs_write(int fd, uint8_t* buffer, int size, int sel);
int fs_lseek(int fd, int offset, int whence);
int fs_close(int fd);
int fs_remove(char *destFilePath);
int fs_stat(int fd, FileStat *stat);
void fs_fork(int fd);
int fs_getDirPath(int inode, char *buf);
int fs_getDirInode(char *path);

void getInodeStat(SuperBlock *superBlock,
                  Inode *inode,
                  int inodeOffset,
                  FileStat *stat);

int readSuperBlock (SuperBlock *superBlock);

int allocInode (SuperBlock *superBlock,
                Inode *fatherInode,
                int fatherInodeOffset,
                Inode *destInode,
                int *destInodeOffset,
                const char *destFilename,
                int destFiletype);

int freeInode (SuperBlock *superBlock,
               Inode *fatherInode,
               int fatherInodeOffset,
               Inode *destInode,
               int destInodeOffset,
               const char *destFilename);

int readInode (SuperBlock *superBlock,
               Inode *destInode,
               int *inodeOffset,
               const char *destFilePath);

int allocBlock (SuperBlock *superBlock,
                Inode *inode,
                int inodeOffset);

int readBlock (SuperBlock *superBlock,
               Inode *inode,
               int blockIndex,
               uint8_t *buffer);

int writeBlock (SuperBlock *superBlock,
                Inode *inode,
                int blockIndex,
                uint8_t *buffer);

int freeBlock (SuperBlock *superBlock,
               Inode *inode);

int initDir (SuperBlock *superBlock,
			 Inode *fatherInode,
			 int fatherInodeOffset,
			 Inode *destInode,
			 int destInodeOffset);

int getDirFullPath(SuperBlock *SuperBlock,
                   int inodeId,
                   char *buf);

int getDirEntry (SuperBlock *superBlock,
                 Inode *inode,
                 int dirIndex,
                 DirEntry *destDirEntry);

int getDirEntryByName (SuperBlock *superBlock,
                       Inode *inode,
                       char *name);

void initFS (void);

void initFile (void);

#endif /* __FS_H__ */
