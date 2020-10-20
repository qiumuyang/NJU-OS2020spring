#include "lib.h"
#include "types.h"

void handler() {
	printf("\033[7mPid %d exits, ppid %d\n", getpid(), getppid());
	exit();
}

int main(void) {
	printf("[official scanf test]\n");
	signal(SIG_TERM, &handler);
	int dec = 0;
	int hex = 0;
	char str[7];
	for (int i = 0; i < 7; i++)
		str[i] = 0;
	char cha = 0;
	int ret = 0;
	while(1){
		printf("Input:\" Test %%c Test %%6s %%d %%x\"\n");
		ret = scanf(" Test %c Test %6s %d %x", &cha, str, &dec, &hex);
		printf("Ret: %d; %c, %s, %d, %x.\n", ret, cha, str, dec, hex);
		if (ret == 4)
			break;
		dec = 0; hex = 0; cha = 0; for(int i = 0; i < 7; i++) str[i] = 0;
	}
	exit();
	return 0;
}
