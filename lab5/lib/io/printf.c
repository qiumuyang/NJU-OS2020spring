#include "lib.h"
#include "types.h"


static int dec2Str(int decimal, char *buffer, int size, int count, int font, int bg);
static int hex2Str(uint32_t hexadecimal, char *buffer, int size, int count, int font, int bg);
static int str2Str(char *string, char *buffer, int size, int count, int font, int bg);
static int str2Color(const char *color, int *font, int *bg);
static int oct2Str(uint32_t octal, char *buffer, int size, int count, int font, int bg);
static int u2Str(uint32_t u, char *buffer, int size, int count, int font, int bg);
static char is_specifier(char ch);
static bool decode_fmt(const char *format, int *pos, int *type, int *align, char *specifier);
static int decorate(int type, int align, char specifier, char *buffer, int size, int count, void *para, int font, int bg);

enum {
	NONE=0,
	LEFT=1,
	SIGN=2,
	SPACE=4,
	RADIX=8,
	ZERO=16,
	FILL=32
};

int printf(const char *format,...){
	int i=0; // format index
	char buffer[MAX_BUFFER_SIZE];
	int count=0; // buffer index
	void *paraList=(void*)&format; // address of format in stack

	int state=0; // 0: legal character; 1: '%'; 2: illegal format; 3: '\033'
	int type = 0, align = 0; char specifier = 0;

	int font=0xf;
	int bg=0;
	int ret=0;

	while(format[i]!=0){
		switch(state){
			case 0:
				switch(format[i]){
					case '%': // '%' inputed
						state = 1;
						break;
					case '\033':
						state = 3;
						break;
					default: // normal character inputed
						state = 0;
						buffer[count]=format[i];
						count++;
						break;
				}
				break;
			case 1: // '%' is on top of stack
				type = 0; align = 0; specifier = 0;
				if (format[i] == '%') {	// '%%' means '%'
					buffer[count] = format[i];
					count++;
					state = 0;
				}
				else {
					if (decode_fmt(format, &i, &type, &align, &specifier)) {
						// do some decoration before putting real str
						count = decorate(type, align, specifier, buffer, MAX_BUFFER_SIZE, count, paraList+4, font, bg);
						if (type & LEFT) state = 4;
						else state = 2;
					}
					else {	// keep the origin string
						buffer[count] = '%'; count++;
						i--;
						state = 0;
					}
				}
				break;
			case 2:
			case 4:
				switch(format[i]) {
					case 'd':
						paraList += 4;
						if (*(int *)paraList == 0x80000000) {
							count = u2Str(*(int *)paraList * -1, buffer, MAX_BUFFER_SIZE, count, font, bg);
						}
						else if (*(int *)paraList < 0) {
							count = dec2Str(*(int *)paraList * -1, buffer, MAX_BUFFER_SIZE, count, font, bg);
						}
						else {
							count = dec2Str(*(int *)paraList, buffer, MAX_BUFFER_SIZE, count, font, bg);
						}
						break;
					case 'u':
						paraList += 4;
						count = u2Str(*(uint32_t *)paraList, buffer, MAX_BUFFER_SIZE, count, font, bg);
						break;
					case 'x':
						paraList += 4;
						count = hex2Str(*(uint32_t *)paraList, buffer, MAX_BUFFER_SIZE, count, font, bg);
						break;
					case 'c':
						paraList += 4;
						buffer[count] = *(char *)paraList;
						count++;
						break;
					case 's':
						paraList += 4;
						count = str2Str(*(char **)paraList, buffer, MAX_BUFFER_SIZE, count, font, bg);
						break;
					case 'o':
						paraList += 4;
						count = oct2Str(*(uint32_t *)paraList, buffer, MAX_BUFFER_SIZE, count, font, bg);
				}
				if (state == 4) { // fill space after str
					type &= ~LEFT;
					type |= FILL;
					count = decorate(type, align, specifier, buffer, MAX_BUFFER_SIZE, count, paraList, font, bg);
				}
				state = 0;
				break;
			case 3:
				syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, count, font, bg);
				count = 0; // clear buffer for new color
				ret = str2Color(format + i, &font, &bg);
				i += ret; // if escape failed, then i--, so as to print original letters
				state = 0;
				break;
			default:
				break;
		}
		if(count == MAX_BUFFER_SIZE) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)MAX_BUFFER_SIZE, font, bg);
			count=0;
		}
		i++;
	}
	if(count!=0)
		syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)count, font, bg);
	return 0;
}

