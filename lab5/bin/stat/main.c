#include "lib.h"
#include "types.h"


void statfile(char *cmd, char *path) {
	FileStat fstat;
	int ret = stat(path, &fstat);
	if (ret < 0)
		printf("%s: cannot stat '%s': %s\n", cmd, path, errs[-1 * ret]);
	else {
		printf("File: %-20s\n", fstat.name);
		printf("Size: %-10dBlocks: %-7d\t%s\n", fstat.size, fstat.blocks, typestr[fstat.type]);
		printf("Inode: %-9dLinks: %-8d\n", fstat.inode, fstat.links);
	}
}

int main(int argc, char *argv[]) {
	if (argc == 1) {
        printf("%s: missing operand\n", argv[0]);
    }
    for (int i = 1; i < argc; i++) {
        statfile(argv[0], argv[i]);
    }
    exit();
}