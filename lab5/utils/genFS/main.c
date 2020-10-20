#include <stdio.h>
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

	stringCpy(argv[2], srcFilePath, NAME_LENGTH - 1);
	stringCpy("/home/app_sema", destFilePath, NAME_LENGTH - 1);
	cp(driver, srcFilePath, destFilePath);

	stringCpy(argv[3], srcFilePath, NAME_LENGTH - 1);
	stringCpy("/home/app_print", destFilePath, NAME_LENGTH - 1);
	cp(driver, srcFilePath, destFilePath);

	stringCpy(argv[4], srcFilePath, NAME_LENGTH - 1);
	stringCpy("/home/write_alpha", destFilePath, NAME_LENGTH - 1);
	cp(driver, srcFilePath, destFilePath);

	stringCpy(argv[5], srcFilePath, NAME_LENGTH - 1);
	stringCpy("/home/argv", destFilePath, NAME_LENGTH - 1);
	cp(driver, srcFilePath, destFilePath);

	stringCpy(argv[6], srcFilePath, NAME_LENGTH - 1);
	stringCpy("/home/philosopher", destFilePath, NAME_LENGTH - 1);
	cp(driver, srcFilePath, destFilePath);

	stringCpy(argv[7], srcFilePath, NAME_LENGTH - 1);
	stringCpy("/home/fork_write", destFilePath, NAME_LENGTH - 1);
	cp(driver, srcFilePath, destFilePath);

	stringCpy(argv[8], srcFilePath, NAME_LENGTH - 1);
	stringCpy("/usr/bin/stat", destFilePath, NAME_LENGTH - 1);
	cp(driver, srcFilePath, destFilePath);

	stringCpy(argv[9], srcFilePath, NAME_LENGTH - 1);
	stringCpy("/usr/bin/cp", destFilePath, NAME_LENGTH - 1);
	cp(driver, srcFilePath, destFilePath);

	stringCpy(argv[10], srcFilePath, NAME_LENGTH - 1);
	stringCpy("/usr/bin/ls", destFilePath, NAME_LENGTH - 1);
	cp(driver, srcFilePath, destFilePath);

	stringCpy(argv[11], srcFilePath, NAME_LENGTH - 1);
	stringCpy("/usr/bin/cat", destFilePath, NAME_LENGTH - 1);
	cp(driver, srcFilePath, destFilePath);

	stringCpy(argv[12], srcFilePath, NAME_LENGTH - 1);
	stringCpy("/usr/bin/mkdir", destFilePath, NAME_LENGTH - 1);
	cp(driver, srcFilePath, destFilePath);

	stringCpy(argv[13], srcFilePath, NAME_LENGTH - 1);
	stringCpy("/usr/bin/rm", destFilePath, NAME_LENGTH - 1);
	cp(driver, srcFilePath, destFilePath);

	stringCpy(argv[14], srcFilePath, NAME_LENGTH - 1);
	stringCpy("/usr/bin/touch", destFilePath, NAME_LENGTH - 1);
	cp(driver, srcFilePath, destFilePath);

	stringCpy(argv[15], srcFilePath, NAME_LENGTH - 1);
	stringCpy("/usr/bin/rmdir", destFilePath, NAME_LENGTH - 1);
	cp(driver, srcFilePath, destFilePath);

	stringCpy(argv[16], srcFilePath, NAME_LENGTH - 1);
	stringCpy("/usr/bin/mv", destFilePath, NAME_LENGTH - 1);
	cp(driver, srcFilePath, destFilePath);

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