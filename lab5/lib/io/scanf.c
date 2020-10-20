#include "lib.h"
#include "types.h"


static int matchWhiteSpace(char *buffer, int size, int *count);
static int str2Dec(int *dec, char *buffer, int size, int *count);
static int str2Hex(int *hex, char *buffer, int size, int *count);
static int str2Str(char *string, int avail, char *buffer, int size, int *count);


int scanf(const char *format,...) {
	int i=0;
	char buffer[MAX_BUFFER_SIZE];
	int count=0; // buffer index
	int index=0; // parameter index
	void *paraList=(void*)&format;
	int state=0; // 0: legal character; 1: '%'; 2: string width;
	int avail=0; // string size
	int ret=0;
	buffer[0]=0;
	while(format[i]!=0){
		if(buffer[count]==0){
			do{
				ret = syscall(SYS_READ, STD_IN, (uint32_t)buffer, (uint32_t)MAX_BUFFER_SIZE, 0, 0);
			} while(ret == 0 || ret == -1);
			count=0;
		}
		switch(state){
			case 0:
				switch(format[i]){
					case '%':
						state = 1;
						break;
					case ' ':
					case '\t':
					case '\n':
						state = 0;
						matchWhiteSpace(buffer, MAX_BUFFER_SIZE, &count);
						break;
					default:
						if(format[i]!=buffer[count])
							return index/4;
						else{
							state=0;
							count++;
							break;
						}
				}
				break;
			case 1:
				switch(format[i]){
					case '%':
						if(format[i]!=buffer[count])
							return index/4;
						else{
							state=0;
							count++;
							break;
						}
					case 'd':
						state = 0;
						index+=4;
						ret=str2Dec(*(int**)(paraList+index), buffer, MAX_BUFFER_SIZE, &count);
						if(ret==-1)
							return (index-4)/4;
						else
							break;
					case 'x':
						state = 0;
						index+=4;
						ret=str2Hex(*(int**)(paraList+index), buffer, MAX_BUFFER_SIZE, &count);
						if(ret==-1)
							return (index-4)/4;
						else
							break;
					case 'c':
						state = 0;
						index+=4;
						*(*(char**)(paraList+index))=buffer[count];
						count++;
						break;
					case 's':
						state = 0;
						index+=4;
						ret=str2Str(*(char**)(paraList+index), -1, buffer, MAX_BUFFER_SIZE, &count);
						if(ret==-1)
							return (index-4)/4;
						else
							break;
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						state = 2;
						avail=0;
						avail+=format[i]-'0';
						break;
					default:
						return index/4;
				}
				break;
			case 2:
				switch(format[i]){
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						state = 2;
						avail*=10;
						avail+=format[i]-'0';
						break;
					case 's':
						state = 0;
						index+=4;
						ret=str2Str(*(char**)(paraList+index), avail, buffer, MAX_BUFFER_SIZE, &count);
						if(ret==-1)
							return (index-4)/4;
						else
							break;
					default:
						return index/4;
				}
				break;
			default:
				return index/4;
		}
		i++;
	}
	return index/4;
}

int matchWhiteSpace(char *buffer, int size, int *count){
	int ret=0;
	while(1){
		if(buffer[*count]==0){
			do{
				ret=syscall(SYS_READ, STD_IN, (uint32_t)buffer, (uint32_t)size, 0, 0);
			}while(ret == 0 || ret == -1);
			(*count)=0;
		}
		if(buffer[*count]==' ' ||
		   buffer[*count]=='\t' ||
		   buffer[*count]=='\n'){
			(*count)++;
		}
		else
			return 0;
	}
}

