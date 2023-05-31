#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/socket.h>

#define N 100

static void open_more_than_N_fds(char *name)
{
	int count, i;
	char buf[2048];

	sleep(1);
	for (i = 0; i < N+5; i++) {
		int fd;

		fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
		if (fd < 0) {
			fprintf(stderr, "%s %d socket: %m\n", name, i);
		}
	}
	sleep(10000000);
}

int main(void)
{
	struct rlimit l = {
		.rlim_cur = N,
		.rlim_max = N,
	};
	pid_t first = 0, second = 0;

	if (setrlimit(RLIMIT_NOFILE, &l) < 0) {
		perror("setrlimit");
		return 1;
	}

	first = fork();
	if (first < 0) {
		perror("fork");
		return 1;
	}
	if (first == 0)
		open_more_than_N_fds("first");

	second = fork();
	if (second < 0) {
		perror("fork2");
		goto kill_first;
	}
	if (second == 0)
		open_more_than_N_fds("second");

	sleep(100);

	kill(second, SIGKILL);
kill_first:
	kill(first, SIGKILL);
	return 0;
}
