#include "lib.h"
#include "types.h"

int child = -1;
int parent;
void handler() {
	//printf("%d In SigInt handler parent=%d\n", getpid(), parent);
	if (parent == getpid()) {
		//printf("Pid 1: kill Child (pid: %d) SIG_TERM\n", child);
		if (child != -1) kill(child, SIG_TERM);
	}
}

void sigchild() {
	if (parent == getpid()) {
		printf("\033[1mPid 1: Got SIG_CHLD\n");
		child = -1;
	}
}

void quit() {
	if (parent == getpid()) {	// in case child-process come in
		printf("\033[1mPid 1: Exit\n");
		exit();
	}
}

int uEntry(void) {
	char ch = 0;
	parent = getpid();
	while (1) {
		int ret = fork();
		if (ret != 0) { child = ret; signal(SIG_INT, &handler); signal(SIG_CHLD, &sigchild); signal(SIG_TERM, &quit); while(child != -1) sleep(128); }
		else {
			printf("---------------------------------------------------------------------------\n[uEntry]\nInput: 1 for bounded_buffer\t2 for philosopher\t3 for reader_writer\n       4 for app_print\t\t5 for scanf_test\t6 for sharedMem_test\n       7 for semaphore_test\t8 for random_number\t9 exit\n       A color_print\n");
			while (!((ch >= '1' && ch <= '9') || (ch == 'A' || ch == 'a'))) scanf("%c", &ch);
			switch (ch) {
				case '1':
					exec("/usr/bounded_buffer", 0);
					break;
				case '2':
					exec("/usr/philosopher", 0);
					break;
				case '3':
					exec("/usr/reader_writer", 0);
					break;
				case '4':
					exec("/usr/app_print", 0);
					break;
				case '5':
					exec("/usr/app_scanf", 0);
					break;
				case '6':
					exec("/usr/app_shmem", 0);
					break;
				case '7':
					exec("/usr/app_sema", 0);
					break;
				case '8':
					exec("/usr/app_rand", 0);
					break;
				case '9':
					kill(parent, SIG_TERM);
					sleep(10);
					exit();
					break;
				case 'A':
				case 'a':
					exec("/usr/app_color", 0);
					break;
				default:
					break;
			}
		}
	}
	exit();
	return 0;
}
