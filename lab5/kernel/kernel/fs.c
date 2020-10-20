#include "x86.h"
#include "device.h"
#include "fs.h"

extern SuperBlock sBlock;
extern File file[MAX_FILE_NUM];

int rootDirInode;

static inline int inodetoOffset(int inode) {
    return sBlock.inodeTable * SECTOR_SIZE + (inode - 1) * sizeof(Inode);
}
static inline int offsettoInode(int offset) {
    return (offset - sBlock.inodeTable * SECTOR_SIZE) / sizeof(Inode) + 1;
}

void initFS () {
	readSuperBlock(&sBlock);
    Inode root;
    int rootOffset;
    readInode(&sBlock, &root, &rootOffset, "/");
    rootDirInode = offsettoInode(rootOffset);
}

void initFile() {
    int i;
	for (i = 0; i < MAX_FILE_NUM; i++) {
		file[i].state = 0; // 0: not in use; 1: in use;
		file[i].inodeOffset = 0; // >=0: no process blocked; -1: 1 process blocked; -2: 2 process blocked;...
		file[i].offset = 0;
		file[i].flags = 0;
        file[i].f_count = 0;
	}
    file[STD_OUT].state = 1;
    file[STD_IN].state = 1;
    file[STD_ERR].state = 1;
    file[SH_MEM].state = 1;
}

int isNameValid(char *path, Inode *fatherInode) {
    char invalid[] = "/\\:*?";
    for (int i = 0; i < strlen(invalid); i++) {
        char *find = strchr(path, invalid[i]);
        if (find != NULL)
            return EBADN;
    }
    DirEntry dirent;
    int dirIndex = 0;
    int ret = getDirEntry(&sBlock, fatherInode, dirIndex, &dirent);
    while (ret != -1) {
        if (strcmp(path, dirent.name) == 0)
            return EEXIST;
        dirIndex++;
        ret = getDirEntry(&sBlock, fatherInode, dirIndex, &dirent);
    }
    return 1;
}

int isDirEmpty(SuperBlock *superBlock, Inode *inode) {
	int i = 0;
    int j = 0;
    int ret = 0;
    DirEntry *dirEntry = NULL;
    uint8_t buffer[superBlock->blockSize];

	for (i = 0; i < inode->blockCount; i++) {
        ret = readBlock(superBlock, inode, i, buffer);
        if (ret == -1)
            return -1;
        dirEntry = (DirEntry *)buffer;
        for (j = 0; j < superBlock->blockSize / sizeof(DirEntry); j++) {
        	if (stringCmp(dirEntry[j].name, ".", stringLen(dirEntry[j].name)) == 0
        	 || stringCmp(dirEntry[j].name, "..", stringLen(dirEntry[j].name)) == 0) {
        		continue;
        	}
            if (dirEntry[j].inode != 0)
                return 0;
        }
    }
    return 1;
}

int fs_open(char *path, int flags) {
    if (path == NULL)
        return EINVAL;
    int i;
    int ret;
    Inode inode;
    Inode fatherInode;
    int fatherInodeOffset;
    int inodeOffset;
    for (i = 4; i < MAX_FILE_NUM; i++) {
		if (file[i].state == 0)
            break;
	}
    if (i == MAX_FILE_NUM)
        return EMFILE;

    int len = strlen(path);
    if (len > 1 && path[len - 1] == '/')
        path[len - 1] = '\0';

    char *childPath = strrchr(path, '/');
    ret = readInode(&sBlock, &inode, &inodeOffset, path);
    if ((flags & O_WRITE) && (flags & O_DIRECTORY)) // dir cannot be written
        return EISDIR;

    if (ret == -1) {
        if (flags & O_CREATE) {
            if (childPath == NULL)
                return EINVAL;
            if (strcmp(path, "/") == 0)
                return EEXIST;
            if (childPath == path)
                ret = readInode(&sBlock, &fatherInode, &fatherInodeOffset, "/");
            else {
                *childPath = '\0';
                ret = readInode(&sBlock, &fatherInode, &fatherInodeOffset, path);
            }
            if (ret == -1) {
                putString("UNDEF 1\n");
                return EUNDEF;
            }
            ret = isNameValid(childPath + 1, &fatherInode);
            if (ret < 0)
                return ret;
            if (flags & O_DIRECTORY) {
                ret = allocInode(&sBlock, &fatherInode, fatherInodeOffset, &inode, &inodeOffset, childPath + 1, DIRECTORY_TYPE);
                if (ret == -1) {
                    putString("UNDEF 2\n");
                    return EUNDEF;
                }
                ret = initDir(&sBlock, &fatherInode, fatherInodeOffset, &inode, inodeOffset);
                if (ret == -1) {
                    putString("UNDEF 3\n");
                    return EUNDEF;
                }
            }
            else {
                ret = allocInode(&sBlock, &fatherInode, fatherInodeOffset, &inode, &inodeOffset, childPath + 1, REGULAR_TYPE);
                if (ret == -1) {
                    putString("UNDEF 4\n");
                    return EUNDEF;
                }
            }
        }
        else
            return ENOENT;
    }
    else {
        if ((flags & O_DIRECTORY) && inode.type != DIRECTORY_TYPE)
            return ENOTDIR;
        else if (!(flags & O_DIRECTORY) && inode.type == DIRECTORY_TYPE)
            return EISDIR;
        if (inode.type == REGULAR_TYPE && (flags & O_WRITE) && (flags & O_TRUNC)) {
            freeBlock(&sBlock, &inode);
            inode.blockCount = 0;
            inode.size = 0;
        }
    }
    
    strcpy(file[i].name, childPath + 1);
    if (inode.type == DIRECTORY_TYPE) {
        strcat(file[i].name, "/");
    }

    diskWrite((void *)&inode, sizeof(Inode), 1, inodeOffset);
    file[i].state = 1;
    file[i].inodeOffset = inodeOffset;
    file[i].offset = 0;
	file[i].flags = flags;
    file[i].f_count = 1;
    return i;
}