int str2Dec(int *dec, char *buffer, int size, int *count) {
	int sign=0; // positive integer
	int state=0;
	int integer=0;
	int ret=0;
	while(1){
		if(buffer[*count]==0){
			do{
				ret=syscall(SYS_READ, STD_IN, (uint32_t)buffer, (uint32_t)size, 0, 0);
			} while(ret == 0 || ret == -1);
			(*count)=0;
		}
		if(state==0){
			if(buffer[*count]=='-'){
				state=1;
				sign=1;
				(*count)++;
			}
			else if(buffer[*count]>='0' &&
				buffer[*count]<='9'){
				state=2;
				integer=buffer[*count]-'0';
				(*count)++;
			}
			else if(buffer[*count]==' ' ||
				buffer[*count]=='\t' ||
				buffer[*count]=='\n'){
				state=0;
				(*count)++;
			}
			else
				return -1;
		}
		else if(state==1){
			if(buffer[*count]>='0' &&
			   buffer[*count]<='9'){
				state=2;
				integer=buffer[*count]-'0';
				(*count)++;
			}
			else
				return -1;
		}
		else if(state==2){
			if(buffer[*count]>='0' &&
			   buffer[*count]<='9'){
				state=2;
				integer*=10;
				integer+=buffer[*count]-'0';
				(*count)++;
			}
			else{
				if(sign==1)
					*dec=-integer;
				else
					*dec=integer;
				return 0;
			}
		}
		else
			return -1;
	}
	return 0;
}

int str2Hex(int *hex, char *buffer, int size, int *count) {
	int state=0;
	int integer=0;
	int ret=0;
	while(1){
		if(buffer[*count]==0){
			do{
				ret=syscall(SYS_READ, STD_IN, (uint32_t)buffer, (uint32_t)size, 0, 0);
			}while(ret == 0 || ret == -1);
			(*count)=0;
		}
		if(state==0){
			if(buffer[*count]=='0'){
				state=1;
				(*count)++;
			}
			else if(buffer[*count]==' ' ||
				buffer[*count]=='\t' ||
				buffer[*count]=='\n'){
				state=0;
				(*count)++;
			}
			else if ((buffer[*count]>='0' && buffer[*count]<='9') ||
					(buffer[*count]>='a' && buffer[*count]<='f') ||
					(buffer[*count]>='A' && buffer[*count]<='F')) {
				state=2;
			}
			else
				return -1;
		}
		else if(state==1){
			if(buffer[*count]=='x'){
				state=2;
				(*count)++;
			}
			else
				return -1;
		}
		else if(state==2){
			if(buffer[*count]>='0' && buffer[*count]<='9'){
				state=3;
				integer*=16;
				integer+=buffer[*count]-'0';
				(*count)++;
			}
			else if(buffer[*count]>='a' && buffer[*count]<='f'){
				state=3;
				integer*=16;
				integer+=buffer[*count]-'a'+10;
				(*count)++;
			}
			else if(buffer[*count]>='A' && buffer[*count]<='F'){
				state=3;
				integer*=16;
				integer+=buffer[*count]-'A'+10;
				(*count)++;
			}
			else
				return -1;
		}
		else if(state==3){
			if(buffer[*count]>='0' && buffer[*count]<='9'){
				state=3;
				integer*=16;
				integer+=buffer[*count]-'0';
				(*count)++;
			}
			else if(buffer[*count]>='a' && buffer[*count]<='f'){
				state=3;
				integer*=16;
				integer+=buffer[*count]-'a'+10;
				(*count)++;
			}
			else if(buffer[*count]>='A' && buffer[*count]<='F'){
				state=3;
				integer*=16;
				integer+=buffer[*count]-'A'+10;
				(*count)++;
			}
			else{
				*hex=integer;
				return 0;
			}
		}
		else
			return -1;
	}
	return 0;
}

int str2Str(char *string, int avail, char *buffer, int size, int *count) {
	int i=0;
	int state=0;
	int ret=0;
	if (avail < 0)
		avail = 0x7fffffff;
	while(i < avail){
		if(buffer[*count]==0){
			do{
				ret=syscall(SYS_READ, STD_IN, (uint32_t)buffer, (uint32_t)size, 0, 0);
			}while(ret == 0 || ret == -1);
			(*count)=0;
		}
		if(state==0){
			if(buffer[*count]==' ' ||
			   buffer[*count]=='\t' ||
			   buffer[*count]=='\n'){
				state=0;
				(*count)++;
			}
			else{
				state=1;
				string[i]=buffer[*count];
				i++;
				(*count)++;
			}
		}
		else if(state==1){
			if(buffer[*count]==' ' ||
			   buffer[*count]=='\t' ||
			   buffer[*count]=='\n'){
				string[i]=0;
				return 0;
			}
			else{
				state=1;
				string[i]=buffer[*count];
				i++;
				(*count)++;
			}
		}
		else
			return -1;
	}
	string[i]=0;
	return 0;
}
