#include "lib.h"
#include "types.h"
#define EXTEND

int data = 0;

int uEntry(void) {
	int ret = fork();
	int i = 8;
	if (ret == 0) {
		data = 2;
		while(i != 0) {
			i --;
			printf("Child Process: Pong %d, %d;\n", data, i);
			sleep(128);
		}
#ifdef EXTEND
		char str1[] = "/usr/argv"; char str2[] = "test exec with argv"; char str3[] = "12345678";
		char * const argv[] = {str1, str2, str3, 0};
		//char * const argv[] = {"/usr/argv", "this will fail", 0};
		exec("/usr/argv\0", argv);
#endif
		exit();
	}
	else if (ret != -1) {
		data = 1;
		while(i != 0) {
			i --;
			printf("Father Process: Ping %d, %d;\n", data, i);
			sleep(128);
		}
		exec("/usr/print\0", 0);
		exit();
	}
	while(1);
	return 0;
}