int fs_read(int fd, uint8_t* buffer, int size, int sel) {
    if (fd < 0 || fd >= MAX_FILE_NUM || file[fd].state == 0)
        return EBADF;
    if (size < 0)
        return EINVAL;
    Inode inode;
    int inodeOffset = file[fd].inodeOffset;
    int offset = file[fd].offset;
    int flags = file[fd].flags;
    if (!(flags & O_READ))
        return EACCES;
    diskRead((void *)&inode, sizeof(Inode), 1, inodeOffset);

    if (inode.type == REGULAR_TYPE) {
        int blockSize = sBlock.blockSize;
        int availByte = inode.size - offset;
        int i = 0;
        int blockIndex = -1;
        uint8_t data;
        uint8_t block[blockSize];
        asm volatile("movw %0, %%es"::"m"(sel));
        for (i = 0; i < availByte; i++) {
            if (i == size)
                break;
            if (offset / blockSize != blockIndex) {
                blockIndex = offset / blockSize;
                int ret = readBlock(&sBlock, &inode, blockIndex, block);
                if (ret == -1)
                    break;
            }
            data = block[offset % blockSize];
            asm volatile("movb %0, %%es:(%1)"::"r"(data),"r"(buffer + i));
            offset++;
        }
        file[fd].offset = offset;
        return i;
    }
    else if (inode.type == DIRECTORY_TYPE) {
        // TODO write filestat to buffer
        DirEntry dirent;
        int dirIndex = offset;
        int ret = getDirEntry(&sBlock, &inode, dirIndex, &dirent);
        if (ret == -1)
            return -1;
        dirIndex++;
        file[fd].offset = dirIndex;

        FileStat stat;
        strcpy(stat.name, dirent.name);
        inodeOffset = sBlock.inodeTable * SECTOR_SIZE + (dirent.inode - 1) * sizeof(Inode);
        diskRead((void *)&inode, sizeof(Inode), 1, inodeOffset);
        getInodeStat(&sBlock, &inode, inodeOffset, &stat);
        
        uint8_t *p = (uint8_t *)&stat;
        uint8_t data;
	    asm volatile("movw %0, %%es"::"m"(sel));
	    for (int i = 0; i < sizeof(FileStat); i++) {
		    data = p[i];
		    asm volatile("movb %0, %%es:(%1)"::"r"(data),"r"(buffer + i));
	    }
        return 1;
    }
    return EBADT;
}

