#include "lib.h"
#include "types.h"

sem_t WriteMutex = 0;
sem_t CountMutex = 0;
int Rcount = 0;
int parent;
int child[5];

void randSleep() {
	int tm = rand() % 11 * 37 + 17;
	sleep(tm);
}

void writer(int id) {
	while (1) {
		sem_wait(&WriteMutex);
		
		printf("Writer %d: write\n", id);
		randSleep();	// writing
		
		sem_post(&WriteMutex);
		randSleep();	// after a few seconds require to write again
	}
}

void reader(int id) {
	while (1) {
	
		// modify reader_count
		sem_wait(&CountMutex);
		read(SH_MEM, (uint8_t *)&Rcount, 4, 0);
		if (Rcount == 0) {		// no reader, wait until writer finish
			sem_wait(&WriteMutex);
		}
		++Rcount;
		write(SH_MEM, (uint8_t *)&Rcount, 4, 0);
		sem_post(&CountMutex);	// finish write-back reader_count
	
		
		// reading
		printf("Reader %d: read, total %d reader\n", id, Rcount);
		randSleep();
		
		
		// modify reader_count
		sem_wait(&CountMutex);
		read(SH_MEM, (uint8_t *)&Rcount, 4, 0);
		--Rcount;
		printf("Reader %d: Stop read, total %d reader\n", id, Rcount);	// finish reading
		write(SH_MEM, (uint8_t *)&Rcount, 4, 0);
		if (Rcount == 0) {		// no reader, writer now can write
			sem_post(&WriteMutex);
		}
		sem_post(&CountMutex);	// finish write-back reader_count
		randSleep();
	}
}

void handler() {
	if (getpid() == parent) {
		for (int i = 0; i < 5; i++) {
			kill(child[i], SIG_TERM);
		}
		if (WriteMutex) sem_destroy(&WriteMutex);
		if (CountMutex) sem_destroy(&CountMutex);
		sleep(256);
	}
	printf("\033[7mPid %d exits, ppid %d\n", getpid(), getppid());
	exit();
}

int main(void) {
	// TODO in lab4
	printf("[reader_writer]\n");
	signal(SIG_TERM, &handler);
	parent = getpid();
	
	printf("Enter Writer Num (1 <= num <= 3): ");
	int writer_cnt = -1;
	while (writer_cnt > 3 || writer_cnt < 1) {
		scanf("%d", &writer_cnt);
		printf("%d\n", writer_cnt);
	}
	write(SH_MEM, (uint8_t *)&Rcount, 4, 0);
	if (sem_init(&CountMutex, 1) == -1 || sem_init(&WriteMutex, 1) == -1) {
		printf("Semaphore Init Fail\n");
		exit();
	}
	
	int id = 0;
	for (int i = 0; i < 5; i++) {
		if (getpid() == parent) {
			int ret = fork();
			if (ret != 0) {	// father process
				child[i] = ret;
			}
			else {			// child process
				id = i + 1;
				break;
			}
		}
	}
	srand(time());
	randSleep();
	if (id < writer_cnt)
		writer(id + 1);
	else 
		reader(id - writer_cnt + 1);
	exit();
	return 0;
}
