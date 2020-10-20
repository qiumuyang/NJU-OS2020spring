#include "lib.h"
#include "types.h"

void colorful_test() {
	char tmp[15] = "\033[";
	for (int i = 0; i < 16; i++) {
		int ret = itoa(i, tmp + 2);
		strcpy(tmp + ret, "mtest");
		printf(tmp);
		printf(" \\033[%dm\n", i);
	}	
}

int main() {
	printf("[\033[1mcolorful\033[2mprintf\033[3mtest\033[12m]\n");
	colorful_test();
	exit();
}