int fs_write(int fd, uint8_t* buffer, int size, int sel) {
    if (fd < 0 || fd >= MAX_FILE_NUM || file[fd].state == 0)
        return EBADF;
    if (size < 0)
        return EINVAL;
    Inode inode;
    int inodeOffset = file[fd].inodeOffset;
    int offset = file[fd].offset;
    int flags = file[fd].flags;
    if (!(flags & O_WRITE))
        return EACCES;
    diskRead((void *)&inode, sizeof(Inode), 1, inodeOffset);

    if (inode.type == REGULAR_TYPE) {
        int blockSize = sBlock.blockSize;
        int blockIndex = -1;
        uint8_t data;
        uint8_t block[blockSize];
        int i = 0;
        int ret = 0;
        
        int maxsize = size + offset;
        int bound = (POINTER_NUM + blockSize / 4) * blockSize;
        if (maxsize >= bound)
            return EFBIG;

        asm volatile("movw %0, %%es"::"m"(sel));
        for (i = 0; i < size; i++) {
            if (offset / blockSize != blockIndex) {
                blockIndex = offset / blockSize;
                while (inode.blockCount < blockIndex + 1) { // not enough blocks
                    ret = allocBlock(&sBlock, &inode, inodeOffset);
                    if (ret == -1)
                        return ENOSPC;
                    memset(block, 0, blockSize);
                    writeBlock(&sBlock, &inode, inode.blockCount - 1, block);
                }
                readBlock(&sBlock, &inode, blockIndex, block);
            }
            asm volatile("movb %%es:(%1), %0":"=r"(data):"r"(buffer + i));
            block[offset % blockSize] = data;
            offset++;
            if (offset % blockSize == 0)
                writeBlock(&sBlock, &inode, blockIndex, block);
        }
        writeBlock(&sBlock, &inode, blockIndex, block);
        file[fd].offset = offset;
        inode.size = offset;
        inode.blockCount = blockIndex + 1;
        diskWrite((void *)&inode, sizeof(Inode), 1, inodeOffset);
        return size;
    }
    return EBADT;
}

int fs_lseek(int fd, int offset, int whence) {
    if (fd < 4 || fd >= MAX_FILE_NUM)
        return EBADF;
    if (file[fd].state == 0)
        return EBADF;
    Inode inode;
    int inodeOffset = file[fd].inodeOffset;
    diskRead((void *)&inode, sizeof(Inode), 1, inodeOffset);

    int _off;
    switch (whence) {
        case SEEK_SET:
            _off = offset;
            break;
        case SEEK_CUR:
            _off = offset + file[fd].offset;
            break;
        case SEEK_END:
            _off = offset + inode.size;
            break;
        default:
            return EINVAL;
    }
    if (_off < 0)
        return EINVAL;
    file[fd].offset = _off;
    return _off;
}

int fs_close(int fd) {
    if (fd < 4 || fd >= MAX_FILE_NUM)
        return EBADF;
    if (file[fd].state == 0)
        return EBADF;
    
    if (file[fd].f_count > 0) {
        file[fd].f_count--;
    }
    else
        return EINVAL;
    if (file[fd].f_count == 0) {
        file[fd].state = 0;
        file[fd].inodeOffset = 0;
        file[fd].offset = 0;
        file[fd].flags = 0;
    }
    return 0;
}

int fs_remove(char *destFilePath) {
    if (destFilePath == NULL)
        return -1;
    int ret;
    Inode destInode;
    Inode fatherInode;
    int fatherInodeOffset;
    int destInodeOffset;
    if (destFilePath[0] != '/')
        return EINVAL;

    int len = strlen(destFilePath);
    if (len > 1 && destFilePath[len - 1] == '/')
        destFilePath[len - 1] = '\0';
    else if (len == 1)
        return EBADF;

    char *path = strrchr(destFilePath, '/');
    if (path == NULL)
        return EINVAL;
    if (path == destFilePath)   // root dir
        ret = readInode(&sBlock, &fatherInode, &fatherInodeOffset, "/");
    else {
        *path = '\0';
        ret = readInode(&sBlock, &fatherInode, &fatherInodeOffset, destFilePath);
    }
    if (ret == -1)
        return ENOENT;
    
    *path = '/';
    ret = readInode(&sBlock, &destInode, &destInodeOffset, destFilePath);
    if (ret == -1)
        return ENOENT;

    // check whether file in use
    int i = 0;
    for (i = 0; i < MAX_FILE_NUM; i++) {
        if (file[i].state == 1 && file[i].inodeOffset == destInodeOffset)
            break;
    }
    if (i != MAX_FILE_NUM)
        return EACCES;

    // check file type & dir empty
    if (destInode.type == DIRECTORY_TYPE) {
        if (!isDirEmpty(&sBlock, &destInode)) {
            return ENOTEMPTY;
        }
    }
    else if (destInode.type != REGULAR_TYPE)
        return EBADT;
    ret = freeInode(&sBlock, &fatherInode, fatherInodeOffset, &destInode, destInodeOffset, path + 1);
    if (ret == -1) {
        putString("UNDEF 5\n");
        return EUNDEF;
    }
    return 0;
}

