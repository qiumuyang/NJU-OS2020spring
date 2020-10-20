#include "lib.h"
#include "types.h"
#define true 1
#define false 0
#define bool _Bool
/*
 * io lib here
 * 库函数写在这
 */
//static inline int32_t syscall(int num, uint32_t a1,uint32_t a2,
int32_t syscall(int num, uint32_t a1,uint32_t a2,
		uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret = 0;
	//Generic system call: pass system call number in AX
	//up to five parameters in DX,CX,BX,DI,SI
	//Interrupt kernel with T_SYSCALL
	//
	//The "volatile" tells the assembler not to optimize
	//this instruction away just because we don't use the
	//return value
	//
	//The last clause tells the assembler that this can potentially
	//change the condition and arbitrary memory locations.

	/*
	XXX Note: ebp shouldn't be flushed
	    May not be necessary to store the value of eax, ebx, ecx, edx, esi, edi
	*/
	uint32_t eax, ecx, edx, ebx, esi, edi;
	// uint16_t selector;
	
	asm volatile("movl %%eax, %0":"=m"(eax));
	asm volatile("movl %%ecx, %0":"=m"(ecx));
	asm volatile("movl %%edx, %0":"=m"(edx));
	asm volatile("movl %%ebx, %0":"=m"(ebx));
	asm volatile("movl %%esi, %0":"=m"(esi));
	asm volatile("movl %%edi, %0":"=m"(edi));
	asm volatile("movl %0, %%eax"::"m"(num));
	asm volatile("movl %0, %%ecx"::"m"(a1));
	asm volatile("movl %0, %%edx"::"m"(a2));
	asm volatile("movl %0, %%ebx"::"m"(a3));
	asm volatile("movl %0, %%esi"::"m"(a4));
	asm volatile("movl %0, %%edi"::"m"(a5));
	asm volatile("int $0x80");
	asm volatile("movl %%eax, %0":"=m"(ret));
	asm volatile("movl %0, %%eax"::"m"(eax));
	asm volatile("movl %0, %%ecx"::"m"(ecx));
	asm volatile("movl %0, %%edx"::"m"(edx));
	asm volatile("movl %0, %%ebx"::"m"(ebx));
	asm volatile("movl %0, %%esi"::"m"(esi));
	asm volatile("movl %0, %%edi"::"m"(edi));
	
	// asm volatile("movw %%ss, %0":"=m"(selector)); //XXX %ds is reset after iret
	// selector = 16;
	// asm volatile("movw %%ax, %%ds"::"a"(selector));
	
	return ret;
}

unsigned time() {
	return syscall(SYS_TIME,0,0,0,0,0);
}
void sleep(unsigned seconds) {
	unsigned start = time();	// ms   10ms precision
	while (time() - start < seconds * 1000);
}

int dec2Str(int decimal, char *buffer, int size, int count);
int hex2Str(uint32_t hexadecimal, char *buffer, int size, int count);
int str2Str(char *string, char *buffer, int size, int count);
int oct2Str(uint32_t octal, char *buffer, int size, int count);
int u2Str(uint32_t u, char *buffer, int size, int count);

enum {
	NONE=0,
	LEFT=1,
	SIGN=2,
	SPACE=4,
	RADIX=8,
	ZERO=16,
	FILL=32
};

char is_specifier(char ch) {
	switch (ch) {
		case 'd': case 'x': case 'c': case 's': case 'o': case 'u': return ch;
		default: return false;
	}
}

int atoi(const char *p) {
	int res = 0;
	while (*p >= '0' && *p <= '9') {
		res *= 10;
		res += *p - '0';
		p++;
	}
	return res;
}

