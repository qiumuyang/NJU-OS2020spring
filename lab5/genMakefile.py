import os
import re

built_in = ['utils/genFS', 'bootloader', 'kernel', 'shell', 'utils', 'lib', 'bin', 'usr']

def write_AppMakefile(path, name):
    print(f'Generate Makefile for {path}/{name}')
    content = f'''CC = gcc
LD = ld

CFLAGS = -m32 -march=i386 -static \
	 -fno-builtin -fno-stack-protector -fno-omit-frame-pointer \
	 -Wall -Werror -O2 -I../../lib
LDFLAGS = -m elf_i386

UCFILES = $(shell find ./ -name "*.c")
LCFILES = $(shell find ../../lib -name "*.c")
UOBJS = $(UCFILES:.c=.o) $(LCFILES:.c=.o)

app.bin: $(UOBJS)
	$(LD) $(LDFLAGS) -e main -Ttext 0x00000000 -o {name}.elf $(UOBJS)

clean:
	rm -rf $(UOBJS) {name}.elf'''
    with open(f'{path}/Makefile', 'w') as f:
        f.write(content)

def write_genFS(files):
    insideNames = [lis[1] for lis in files]
    head = \
'''#include <stdio.h>
#include "utils.h"
#include "data.h"
#include "func.h"

int main(int argc, char *argv[]) {
    char driver[NAME_LENGTH];
    char srcFilePath[NAME_LENGTH];
    char destFilePath[NAME_LENGTH];

    stringCpy("fs.bin", driver, NAME_LENGTH - 1);
    format(driver, SECTOR_NUM, SECTORS_PER_BLOCK);

    stringCpy("/boot", destFilePath, NAME_LENGTH - 1);
    mkdir(driver, destFilePath);

    stringCpy("/dev", destFilePath, NAME_LENGTH - 1);
    mkdir(driver, destFilePath);
    
    stringCpy("/dev/stdin", destFilePath, NAME_LENGTH - 1);
	touch(driver, destFilePath, CHARACTER_TYPE);
	
	stringCpy("/dev/stdout", destFilePath, NAME_LENGTH - 1);
	touch(driver, destFilePath, CHARACTER_TYPE);
	
	stringCpy("/dev/shmem", destFilePath, NAME_LENGTH - 1);
	touch(driver, destFilePath, CHARACTER_TYPE);

    stringCpy(argv[1], srcFilePath, NAME_LENGTH - 1);
    stringCpy("/boot/initrd", destFilePath, NAME_LENGTH - 1);
    cp(driver, srcFilePath, destFilePath);

    stringCpy("/usr", destFilePath, NAME_LENGTH - 1);
    mkdir(driver, destFilePath);
    
    stringCpy("/usr/bin", destFilePath, NAME_LENGTH - 1);
    mkdir(driver, destFilePath);

    stringCpy("/home", destFilePath, NAME_LENGTH - 1);
    mkdir(driver, destFilePath);

'''
    base = 2
    for i, name in enumerate(insideNames):
        head += f'\tstringCpy(argv[{base+i}], srcFilePath, NAME_LENGTH - 1);\n'
        head += f'\tstringCpy("{name}", destFilePath, NAME_LENGTH - 1);\n'
        head += '\tcp(driver, srcFilePath, destFilePath);\n\n'
    end = \
'''
    stringCpy("write_alpha/main.c", srcFilePath, NAME_LENGTH - 1);
    stringCpy("/home/write_alpha.c", destFilePath, NAME_LENGTH - 1);
    cp(driver, srcFilePath, destFilePath);

    stringCpy("/", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);

    stringCpy("/boot", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);

    stringCpy("/usr", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);

    stringCpy("/usr/bin", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);

    stringCpy("/dev", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);

    stringCpy("/home", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);

    return 0;
}
'''.strip('\n')
    with open('utils/genFS/main.c', 'w') as f:
        f.write(head+end)


def decode_GenFS():
    ret = {}
    with open('utils/genFS/main.c') as f:
        code = f.readlines()
    for i, line in enumerate(code):
        match = re.search(r'stringCpy\(argv\[(\d+)\]', line)
        if match:
            next_match = re.search(r'stringCpy\("(.*?)"', code[i+1])
            if next_match:
                ret[int(match.group(1)) - 1] = next_match.group(1)
    return ret


def decode_MainMakefile():
    with open('Makefile') as f:
        code = f.readlines()
    for line in code:
        match = re.search(r'@\./utils/genFS/genFS (.*)', line)
        if match:
            return re.split(' +', match.group(1))


def write_MainMakefile(files):
    folders = [lis[2] for lis in files]
    fullnames = [lis[0] for lis in files]
    head = \
        '''QEMU = qemu-system-i386

os.img:
	@cd utils/genFS; make
	@cd bootloader; make
	@cd kernel; make
	@cd shell; make
'''
    head += '\n'.join(
        [f'\t@cd {name}; make' for name in folders if name not in built_in])
    head += '\n\t@./utils/genFS/genFS shell/uMain.elf ' + ' '.join(fullnames)
    mid = '''
	cat bootloader/bootloader.bin kernel/kMain.elf fs.bin > os.img

play: os.img
	$(QEMU) -serial stdio os.img

debug: os.img
	$(QEMU) -serial stdio -s -S os.img

clean:
	@cd utils/genFS; make clean
	@cd bootloader; make clean
	@cd kernel; make clean
	@cd shell; make clean
'''
    mid += '\n'.join(
        [f'\t@cd {name}; make clean' for name in folders if name not in built_in])
    end = '''\n\trm -f fs.bin os.img'''
    with open('Makefile', 'w') as f:
        f.write(head+mid+end)


def read_apps():
    insideName = decode_GenFS()
    outsideName = decode_MainMakefile()
    data = [[name, insideName[i], name.split("/")[0], name.split("/")[1]] for i, name in enumerate(outsideName)]
    return data


def show_apps(data: list):
    m1 = max([len(name) for name in [lis[0] for lis in data]])
    m2 = max([len(name) for name in [lis[1] for lis in data]])
    for item in data:
        print(f'{item[0]:-<{m1}}-->{item[1]:<{m2}} {"Built-in" if item[2] in built_in else " "}')


def add_apps():
    data = []
    folders = [item for item in os.listdir('usr/') if os.path.isdir('usr/' + item) and item not in built_in and item[0] != '.']
    for folder in folders:
        files = os.listdir('usr/' + folder)
        elf_name = folder
        insideName = f'/home/{elf_name}'
        data.append([os.path.join('usr', folder, elf_name+'.elf'), insideName, 'usr/' + folder, elf_name+'.elf'])
        if 'Makefile' not in files:
            write_AppMakefile('usr/' + folder, elf_name)

    folders = [item for item in os.listdir('bin/') if os.path.isdir('bin/'+item) and item not in built_in and item[0] != '.']
    for folder in folders:
        files = os.listdir('bin/' + folder)
        elf_name = folder
        insideName = f'/usr/bin/{elf_name}'
        data.append([os.path.join('bin', folder, elf_name+'.elf'), insideName, 'bin/' + folder, elf_name+'.elf'])
        if 'Makefile' not in files:
            write_AppMakefile('bin/' + folder, elf_name)
    
    return data


def genMakefile():
    app_lis = add_apps()
    write_MainMakefile(app_lis)
    #write_binMakefile(app_lis)
    write_genFS(app_lis)


if __name__ == "__main__":
    s = input('1. Show Current Apps\n2. Generate Makefile according to Folder-Structure\n')
    if s == '1':
        app_lis = read_apps()
        show_apps(app_lis)
    elif s == '2':
        genMakefile()
