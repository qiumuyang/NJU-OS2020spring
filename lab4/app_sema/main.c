#include "lib.h"
#include "types.h"

int child = -1;
int parent = 0;

sem_t sem;

void handler() {
	if (getpid() == parent) {
		if (child != -1) {
			kill(child, SIG_TERM);
			child = -1;
		}
		sem_destroy(&sem);
		sleep(100);
	}
	printf("\033[7mPid %d exits, ppid %d\n", getpid(), getppid());
	exit();
}

int main(void) {
	printf("[official semaphore test]\n");
	signal(SIG_TERM, &handler);
	parent = getpid();
	int i = 4;
	int ret = 0;
	int value = 2;
	printf("Father Process: Semaphore Initializing.\n");
	ret = sem_init(&sem, value);
	if (ret == -1) {
		printf("Father Process: Semaphore Initializing Failed.\n");
		exit();
	}
	ret = fork();
	if (ret == 0) {
		while(i != 0) {
			i --;
			printf("Child Process: Semaphore Waiting.\n");
			sem_wait(&sem);
			printf("Child Process: In Critical Area.\n");
		}
		printf("Child Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		exit();
	}
	else if (ret != -1) {
		child = ret;
		while(i != 0) {
			i --;
			printf("Father Process: Sleeping.\n");
			sleep(128);
			printf("Father Process: Semaphore Posting.\n");
			sem_post(&sem);
		}
		printf("Father Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		sleep(1024);
		exit();
	}
	return 0;
}
