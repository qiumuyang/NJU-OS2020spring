#include "lib.h"
#include "types.h"


int main(int argc, const char *argv[]) {
    printf("Total argc:%d\n", argc);
    for (int i = 0; i < argc; i++){
        printf("argv%d: %s\n", i+1, argv[i]);
    }
    exit();
    return 0;
}