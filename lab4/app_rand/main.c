#include "lib.h"
#include "types.h"

void handler() {
	printf("\033[7mPid %d exits, ppid %d\n", getpid(), getppid());
	exit();
}

int main(void) {
	printf("[Rand]\nInput maximum random result(<32767)\nEach press generates 1 result.\n");
	signal(SIG_TERM, &handler);
	char c = 0;
	int max = -1;
	printf("Input: Max(%%d) ");
	while (!scanf("%d", &max) || max < 0);
	printf("%d\n", max);
	srand(time());
	do {
		printf("%d\n", rand()%max);
		scanf("%c", &c);
	} while (c != 'q' && c != 'Q');
	exit(0);
	return 0;
}
