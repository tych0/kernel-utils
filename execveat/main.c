#define _GNU_SOURCE
#include <stdio.h>
#include <syscall.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>

#ifndef AT_EMPTY_PATH
#define AT_EMPTY_PATH                        0x1000  /* Allow empty relative */
#endif

#ifndef AT_EXEC_REASONABLE_COMM
#define AT_EXEC_REASONABLE_COMM         0x200
#endif

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	bool wants_reasonable_comm = argc > 1;

	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	}

	if (pid == 0) {
		int fd;
		long ret, flags;

		fd = open("./catprocselfcomm", O_PATH);
		if (fd < 0) {
			perror("open catprocselfname");
			exit(1);
		}

		flags = AT_EMPTY_PATH;
		if (wants_reasonable_comm)
			flags |= AT_EXEC_REASONABLE_COMM;
		syscall(__NR_execveat, fd, "", (char *[]){"./catprocselfcomm", NULL}, NULL, flags);
		fprintf(stderr, "execveat failed %d\n", errno);
		exit(1);
	}

	if (waitpid(pid, &status, 0) != pid) {
		fprintf(stderr, "wrong child\n");
		exit(1);
	}

	if (!WIFEXITED(status)) {
		fprintf(stderr, "exit status %x\n", status);
		exit(1);
	}

	if (WEXITSTATUS(status) != 0) {
		fprintf(stderr, "child failed\n");
		exit(1);
	}

	return 0;
}