int fs_stat(int fd, FileStat *stat) {
    if (fd < 0 || fd >= MAX_FILE_NUM) {
        return EBADF;
    }
    if (file[fd].state == 0) {
        return EBADF;
    }
    Inode inode;
    int inodeOffset = file[fd].inodeOffset;
    diskRead((void *)&inode, sizeof(Inode), 1, inodeOffset);
    
    getInodeStat(&sBlock, &inode, inodeOffset, stat);
    strcpy(stat->name, file[fd].name);
    return 0;
}

void fs_fork(int fd) {
    file[fd].f_count++;
}

int fs_getDirPath(int inode, char *buf) {
    return getDirFullPath(&sBlock, inode, buf);
}

int fs_getDirInode(char *path) {
    if (path == NULL)
        return EINVAL;
    int ret;
    Inode inode;
    int inodeOffset;

    int len = strlen(path);
    if (len > 1 && path[len - 1] == '/')
        path[len - 1] = '\0';

    ret = readInode(&sBlock, &inode, &inodeOffset, path);
    if (ret == -1)
        return ENOENT;
    if (inode.type != DIRECTORY_TYPE)
        return ENOTDIR;
    return offsettoInode(inodeOffset);
}

void getInodeStat(SuperBlock *superBlock, Inode *inode, int inodeOffset, FileStat *stat) {
    stat->type = inode->type;
    stat->blocks = inode->blockCount;
    stat->links = inode->linkCount;
    stat->size = inode->size;
    stat->inode = (inodeOffset - superBlock->inodeTable * SECTOR_SIZE) / sizeof(Inode) + 1;
}

int getDirFullPath(SuperBlock *SuperBlock, int inodeId, char *buf) {
    int ret = 0;
    buf[0] = 0;
    if (inodeId == rootDirInode) {
        strcpy(buf, "/");
        return 0;
    }

    Inode inode;
    diskRead((void *)&inode, sizeof(Inode), 1, inodetoOffset(inodeId));
    int tmp[20];
    int *p = tmp + 20;
    p--;
    *p = inodeId;

    ret = getDirEntryByName(&sBlock, &inode, "..");
    while (ret != rootDirInode) {
        if (ret == -1)
            return -1;
        p--;
        *p = ret;
        diskRead((void *)&inode, sizeof(Inode), 1, inodetoOffset(ret));
        ret = getDirEntryByName(&sBlock, &inode, "..");
    }
    p--;
    *p = rootDirInode;
    strcat(buf, "/");
    p++;

    while (p < tmp + 20) {
        diskRead((void *)&inode, sizeof(Inode), 1, inodetoOffset(*(p-1)));
        int dirIndex = 0;
        DirEntry dirent;
        while (1) {
            ret = getDirEntry(&sBlock, &inode, dirIndex, &dirent);
            if (ret == -1)
                return -1;
            if (dirent.inode == *p) {
                strcat(buf, dirent.name);
                strcat(buf, "/");
                break;
            }
            dirIndex++;
        }
        p++;
    }
    return 0;
}

int readSuperBlock(SuperBlock *superBlock) {
    diskRead((void*)superBlock, sizeof(SuperBlock), 1, 0);
    return 0;
}

/*
 *  Find a Block unused, this function will update SuperBlock and BlockBitmap and write back.
 *  Input: superblock.
 *  Output: blockOffset (sector as unit)
 *  Return -1 when failed.
 */
int getAvailBlock (SuperBlock *superBlock, int *blockOffset) {
    int j = 0;
    int k = 0;
    int blockBitmapOffset = 0;
    BlockBitmap blockBitmap;

    if (superBlock->availBlockNum == 0)
        return -1;
    superBlock->availBlockNum--;

    blockBitmapOffset = superBlock->blockBitmap;
    diskRead((void *)&blockBitmap, sizeof(BlockBitmap), 1, blockBitmapOffset * SECTOR_SIZE);
    for (j = 0; j < superBlock->blockNum / 8; j++) {
        if (blockBitmap.byte[j] != 0xff) {
            break;
        }
    }
    for (k = 0; k < 8; k++) {
        if ((blockBitmap.byte[j] >> (7 - k)) % 2 == 0) {
            break;
        }
    }
    blockBitmap.byte[j] = blockBitmap.byte[j] | (1 << (7 - k));
    
    *blockOffset = superBlock->blocks + ((j * 8 + k) * superBlock->blockSize / SECTOR_SIZE);

    diskWrite((void *)superBlock, sizeof(SuperBlock), 1, 0);
    diskWrite((void *)&blockBitmap, sizeof(BlockBitmap), 1, blockBitmapOffset * SECTOR_SIZE);
    return 0;
}

