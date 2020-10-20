#include "lib.h"
#include "types.h"


int main(int argc, const char *argv[]) {
    char *content = malloc(sizeof(char) * 256);
    if (argc > 1)
        strncpy(content, argv[1], 128);
    else
        strcpy(content, "Write something");
    content[128] = 0;

    int fd = open("write.test", O_CREATE|O_WRITE|O_TRUNC);

    if (fork() == 0) {
        sleep(128);
        strcat(content, " Child process\n");
        write(fd, (uint8_t *)content, strlen(content) + 1);
    }
    else {
        strncpy(content, "Father: ", 8);
        write(fd, (uint8_t *)content, strlen(content) + 1);
        char ch = '\n';
        write(fd, (uint8_t *)&ch, 1);
    }
    free(content);
    close(fd);
    exit();
    return 0;
}