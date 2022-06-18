#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

static void do_write(void)
{
	int fd;

	fd = open("mount/a", O_CREAT|O_WRONLY, 0600);
	if (fd < 0) {
		perror("open fuse file");
		exit(1);
	}

	errno = 0;
	write(fd, "hello", 5);
	printf("write returned, doing a flush to hang...\n");
	fsync(fd);
	printf("flush didn't hang?\n");
	exit(0);
}

int main(void)
{
	pid_t fuse_pid, write_pid;

	fuse_pid = fork();
	if (fuse_pid < 0) {
		perror("fuse fork");
		exit(1);
	}

	if (fuse_pid == 0) {
		execlp("./fuse", "./fuse", "-s", "-f", "mount", NULL);
		perror("exec");
		exit(1);
	}

	// lazy... let fuse get mounted
	sleep(2);

	write_pid = fork();
	if (write_pid < 0) {
		perror("getattr fork");
		exit(1);
	}

	if (write_pid == 0)
		do_write();

	// lazy let the write() and nested fsync() from fuse happen
	sleep(2);

	printf("killing pid ns...\n");
	return 0;
}
