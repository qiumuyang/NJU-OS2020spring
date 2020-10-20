#include "lib.h"
#include "types.h"

int buffer_size = 0;

sem_t empty = 0;
sem_t full = 0;
sem_t mutex = 0;

int parent;
int child[4] = {0};

void randSleep() {
	int tm = rand() % 5 * 29 + 17;
	sleep(tm);
}

void producer(int id) {
	while (1) {
		sem_wait(&empty);
		sem_wait(&mutex);
		
		printf("Producer %d: produce\n", id);
		randSleep();	// producing
		
		sem_post(&mutex);
		sem_post(&full);
		randSleep();	// wait for next producing period
	}
}

void consumer() {
	while (1) {
		sem_wait(&full);
		sem_wait(&mutex);
		
		printf("Consumer  : consume\n");
		randSleep();	// consuming
		
		sem_post(&mutex);
		sem_post(&empty);
		randSleep();	// wait for next consuming period
	}
}

void handler() {
	if (getpid() == parent) {
		for (int i = 0; i < 4; i++) {
			if (child[i] != 0) kill(child[i], SIG_TERM);
		}
		if (full) sem_destroy(&full);
		if (empty) sem_destroy(&empty);
		if (mutex) sem_destroy(&mutex);
		sleep(256);
	}
	printf("\033[7mPid %d exits, ppid %d\n", getpid(), getppid());
	exit();
}

int main(void) {
	// TODO in lab4
	printf("[bounded_buffer]\n");
	signal(SIG_TERM, &handler);
	parent = getpid();
	
	printf("Enter Buffer Size (size > 2): ");
	while (buffer_size <= 2) {
		scanf("%d", &buffer_size);
		printf("%d\n", buffer_size);
	}
		
	int id = 0;
	if (sem_init(&empty, buffer_size) == -1 || sem_init(&full, 0) == -1 || sem_init(&mutex, 1) == -1) {
		printf("Semaphore Init Fail\n");
		exit();
	}
	for (int i = 0; i < 4; i++) {
		if (parent == getpid()) {
			int ret = fork();
			if (ret == 0) { // child
				id = i + 1;
				break;
			}
			else {			// parent
				child[i] = ret;
			}
		}
	}
	srand(time());
	randSleep();
	if (getpid() == parent) consumer();
	else producer(id);
	exit();
	return 0;
}




/*
int main(void) {
	// TODO in lab4
	printf("[bounded_buffer]\n");
	signal(SIG_TERM, &handler);
	parent = getpid();
	
	printf("Enter Buffer Size(> 2): ");
	while (buffer_size <= 2) {
		scanf("%d", &buffer_size);
		printf("%d\n", buffer_size);
	}
		
	int id = 0;
	int in = 0, out = 0, product = 0;
	int buffer = 0;
	write(SH_MEM, (uint8_t *)&in, 4, 0);
	write(SH_MEM, (uint8_t *)&out, 4, 4);
	write(SH_MEM, (uint8_t *)&product, 4, 8);
	write(SH_MEM, (uint8_t *)&buffer, 4 * buffer_size, 12);
	if (sem_init(&empty, buffer_size) == -1 || sem_init(&full, 0) == -1 || sem_init(&mutex, 1) == -1) {
		printf("Semaphore Init Fail\n");
		exit();
	}
	for (int i = 0; i < 4; i++) {
		if (parent == getpid()) {
			int ret = fork();
			if (ret == 0) { // child
				id = i + 1;
				break;
			}
			else {			// parent
				child[i] = ret;
			}
		}
	}
	srand(time());
	randSleep();
	if (getpid() == parent) consumer();
	else producer(id);
	exit();
	return 0;
}


void producer(int id) {
	while (1) {
		sem_wait(&empty);
		sem_wait(&mutex);
		
		int product, in;
		read(SH_MEM, (uint8_t *)&in, 4, 0);
		read(SH_MEM, (uint8_t *)&product, 4, 8);
		product = rand() % 100 + 1;
		printf("Producer %d: produce (%d)\n", id, product);
		write(SH_MEM, (uint8_t *)&product, 4, 12 + in * 4);
		in = (in + 1) % buffer_size;
		write(SH_MEM, (uint8_t *)&in, 4, 0);
		write(SH_MEM, (uint8_t *)&product, 4, 8);
		
		randSleep();	// producing
		sem_post(&mutex);
		sem_post(&full);
		randSleep();	// wait for next producing period
	}
}

void consumer() {
	while (1) {
		sem_wait(&full);
		sem_wait(&mutex);
		
		int out, product;
		read(SH_MEM, (uint8_t *)&out, 4, 4);
		read(SH_MEM, (uint8_t *)&product, 4, 12 + out * 4);
		out = (out + 1) % buffer_size;
		printf("Consumer  : consume (%d)\n", product);
		write(SH_MEM, (uint8_t *)&out, 4, 4);
		
		randSleep();	// consuming
		sem_post(&mutex);
		sem_post(&empty);
		randSleep();	// wait for next consuming period
	}
}
*/