/*
 *  Release all blocks alloced to an Inode,
 * 			this function will update SuperBlock and BlockBitmap and write back, but not Inode itself.
 *  Input: superblock, inode.
 *  No Error Checking.
 */
int freeBlock (SuperBlock *superBlock, Inode *inode) {
	int j = 0;
	int k = 0;
	int divider0 = superBlock->blockSize / 4;
    int bound0 = POINTER_NUM;
    int bound1 = bound0 + divider0;
    
    uint32_t singlyPointerBuffer[divider0];
    
    int blockBitmapOffset = 0;
    BlockBitmap blockBitmap;
    
    int blockOffset = 0;
    int blockId = 0;
    
    blockBitmapOffset = superBlock->blockBitmap;
    diskRead((void *)&blockBitmap, sizeof(BlockBitmap), 1, blockBitmapOffset * SECTOR_SIZE);
    
    if (inode->blockCount >= bound0) {
        diskRead((void *)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, inode->singlyPointer * SECTOR_SIZE);
    }
    
    // release block: 1. modify blockBitmap 2. increase availBlockNum
	for (int i = 0; i < inode->blockCount; i++) {
		if (i < bound0) {
        	blockOffset = inode->pointer[i];
    	}
    	else if (i < bound1) {
    		blockOffset = singlyPointerBuffer[i - bound0];
    	}
    	blockId = (blockOffset - superBlock->blocks) / (superBlock->blockSize / SECTOR_SIZE);
    	j = blockId / 8; k = blockId % 8;
    	blockBitmap.byte[j] &= ~(1 << (7 - k));
    	
    	superBlock->availBlockNum++;
	}

    diskWrite((void *)superBlock, sizeof(SuperBlock), 1, 0);
    diskWrite((void *)&blockBitmap, sizeof(BlockBitmap), 1, blockBitmapOffset * SECTOR_SIZE);
    return 0;
}

/*
 *  Alloc a new Block for inode, inode.blockCount++ and write back inode.
 *  Input: file, superBlock, inode, inodeOffset(byte as unit), blockOffset (sector as unit).
 *  Output: inode
 *  Return -1 when failed.
 */
int allocLastBlock (SuperBlock *superBlock, Inode *inode, int inodeOffset, int blockOffset) {
    int divider0 = superBlock->blockSize / 4;
    int bound0 = POINTER_NUM;
    int bound1 = bound0 + divider0;

    uint32_t singlyPointerBuffer[divider0];
    int singlyPointerBufferOffset = 0;

    if (inode->blockCount < bound0) {
        inode->pointer[inode->blockCount] = blockOffset;
    }
    else if (inode->blockCount == bound0) {
        getAvailBlock(superBlock, &singlyPointerBufferOffset);
        singlyPointerBuffer[0] = blockOffset;
        inode->singlyPointer = singlyPointerBufferOffset;
        diskWrite((void *)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, singlyPointerBufferOffset * SECTOR_SIZE);
    }
    else if (inode->blockCount < bound1) {
        diskRead((void *)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, inode->singlyPointer * SECTOR_SIZE);
        singlyPointerBuffer[inode->blockCount - bound0] = blockOffset;
        diskWrite((void *)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, inode->singlyPointer * SECTOR_SIZE);
    }
    else
        return -1;
    
    inode->blockCount++;
    diskWrite((void *)inode, sizeof(Inode), 1, inodeOffset);
    return 0;
}


 int calNeededPointerBlocks (SuperBlock *superBlock, int blockCount) {
    int divider0 = superBlock->blockSize / 4;
    int bound0 = POINTER_NUM;
    int bound1 = bound0 + divider0;

    if (blockCount == bound0)
        return 1;
    else if (blockCount >= bound1)
        return -1;
    else
        return 0;
}
/*
 *  Alloc a new block for inode by call getAvailBlock and allocLastBlock.
 *  Input: superBlock, inode, inodeOffset (byte as unit).
 *  Output: inode.
 *  Return -1 when failed.
 */
int allocBlock (SuperBlock *superBlock, Inode *inode, int inodeOffset) {
    int ret = 0;
    int blockOffset = 0;

    ret = calNeededPointerBlocks(superBlock, inode->blockCount);
    if (ret == -1)
        return -1;
    if (superBlock->availBlockNum < ret + 1)
        return -1;
    
    getAvailBlock(superBlock, &blockOffset);
    allocLastBlock(superBlock, inode, inodeOffset, blockOffset);
    return 0;
}

