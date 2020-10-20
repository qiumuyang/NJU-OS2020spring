#include "lib.h"
#include "types.h"

int child = -1;
int parent = 0;

void handler() {
	if (getpid() == parent) {
		//printf("%d Kill %d\n", getpid(), child);
		kill(child, SIG_TERM);
		sleep(100);
	}
	printf("\033[7mPid %d exits, ppid %d\n", getpid(), getppid());
	exit();
}

int main(void) {
	printf("[official shared memory test]\n");
	signal(SIG_TERM, &handler);
	parent = getpid();
	
	int data = 2020;
	int data1 = 1000;
	int i = 4;
	int ret = fork();
	/* initialize SHMEM with all 0 */
	uint8_t clr[4096] = {0};
	write(SH_MEM, clr, 4096, 0);
	
	if (ret == 0) {
		while (i != 0) {
			i--;
			printf("Child Process: %d, %d\n", data, data1);
			write(SH_MEM, (uint8_t *)&data, 4, 0);
			data += data1;
			sleep(128);
		}
		exit();
		} 
	else if (ret != -1) {
		child = ret;
		while (i != 0) {
			i--;
			read(SH_MEM, (uint8_t *)&data1, 4, 0);
			printf("Father Process: %d, %d\n", data, data1);
			sleep(128);
		}
		exit();
	}
	return 0;
}
