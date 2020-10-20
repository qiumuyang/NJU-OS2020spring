#include "lib.h"
#include "types.h"


int main() {
    int fd = open("alpha.txt", O_READ | O_WRITE | O_CREATE);
    for (int i = 0; i < 26; i ++) {
        char tmp = (char)(i % 26 + 'A');
        write(fd, (uint8_t*)&tmp, 1);
    }
    close(fd);
    exit();
    return 0;
}
