#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(void)
{
	int fd;
	char buf[128];

	fd = open("/proc/self/comm", O_RDONLY);
	if (fd < 0) {
		perror("open comm");
		exit(1);
	}

	if (read(fd, buf, 128) < 0) {
		perror("read");
		exit(1);
	}

	printf("comm: %s", buf);
	exit(0);
}
