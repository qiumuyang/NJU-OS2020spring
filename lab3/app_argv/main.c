#include "lib.h"
#include "types.h"

int main(int argc, char * const argv[]) {
	printf("argc: %d\n", argc);
    for (int i = 0; i < argc; i++) {
    	printf("argv[%d]: %s\n", i, argv[i]);
    }
    exit();
}
