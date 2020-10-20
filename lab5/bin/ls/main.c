#include "lib.h"
#include "types.h"


bool executable(char *path) {
    int fd = open(path, O_READ);
    if (fd > 0) {
        int magic;
        read(fd, (uint8_t *)&magic, 4);
        close(fd);
        return magic == 0x464c457f;
    }
    return false;
}

void ls(char *path, bool flaga) {
	int fd = open(path, O_READ|O_DIRECTORY);
	if (fd < 0) {
		printf("ls: cannot ls %s: %s\n", path, errs[-1 * fd]);
		return;
	}
    char tmppath[128];
    char exepath[128];
    strcpy(tmppath, path);
	FileStat fileStat;
	int ret;
	int cnt = 0;
    if (tmppath[strlen(tmppath) -1] != '/') {
        strcat(tmppath, "/");
    }
    do {
        ret = read(fd, (uint8_t *)&fileStat, sizeof(FileStat));
        if (ret == -1) break;
        if (cnt % 5 == 0 && cnt != 0)
            printf("\n");
        if (!flaga && fileStat.name[0] == '.')
            continue;
        if (fileStat.type == DIRECTORY_TYPE)
            printf("\033[9m%-15s", fileStat.name);
        else if (fileStat.type == CHARACTER_TYPE)
            printf("\033[14m%-15s", fileStat.name);
        else {
            strcpy(exepath, tmppath);
            strcat(exepath, fileStat.name);
            if (executable(exepath))
                printf("\033[10m%-15s", fileStat.name);
            else
                printf("%-15s", fileStat.name);
        }
        cnt++;
    } while (ret != -1);
	printf("\n");
	close(fd);
}

void lsl(char *path, bool flaga) {
    int fd = open(path, O_READ|O_DIRECTORY);
	if (fd < 0) {
		printf("ls: cannot ls %s: %s\n", path, errs[-1 * fd]);
		return;
	}
    char tmppath[128];
    char exepath[128];
    strcpy(tmppath, path);
    if (tmppath[strlen(tmppath) -1] != '/') {
        strcat(tmppath, "/");
    }
	FileStat fileStat;
	int ret;
    do {
        ret = read(fd, (uint8_t *)&fileStat, sizeof(FileStat));
        if (ret == -1) break;
        if (!flaga && fileStat.name[0] == '.')
            continue;
        printf("%-4d%-4d%-15d", fileStat.inode, fileStat.links, fileStat.size);
        if (fileStat.type == DIRECTORY_TYPE)
            printf("\033[9m%-15s\n", fileStat.name);
        else if (fileStat.type == CHARACTER_TYPE)
            printf("\033[14m%-15s\n", fileStat.name);
        else {
            strcpy(exepath, tmppath);
            strcat(exepath, fileStat.name);
            if (executable(exepath))
                printf("\033[10m%-15s\n", fileStat.name);
            else
                printf("%-15s\n", fileStat.name);
        }
    } while (ret != -1);
	close(fd);
}


int main(int argc, char *argv[]) {
    bool flagl = 0;
    bool flaga = 0;
    int filecnt = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0)
            flagl = 1;
        else if (strcmp(argv[i], "-a") == 0)
            flaga = 1;
        else
            filecnt++;
    }
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "-a") == 0)
            continue;
        else {
            if (filecnt > 1) 
                printf("%s:\n", argv[i]);
            if (flagl)
                lsl(argv[i], flaga);
            else
                ls(argv[i], flaga);
            if (i != argc - 1)
                printf("\n");
        }
    }
    if (filecnt == 0) {
        if (flagl)
            lsl(".", flaga);
        else
            ls(".", flaga);
    }
    exit();
}