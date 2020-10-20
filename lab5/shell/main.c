#include "lib.h"
#include "types.h"
#define MAX_ARG 10
#define MAX_ARG_LEN 50


char **initArgv();
void showTitle();
int parseInput(char **argv);
void printErr(int errno);
void do_exec(char **argv);
void do_cmd(char **argv);
void do_cd(int argc, char **argv);
bool check_reopen(int argc, char **argv);

int uEntry(void) {
	chdir("/home");
	char **argv = initArgv();
	char **exec_argv = (char **)malloc(sizeof(char *) * (MAX_ARG + 1));
	int argc;
	int stdout_cp = dup(STD_OUT);
	int reopen = -1;

	while (1) {
		dup2(stdout_cp, STD_OUT);

		showTitle();
		argc = parseInput(argv);
		if (argc == 0 || argv[0][0] == '\n') continue;

		if (check_reopen(argc, argv)) {
			reopen = open(argv[argc - 1], O_CREATE|O_TRUNC|O_WRITE);
			if (reopen > 0)
				dup2(reopen, STD_OUT);
			else {
				printf("cannot redirect to '%s': ", argv[argc - 1]);
				printErr(reopen);
			}
			argc -= 2;
		}
		for (int i = 0; i < argc; i++)
			exec_argv[i] = argv[i];
		exec_argv[argc] = NULL;

		if (strcmp("cd", argv[0]) == 0) {
			do_cd(argc, exec_argv);
			continue;
		}
		else if (strcmp("clear", argv[0]) == 0 && argc == 1) {
			clear();
		}
		else if (strncmp("./", argv[0], 2) == 0) {
			strcpy(argv[0], argv[0] + 2);
			do_exec(exec_argv);
		}
		else if (argv[0][0] != '/') {
			do_cmd(exec_argv);
		}
		else {
			do_exec(exec_argv);
		}
		if (reopen > 0)
			close(reopen);
	}
	exit();
	return 0;
}

char **initArgv() {
	char **argv = (char **)malloc(sizeof(char *) * MAX_ARG);
	for (int i = 0; i < MAX_ARG; i++)
		argv[i] = (char *)malloc(sizeof(char) * MAX_ARG_LEN);
	return argv;
}

void showTitle() {
	char cwd[1000];
	int ret = getcwd(cwd, 1000);
	if (ret < 0)
		chdir("/");
	getcwd(cwd, 1000);	
	printf("\033[10mos2020:");
	printf("\033[9m%s", cwd);
	printf("$ ");
}

int parseInput(char **argv) {
	char line[1000];
	getline(line, 1000);
	if (line[0] == 0)
		return 0;
	char *tok = strtok(line, " ");
	int cnt = 0;
	while (tok) {
		strncpy(argv[cnt], tok, MAX_ARG_LEN);
		argv[cnt][MAX_ARG_LEN - 1] = '\0';
		cnt++;
		if (cnt == MAX_ARG)
			break;
		tok = strtok(NULL, " ");
	}
	return cnt;
}

bool check_reopen(int argc, char **argv) {
	if (argc >= 2 && strcmp(argv[argc - 2], ">") == 0) {
		return true;
	}
	return false;
}

void do_exec(char **argv) {
	int pid;
	if ((pid = fork()) == 0) {
		if (exec(argv[0], argv) == -1) {
			printf("%s: No such file or directory\n", argv[0]);
		}
	}
	else {
		waitpid(pid);
	}
}

void do_cmd(char **argv) {
	int pid;
	char tmp[1000];
	strncpy(tmp, "/usr/bin/", 10);
	strcat(tmp, argv[0]);
	if ((pid = fork()) == 0) {
		if (exec(tmp, argv) == -1) {
			printf("%s: Command not found\n", argv[0]);
		}
	}
	else {
		waitpid(pid);
	}
}

void printErr(int errno) {
	switch (errno) {
		case -1:
			printf("No such file or directory");
			break;
		case -3:
			printf("File exists");
			break;
		case -4:
			printf("Not a directory");
			break;
		case -5:
			printf("Is a directory");
			break;
		case -12:
			printf("Bad file type");
			break;
		default:
			printf("%d", errno);
			break;
	}
	printf("\n");
}

void do_cd(int argc, char **argv) {
	if (argc > 2) {
		printf("cd: too many arguments\n");
	}
	else if (argc == 1) {
		chdir("/");
	}
	else {
		int ret = chdir(argv[1]);
		if (ret < 0) {
			printf("cd: %s: ", argv[1]);
			printErr(ret);
		}
	}
}