bool decode_fmt(const char *format, int *pos, int *type, int *align, char *specifier) {
	int fin = *pos;
	int flag = *pos;
	
	while (format[fin] && !is_specifier(format[fin])) fin++;
	if (!format[fin]) return false;
	*specifier = is_specifier(format[fin]);
	
	*type = 0;
	*align = 0;
	while (flag < fin && !(format[flag] > '0' && format[flag] <= '9')) flag++;
	for (int i = flag; i < fin; i++) {
		if (!(format[i] >= '0' && format[i] <= '9')) return false;
	}
	// [pos, flag) : flags
	// [flag, fin) : align
	for (int i = *pos; i < flag; i++) {
		switch (format[i]) {
			case '-': *type |= LEFT; break;
			case '+': *type |= SIGN; break;
			case ' ': *type |= SPACE;break;
			case '#': *type |= RADIX;break;
			case '0': *type |= ZERO;break;
			default: return false;
		}
	}
	if (*type & LEFT) *type &= ~ZERO;	// LEFT-align conflicts with ZERO-padding
	if (*type & SIGN) *type &= ~SPACE;	// fill Space conflicts with fill Sign
	*align = atoi(format + flag);
	*pos = fin - 1;
	return true;
}
int strlen(char *p) {
	int len = 0;
	while (*p) { len++; p++; }
	return len;
}
int numlen(int x, int radix) {
	int len = 0;
	if (radix == 0) {
		if (x < 0) len++;
		do {
			len++;
		} while (x /= 10);
	}
	else {
		uint32_t tmp = x;
		do {
			len++;
		} while (tmp /= radix);
	}
	return len;
}
int decorate(int type, int align, char specifier, char *buffer, int count, void *para) {
	int currentLen = 0;
	if (specifier == 'c') currentLen = 1;
	else if (specifier == 's') currentLen = strlen(*(char **)para);
	else if (specifier == 'u') currentLen = numlen(*(int *)para, 10);
	else if (specifier == 'd') {
		currentLen = numlen(*(int *)para, 0);
		if (((type & SIGN) || (type & SPACE)) && *(int *)para >= 0)
			currentLen++;
	}
	else if (specifier == 'x') {
		currentLen = numlen(*(int *)para, 16);
		if (type & RADIX) currentLen += 2;
	}
	else if (specifier == 'o') {
		currentLen = numlen(*(int *)para, 8);
		if (type & RADIX) currentLen++;
	}
	if (!(type & LEFT) && !(type & ZERO))
	{
		for (int i = 0; i < align - currentLen; i++) {
			buffer[count] = ' ';
			count++;
		}
	}
	if (!(type & FILL)) {
		if (specifier == 'd') {
			if ((type & SIGN) && *(int *)para >= 0) {
				buffer[count] = '+';
				count++;
			}
			else if ((type & SPACE) && *(int *)para >= 0) {
				buffer[count] = ' ';
				count++;
			}
		}
		if (specifier == 'x') {
			if (type & RADIX) {
				buffer[count] = '0';
				count++;
				buffer[count] = 'x';
				count++;
			}
		}
		if (specifier == 'o') {
			if (type & RADIX) {
				buffer[count] = '0';
				count++;
			}
		}
	}
	if (type & ZERO)
	{
		for (int i = 0; i < align - currentLen; i++) {
			buffer[count] = '0';
			count++;
		}
	}
	return count;
}
void printf(const char *format,...){
	int i = 0; // format index
	char buffer[MAX_BUFFER_SIZE];
	int count = 0; // buffer index
	void *paraList=(void*)&format; // address of format in stack
	
	int status = 0;
	int type = 0, align = 0; char specifier = 0;
	int negpos = 0, beginpos = 0;
	while (format[i]) {
		if (status == 0) {
			if (format[i] != '%') {
				buffer[count] = format[i];
				count++;
			}	
			else
				status = 1;
		}
		else if (status == 1) {
			type = 0; align = 0; specifier = 0;
			negpos = 0; beginpos = 0;
			if (format[i] == '%') {	// '%%' means '%'
				buffer[count] = format[i];
				count++;
				status = 0;
			}
			else {
				if (decode_fmt(format, &i, &type, &align, &specifier)) {
					// do some decoration before putting real str
					beginpos = count;
					count = decorate(type, align, specifier, buffer, count, paraList+4);
					if (type & LEFT) status = 3;
					else status = 2;
				}
				else {	// keep the origin string
					buffer[count] = '%'; count++;
					i--;
					status = 0;
				}
			}
		}
		else if (status == 2 || status == 3) {
			negpos = count;
			if (format[i] == 'd') {
				paraList += 4;
				count = dec2Str(*(int *)paraList, buffer, MAX_BUFFER_SIZE, count);
				if ((type & ZERO) && *(int *)paraList < 0) {
					buffer[negpos] = '0';
					buffer[beginpos] = '-';
				}
			}
			else if (format[i] == 'u') {
				paraList += 4;
				count = u2Str(*(uint32_t *)paraList, buffer, MAX_BUFFER_SIZE, count);
			}
			else if (format[i] == 'x') {
				paraList += 4;
				count = hex2Str(*(uint32_t *)paraList, buffer, MAX_BUFFER_SIZE, count);
			}
			else if (format[i] == 'c') {
				paraList += 4;
				buffer[count] = *(char *)paraList;
				count++;
			}
			else if (format[i] == 's') {
				paraList += 4;
				count = str2Str(*(char **)paraList, buffer, MAX_BUFFER_SIZE, count);
			}
			else if (format[i] == 'o') {
				paraList += 4;
				count = oct2Str(*(uint32_t *)paraList, buffer, MAX_BUFFER_SIZE, count);
			}
			if (status == 3) { // fill space after str
				type &= ~LEFT;
				type |= FILL;
				count = decorate(type, align, specifier, buffer, count, paraList);
			}
			status = 0;
		}
		i++;
	}
	if (count)
		syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)count, 0, 0);
}

