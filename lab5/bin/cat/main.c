#include "lib.h"
#include "types.h"


void printFile(int fd, char flag);
void getInput(char flag);

int main(int argc, char *argv[]) {
	int fd;
    int non_arg = 0;
    
    char flag = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-b") == 0) {
            flag = 'b';
            break;
        }
        else if (strcmp(argv[i], "-n") == 0) {
            flag = 'n';
            break;
        }
        else
            non_arg++;
    }
    if (non_arg == 0) {
        getInput(flag);
    }
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "-n") == 0) 
            continue;
        fd = open(argv[i], O_READ);
        if (fd < 0) {
            printf("%c%s: %s: %s\n", i>1?'\n':'\0', argv[0], argv[i], errs[-1*fd]);
        }
        else {
            printFile(fd, flag);
            close(fd);
        }
    }
    exit();
}

void printFile(int fd, char flag) {
    int ret;
    int buffersize = 256;
    uint8_t buffer[256];
    int line = 0;
    bool lineStart = true;
    
    do {
        ret = read(fd, buffer, buffersize);
        for (int i = 0; i < ret; i++) {
            if (lineStart) {
                if ((flag == 'b' && buffer[i] != '\n') || flag == 'n') {
                    lineStart = false;
                    line++;
                    printf("%4d ", line);
                }
            }
            printf("%c", buffer[i]);
            if (buffer[i] == '\n') {
                lineStart = true;
            }
        }
    } while (ret == buffersize); 
}

void getInput(char flag) {
    char line[512];
    int line_num = 0;
    while (1) {
        getline(line, 256);
        if ((flag == 'b' && strlen(line) != 0) || flag == 'n') {
            line_num++;
            printf("%4d %s\n", line_num, line);
        }
        else
            printf("%s\n", line);
    }
}