int readBlock(SuperBlock *superBlock, Inode *inode, int blockIndex, uint8_t *buffer) {
    int divider0 = superBlock->blockSize / 4;
    int bound0 = POINTER_NUM;
    int bound1 = bound0 + divider0;

    uint32_t singlyPointerBuffer[divider0];
    
    if (blockIndex < bound0) {
        diskRead((void*)buffer, sizeof(uint8_t), superBlock->blockSize, inode->pointer[blockIndex] * SECTOR_SIZE);
        return 0;
    }
    else if (blockIndex < bound1) {
        diskRead((void*)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, inode->singlyPointer * SECTOR_SIZE);
        diskRead((void*)buffer, sizeof(uint8_t), superBlock->blockSize, singlyPointerBuffer[blockIndex - bound0] * SECTOR_SIZE);
        return 0;
    }
    else {
        return -1;
    }
}

int writeBlock (SuperBlock *superBlock, Inode *inode, int blockIndex, uint8_t *buffer) {
    int divider0 = superBlock->blockSize / 4;
    int bound0 = POINTER_NUM;
    int bound1 = bound0 + divider0;

    uint32_t singlyPointerBuffer[divider0];

    if (blockIndex < bound0) {
        diskWrite((void *)buffer, sizeof(uint8_t), superBlock->blockSize, inode->pointer[blockIndex] * SECTOR_SIZE);
        return 0;
    }
    else if (blockIndex < bound1) {
        diskRead((void *)singlyPointerBuffer, sizeof(uint8_t), superBlock->blockSize, inode->singlyPointer * SECTOR_SIZE);
        diskWrite((void *)buffer, sizeof(uint8_t), superBlock->blockSize, singlyPointerBuffer[blockIndex - bound0] * SECTOR_SIZE);
        return 0;
    }
    else
        return -1;
}

int getAvailInode(SuperBlock *superBlock, int *inodeOffset) {
    int j = 0;
    int k = 0;
    int inodeBitmapOffset = 0;
    int inodeTableOffset = 0;
    InodeBitmap inodeBitmap;

    if (superBlock->availInodeNum == 0)
        return -1;
    superBlock->availInodeNum--;

    inodeBitmapOffset = superBlock->inodeBitmap;
    inodeTableOffset = superBlock->inodeTable;
    diskRead((void *)&inodeBitmap, sizeof(InodeBitmap), 1, inodeBitmapOffset * SECTOR_SIZE);
    for (j = 0; j < superBlock->availInodeNum / 8; j++) {
        if (inodeBitmap.byte[j] != 0xff) {
            break;
        }
    }
    for (k = 0; k < 8; k++) {
        if ((inodeBitmap.byte[j] >> (7-k)) % 2 == 0) {
            break;
        }
    }
    inodeBitmap.byte[j] = inodeBitmap.byte[j] | (1 << (7 - k));

    *inodeOffset = inodeTableOffset * SECTOR_SIZE + (j * 8 + k) * sizeof(Inode);

    diskWrite((void *)superBlock, sizeof(SuperBlock), 1, 0);
    diskWrite((void *)&inodeBitmap, sizeof(InodeBitmap), 1, inodeBitmapOffset * SECTOR_SIZE);

    return 0;
}

int allocInode(SuperBlock *superBlock, Inode *fatherInode, int fatherInodeOffset,
        Inode *destInode, int *destInodeOffset, const char *destFilename, int destFiletype) {
    int i = 0;
    int j = 0;
    int ret = 0;
    DirEntry *dirEntry = NULL;
    uint8_t buffer[superBlock->blockSize];
    int length = stringLen(destFilename);

    if (destFilename == NULL || destFilename[0] == 0)
        return -1;

    if (superBlock->availInodeNum == 0)
        return -1;
    
    for (i = 0; i < fatherInode->blockCount; i++) {
        ret = readBlock(superBlock, fatherInode, i, buffer);
        if (ret == -1)
            return -1;
        dirEntry = (DirEntry *)buffer;
        for (j = 0; j < superBlock->blockSize / sizeof(DirEntry); j++) {
            if (dirEntry[j].inode == 0) // a valid empty dirEntry
                break;
            else if (stringCmp(dirEntry[j].name, destFilename, length) == 0)
                return -1; // file with filename = destFilename exist
        }
        if (j < superBlock->blockSize / sizeof(DirEntry))
            break;
    }
    if (i == fatherInode->blockCount) {
        ret = allocBlock(superBlock, fatherInode, fatherInodeOffset);
        if (ret == -1)
            return -1;
        fatherInode->size = fatherInode->blockCount * superBlock->blockSize;
        setBuffer(buffer, superBlock->blockSize, 0);
        dirEntry = (DirEntry *)buffer;
        j = 0;
    }
    // dirEntry[j] is the valid empty dirEntry, it is in the i-th block of fatherInode.
    ret = getAvailInode(superBlock, destInodeOffset);
    if (ret == -1)
        return -1;

    stringCpy(destFilename, dirEntry[j].name, NAME_LENGTH);
    dirEntry[j].inode = (*destInodeOffset - superBlock->inodeTable * SECTOR_SIZE) / sizeof(Inode) + 1;
    ret = writeBlock(superBlock, fatherInode, i, buffer);
    if (ret == -1)
        return -1;
    
    diskWrite((void *)fatherInode, sizeof(Inode), 1, fatherInodeOffset);

    destInode->type = destFiletype;
    destInode->linkCount = 1;
    destInode->blockCount = 0;
    destInode->size = 0;

    diskWrite((void *)destInode, sizeof(Inode), 1, *destInodeOffset);

    return 0;
}