void clrscr() {
	syscall(SYS_CLRSCR, 0, 0, 0, 0, 0);
}

int dec2Str(int decimal, char *buffer, int size, int count) {
	int i=0;
	int temp;
	int number[16];

	if(decimal<0){
		buffer[count]='-';
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		temp=decimal/10;
		number[i]=temp*10-decimal;
		decimal=temp;
		i++;
		while(decimal!=0){
			temp=decimal/10;
			number[i]=temp*10-decimal;
			decimal=temp;
			i++;
		}
	}
	else{
		temp=decimal/10;
		number[i]=decimal-temp*10;
		decimal=temp;
		i++;
		while(decimal!=0){
			temp=decimal/10;
			number[i]=decimal-temp*10;
			decimal=temp;
			i++;
		}
	}

	while(i!=0){
		buffer[count]=number[i-1]+'0';
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		i--;
	}
	return count;
}

int hex2Str(uint32_t hexadecimal, char *buffer, int size, int count) {
	int i=0;
	uint32_t temp=0;
	int number[16];

	temp=hexadecimal>>4;
	number[i]=hexadecimal-(temp<<4);
	hexadecimal=temp;
	i++;
	while(hexadecimal!=0){
		temp=hexadecimal>>4;
		number[i]=hexadecimal-(temp<<4);
		hexadecimal=temp;
		i++;
	}

	while(i!=0){
		if(number[i-1]<10)
			buffer[count]=number[i-1]+'0';
		else
			buffer[count]=number[i-1]-10+'a';
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		i--;
	}
	return count;
}

int oct2Str(uint32_t octal, char *buffer, int size, int count) {
	int i=0;
	uint32_t temp=0;
	int number[16];

	temp=octal>>3;
	number[i]=octal-(temp<<3);
	octal=temp;
	i++;
	while(octal!=0){
		temp=octal>>3;
		number[i]=octal-(temp<<3);
		octal=temp;
		i++;
	}
	
	while(i!=0){
		buffer[count]=number[i-1]+'0';
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		i--;
	}
	return count;
}

int u2Str(uint32_t u, char *buffer, int size, int count) {
	int i=0;
	uint32_t temp=0;
	int number[16];

	temp=u/10;
	number[i]=u-(temp*10);
	u=temp;
	i++;
	while(u!=0){
		temp=u/10;
		number[i]=u-(temp*10);
		u=temp;
		i++;
	}
	
	while(i!=0){
		buffer[count]=number[i-1]+'0';
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		i--;
	}
	return count;
}

int str2Str(char *string, char *buffer, int size, int count) {
	int i=0;
	while(string[i]!=0){
		buffer[count]=string[i];
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, 0, 0);
			count=0;
		}
		i++;
	}
	return count;
}
