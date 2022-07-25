#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#ifndef SO_SETNETNS
#define SO_SETNETNS            76
#endif

int main(void)
{
	int oldnetns, newnetns, sock;
	socklen_t optlen = sizeof(newnetns);

	oldnetns = open("/proc/self/ns/net", O_RDONLY);
	if (oldnetns < 0) {
		perror("open");
		return 1;
	}

	if (unshare(CLONE_NEWNET)) {
		perror("unshare");
		return 1;
	}

	newnetns = open("/proc/self/ns/net", O_RDONLY);
	if (newnetns < 0) {
		perror("open");
		return 1;
	}

	if (setns(oldnetns, CLONE_NEWNET)) {
		perror("setns");
		return 1;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		return 1;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_SETNETNS, &newnetns, optlen)) {
		perror("setsockopt");
		return 1;
	}

	printf("OK!\n");

	return 0;
}
