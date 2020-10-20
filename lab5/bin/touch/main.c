#include "lib.h"
#include "types.h"


int main(int argc, char *argv[]) {
	int fd;
    if (argc == 1) {
        printf("%s: missing operand\n", argv[0]);
    }
    for (int i = 1; i < argc; i++) {
        fd = open(argv[i], O_CREATE);
        if (fd < 0) {
            printf("%s: %s: %s\n", argv[0], argv[i], errs[-1*fd]);
        }
        else {
            close(fd);
        }
    }
    exit();
}