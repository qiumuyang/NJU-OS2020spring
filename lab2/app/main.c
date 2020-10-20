#include "lib.h"
#include "types.h"

void prtfmt_test() {
}

void scroll_test(void) {
	printf("%s", "Scroll Screen Test will start in:  ");
	for (int i = 15; i > 0; i--) {
		printf("\b\b%2d",i);
		sleep(1);
	}
	printf("\n");
	for(int i = 0; i < 5; i++) {
		printf("%d\n", i);
		sleep(1);
	}	
}

void official_test(void) {
	printf("printf test begin...\n");
	printf("the answer should be:\n");
	printf("#######################################################\n");
	printf("Hello, welcome to OSlab! I'm the body of the game. ");
	printf("Bootblock loads me to the memory position of 0x100000, and Makefile also tells me that I'm at the location of 0x100000. ");
	printf("\\%%~!@#/(^&*()_+`1234567890-=...... ");
	printf("Now I will test your printf: ");
	printf("1 + 1 = 2, 123 * 456 = 56088\n0, -1, -2147483648, -1412567295, -32768, 102030\n0, ffffffff, 80000000, abcdef01, ffff8000, 18e8e\n");
	printf("#######################################################\n");
	printf("your answer:\n");
	printf("=======================================================\n");
	printf("%s %s%scome %co%s", "Hello,", "", "wel", 't', " ");
	printf("%c%c%c%c%c! ", 'O', 'S', 'l', 'a', 'b');
	printf("I'm the %s of %s. %s %#x, %s 0x%x. ", "body", "the game",
			"Bootblock loads me to the memory position of", 0x100000, "and Makefile also tells me that I'm at the location of", 0x100000);
	printf("\\%%~!@#/(^&*()_+`1234567890-=...... ");
	printf("Now I will test your printf: ");
	printf("%d + %d = %d, %d * %d = %d\n", 1, 1, 1 + 1, 123, 456, 123 * 456);
	printf("%d, %d, %d, %d, %d, %d\n", 0, 0xffffffff, 0x80000000, 0xabcdef01, -32768, 102030);
	printf("%x, %x, %x, %x, %x, %x\n", 0, 0xffffffff, 0x80000000, 0xabcdef01, -32768, 102030);
	printf("=======================================================\n");
	printf("Test end!!! Good luck!!!\n");
}

int uEntry(void) {
	official_test();
	scroll_test();
	prtfmt_test();
	printf("%43s","ALL TESTS END.");
	while(1);
	return 0;
}
