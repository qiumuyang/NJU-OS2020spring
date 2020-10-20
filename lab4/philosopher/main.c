#include "lib.h"
#include "types.h"
#define N 5

sem_t Forks[N];

int parent;
int child[N-1];

void randSleep() {
	int tm = rand() % 11 * 37 + 71;
	sleep(tm);
}

void philosopher(int i) {
	while(1) {
		printf("Philosopher %d: think\n", i+1);
		randSleep(); // think()
	 	if(i % 2 == 0) {
	 		sem_wait(&Forks[i]);
	 		sem_wait(&Forks[(i+1)%N]);
		}
		else {
	 		sem_wait(&Forks[(i+1)%N]);
	 		sem_wait(&Forks[i]);
	 	}
	 	printf("Philosopher %d: eat\n", i+1);
		randSleep(); // eat()
		sem_post(&Forks[i]);
		sem_post(&Forks[(i+1)%N]);
	}
}

void handler() {
	if (getpid() == parent) {
		for (int i = 0; i < N - 1; i++) {
			kill(child[i], SIG_TERM);
		}
		for (int i = 0; i < N; i++) {
			sem_destroy(&Forks[i]);
		}
		sleep(256);
	}
	printf("\033[7mPid %d exits, ppid %d\n", getpid(), getppid());
	exit();
}

int main(void) {
	// TODO in lab4
	printf("[philosopher]\n");
	signal(SIG_TERM, &handler);
	parent = getpid();
	
	int id = 0;
	for (int i = 0; i < N; i++) {
		int ret = sem_init(&Forks[i], 1);
		if (ret == -1) {
			printf("Semaphore Init Fail\n");
			exit();
		}
	}
	for (int i = 1; i < N; i++) {
		if (parent == getpid()) {
			int ret = fork();
			if (ret == 0) { // child
				id = i;
				break;
			}
			else {			// parent
				child[i-1] = ret;
			}
		}
	}
	srand(time());
	randSleep();
	philosopher(id);
	exit();
	return 0;
}