int readInode(SuperBlock *superBlock, Inode *destInode, int *inodeOffset, const char *destFilePath) {
    int i = 0;
    int j = 0;
    int ret = 0;
    int cond = 0;
    *inodeOffset = 0;
    uint8_t buffer[superBlock->blockSize];
    DirEntry *dirEntry = NULL;
    int count = 0;
    int size = 0;
    int blockCount = 0;

    if (destFilePath == NULL || destFilePath[count] == 0) {
        return -1;
    }
    ret = stringChr(destFilePath, '/', &size);
    if (ret == -1 || size != 0) {
        return -1;
    }
    count += 1;
    *inodeOffset = superBlock->inodeTable * SECTOR_SIZE;
    diskRead((void *)destInode, sizeof(Inode), 1, *inodeOffset);

    while (destFilePath[count] != 0) {
        ret = stringChr(destFilePath + count, '/', &size);
        if(ret == 0 && size == 0) {
            return -1;
        }
        if (ret == -1) {
            cond = 1;
        }
        else if (destInode->type == REGULAR_TYPE) {
            return -1;
        }
        blockCount = destInode->blockCount;
        for (i = 0; i < blockCount; i ++) {
            ret = readBlock(superBlock, destInode, i, buffer);
            if (ret == -1) {
                return -1;
            }
            dirEntry = (DirEntry *)buffer;
            for (j = 0; j < superBlock->blockSize / sizeof(DirEntry); j ++) {
                if (dirEntry[j].inode == 0) {
                    continue;
                }
                else if (stringCmp(dirEntry[j].name, destFilePath + count, size) == 0) {
                    *inodeOffset = superBlock->inodeTable * SECTOR_SIZE + (dirEntry[j].inode - 1) * sizeof(Inode);
                    diskRead((void *)destInode, sizeof(Inode), 1, *inodeOffset);
                    break;
                }
            }
            if (j < superBlock->blockSize / sizeof(DirEntry)) {
                break;
            }
        }
        if (i < blockCount) {
            if (cond == 0) {
                count += (size + 1);
            }
            else {
                return 0;
            }
        }
        else {
            return -1;
        }
    }
    return 0;
}

int freeInode(SuperBlock *superBlock, Inode *fatherInode, int fatherInodeOffset,
        Inode *destInode, int destInodeOffset, const char *destFilename) {
    int i = 0;
    int j = 0;
    int ret = 0;
    DirEntry *dirEntry = NULL;
    uint8_t buffer[superBlock->blockSize];
    int length = stringLen(destFilename);
    int inodeId = 0;
    int inodeBitmapOffset = 0;
    int inodeTableOffset = 0;
    InodeBitmap inodeBitmap;

    inodeBitmapOffset = superBlock->inodeBitmap;
    inodeTableOffset = superBlock->inodeTable;
    diskRead((void *)&inodeBitmap, sizeof(InodeBitmap), 1, inodeBitmapOffset * SECTOR_SIZE);
	for (i = 0; i < fatherInode->blockCount; i++) {
        ret = readBlock(superBlock, fatherInode, i, buffer);
        if (ret == -1)
            return -1;
        dirEntry = (DirEntry *)buffer;
        for (j = 0; j < superBlock->blockSize / sizeof(DirEntry); j++) {
            if (stringCmp(dirEntry[j].name, destFilename, length) == 0)
                break;
        }
        if (j < superBlock->blockSize / sizeof(DirEntry))
            break;
    }
    if (i == fatherInode->blockCount)
    	return -1;
    
    // remove from fatherInode
    dirEntry[j].inode = 0;
    dirEntry[j].name[0] = '\0';
    ret = writeBlock(superBlock, fatherInode, i, buffer);
    if (ret == -1) {
        return -1;
    }
    
    // free destInode's Blocks
    freeBlock(superBlock, destInode);
    
    // reset destInode
    destInode->type = UNKNOWN_TYPE;
    destInode->linkCount = 0;
    destInode->blockCount = 0;
    destInode->size = 0;
    
    // update InodeBitmap & availInodeNum
    inodeId = (destInodeOffset - inodeTableOffset * SECTOR_SIZE) / sizeof(Inode);
    i = inodeId / 8; j = inodeId % 8;
    inodeBitmap.byte[i] &= ~(1 << (7 - j));
    superBlock->availInodeNum++;
    
    diskWrite((void *)superBlock, sizeof(SuperBlock), 1, 0);
    diskWrite((void *)&inodeBitmap, sizeof(InodeBitmap), 1, inodeBitmapOffset * SECTOR_SIZE);
    diskWrite((void *)fatherInode, sizeof(Inode), 1, fatherInodeOffset);
    diskWrite((void *)destInode, sizeof(Inode), 1, destInodeOffset);
    
    return 0;
}