int dec2Str(int decimal, char *buffer, int size, int count, int font, int bg) {
	int i=0;
	int temp;
	int number[16];

	if(decimal<0){
		buffer[count]='-';
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, font, bg);
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
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, font, bg);
			count=0;
		}
		i--;
	}
	return count;
}

int hex2Str(uint32_t hexadecimal, char *buffer, int size, int count, int font, int bg) {
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
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, font, bg);
			count=0;
		}
		i--;
	}
	return count;
}

int str2Str(char *string, char *buffer, int size, int count, int font, int bg) {
	int i=0;
	while(string[i]!=0){
		buffer[count]=string[i];
		count++;
		if(count==size) {
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, font, bg);
			count=0;
		}
		i++;
	}
	return count;
}

int oct2Str(uint32_t octal, char *buffer, int size, int count, int font, int bg) {
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
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, font, bg);
			count=0;
		}
		i--;
	}
	return count;
}

int u2Str(uint32_t u, char *buffer, int size, int count, int font, int bg) {
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
			syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, font, bg);
			count=0;
		}
		i--;
	}
	return count;
}

int str2Color(const char *color, int *font, int *bg) {
	int tmp = 0;
	const char *p = color;
	if (*p != '[') return -1;
	p++;
	while ((*p) >= '0' && (*p) <= '9') {
		tmp = tmp * 10 + *p - '0';
		p++;
	}
	if (*p == 'm') {
		*font = tmp;
		return p - color;
	}
	else if (*p == ';') {
		*font = tmp;
		tmp = 0;
		p++;
		while ((*p) >= '0' && (*p) <= '9') {
			tmp = tmp * 10 + *p - '0';
			p++;
		}
		if (*p == 'm') {
			*bg = tmp;
			return p - color;
		}
	}
	return -1;
}

char is_specifier(char ch) {
	switch (ch) {
		case 'd': case 'x': case 'c': case 's': case 'o': case 'u': return ch;
		default: return false;
	}
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

int decorate(int type, int align, char specifier, char *buffer, int size, int count, void *para, int font, int bg) {
	int currentLen = 0;
	if (specifier == 'c') currentLen = 1;
	else if (specifier == 's') currentLen = strlen(*(char **)para);
	else if (specifier == 'u') currentLen = numlen(*(int *)para, 10);
	else if (specifier == 'd') {
		currentLen = numlen(*(int *)para, 0);
		if (((type & SIGN) || (type & SPACE)) && *(int *)para >= 0)
			currentLen++;
		//else if (*(int *)para < 0)
		//	currentLen++;
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
			if(count==size) {
				syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, font, bg);
				count=0;
			}
		}
	}
	if (!(type & FILL)) {
		if (specifier == 'd') {
			if ((type & SIGN) && *(int *)para >= 0) {
				buffer[count] = '+';
				count++;
				if(count==size) {
					syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, font, bg);
					count=0;
				}
			}
			else if ((type & SPACE) && *(int *)para >= 0) {
				buffer[count] = ' ';
				count++;
				if(count==size) {
					syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, font, bg);
					count=0;
				}
			}
			else if (*(int *)para < 0) {
				buffer[count] = '-';
				count++;
				if(count==size) {
					syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, font, bg);
					count=0;
				}
			}
		}
		if (specifier == 'x') {
			if (type & RADIX) {
				buffer[count] = '0';
				count++;
				if(count==size) {
					syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, font, bg);
					count=0;
				}
				buffer[count] = 'x';
				count++;
				if(count==size) {
					syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, font, bg);
					count=0;
				}
			}
		}
		if (specifier == 'o') {
			if (type & RADIX) {
				buffer[count] = '0';
				count++;
				if(count==size) {
					syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, font, bg);
					count=0;
				}
			}
		}
	}
	if (type & ZERO)
	{
		for (int i = 0; i < align - currentLen; i++) {
			buffer[count] = '0';
			count++;
			if(count==size) {
				syscall(SYS_WRITE, STD_OUT, (uint32_t)buffer, (uint32_t)size, font, bg);
				count=0;
			}
		}
	}
	return count;
}