int initDir(SuperBlock *superBlock, Inode *fatherInode, int fatherInodeOffset,
        Inode *destInode, int destInodeOffset) {
    int ret = 0;
    int blockOffset = 0;
    DirEntry *dirEntry = NULL;
    uint8_t buffer[superBlock->blockSize];

    ret = getAvailBlock(superBlock, &blockOffset);
    if (ret == -1)
        return -1;
    destInode->pointer[0] = blockOffset;
    destInode->blockCount = 1;
    destInode->size = superBlock->blockSize;
    setBuffer(buffer, superBlock->blockSize, 0);
    dirEntry = (DirEntry *)buffer;
    dirEntry[0].inode = (destInodeOffset - superBlock->inodeTable * SECTOR_SIZE) / sizeof(Inode) + 1;
    destInode->linkCount ++;
    dirEntry[0].name[0] = '.';
    dirEntry[0].name[1] = '\0';
    dirEntry[1].inode = (fatherInodeOffset - superBlock->inodeTable * SECTOR_SIZE) / sizeof(Inode) + 1;
    fatherInode->linkCount ++;
    dirEntry[1].name[0] = '.';
    dirEntry[1].name[1] = '.';
    dirEntry[1].name[2] = '\0';

    diskWrite((void *)buffer, sizeof(uint8_t), superBlock->blockSize, blockOffset * SECTOR_SIZE);
    diskWrite((void *)fatherInode, sizeof(Inode), 1, fatherInodeOffset);
    diskWrite((void *)destInode, sizeof(Inode), 1, destInodeOffset);
    return 0;
}

int getDirEntry (SuperBlock *superBlock, Inode *inode, int dirIndex, DirEntry *destDirEntry) {
    int i = 0;
    int j = 0;
    int ret = 0;
    int dirCount = 0;
    DirEntry *dirEntry = NULL;
    uint8_t buffer[superBlock->blockSize];

    for (i = 0; i < inode->blockCount; i++) {
        ret = readBlock(superBlock, inode, i, buffer);
        if (ret == -1)
            return -1;
        dirEntry = (DirEntry *)buffer;
        for (j = 0; j < superBlock->blockSize / sizeof(DirEntry); j ++) {
            if (dirEntry[j].inode != 0) {
                if (dirCount == dirIndex)
                    break;
                else
                    dirCount ++;
            }
        }
        if (j < superBlock->blockSize / sizeof(DirEntry))
            break;
    }
    if (i == inode->blockCount)
        return -1;
    else {
        destDirEntry->inode = dirEntry[j].inode;
        stringCpy(dirEntry[j].name, destDirEntry->name, NAME_LENGTH);
        return 0;
    }
}

/*
 * Input:   SuperBlock, Dir Inode, target Name
 * Output:  Inode Num of target, -1 if not found
 * 
 */
int getDirEntryByName (SuperBlock *superBlock, Inode *inode, char *name) {
    int i = 0;
    int j = 0;
    int ret = 0;
    DirEntry *dirEntry = NULL;
    uint8_t buffer[superBlock->blockSize];

    for (i = 0; i < inode->blockCount; i++) {
        ret = readBlock(superBlock, inode, i, buffer);
        if (ret == -1)
            return -1;
        dirEntry = (DirEntry *)buffer;
        for (j = 0; j < superBlock->blockSize / sizeof(DirEntry); j ++) {
            if (dirEntry[j].inode != 0 && strcmp(name, dirEntry[j].name) == 0)
                return dirEntry[j].inode;
        }
    }
    return -1;
}