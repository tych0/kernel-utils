#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <linux/ptrace.h>
#include <pthread.h>

#define NUM_THREADS 1000
#define NUM_QUERIES 10000

#ifndef SECCOMP_FILTER_FLAG_GET_LISTENER
#define SECCOMP_FILTER_FLAG_GET_LISTENER 4

#define SECCOMP_RET_USER_NOTIF 0x7fc00000U

struct seccomp_notif {
	__u64 id;
	pid_t pid;
	struct seccomp_data data;
};

struct seccomp_notif_resp {
	__u64 id;
	__s32 error;
	__s64 val;
	__u8 return_fd;
	__u32 fd;
};
#endif

static int respond_with_pid(int listener, int syscall)
{
	struct seccomp_notif req;
	struct seccomp_notif_resp resp = {};
	ssize_t ret;

	ret = read(listener, &req, sizeof(req));
	if (ret < 0) {
		return ret;
	}

	resp.id = req.id;
	if (req.data.nr != syscall) {
		resp.error = -ENOSYS;
		resp.val = 0;
	} else {
		resp.error = 0;
		resp.val = req.pid;
	}

	ret = write(listener, &resp, sizeof(resp));
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int filter_syscall(int syscall_nr)
{
	struct sock_filter filter[] = {
		BPF_STMT(BPF_LD+BPF_W+BPF_ABS, offsetof(struct seccomp_data, nr)),
		BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, syscall_nr, 0, 1),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_USER_NOTIF),
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};

	struct sock_fprog bpf_prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};

	int ret;

	ret = syscall(__NR_seccomp, SECCOMP_SET_MODE_FILTER, SECCOMP_FILTER_FLAG_GET_LISTENER, &bpf_prog);
	if (ret < 0) {
		perror("prctl failed");
		return -1;
	}

	return ret;
}

void *tracee(void *arg)
{
	bool *failure = arg;
	pid_t pid = syscall(__NR_gettid);
	long ret;
	int i;

	/* invalid flags -- but we should get a valid result! */
	for (i = 0; i < NUM_QUERIES; i++) {
		ret = syscall(__NR_open, NULL, 0);
		if (ret != pid) {
			printf("got bad pid: %ld expected %d\n", ret, pid);
			*failure = true;
			break;
		}
	}
	return NULL;
}

struct responder_arg {
	bool *failure;
	int listener;
};

void *responder(void *arg)
{
	struct responder_arg *resp = arg;

	while (1) {
		int ret;

		ret = respond_with_pid(resp->listener, __NR_open);
		if (ret < 0) {
			if (errno != EBADF) {
				perror("couldn't respond with pid");
				*(resp->failure) = true;
				close(resp->listener);
			}
			break;
		}
	}

	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t tracees[NUM_THREADS], responders[NUM_THREADS];
	int i, ret = 1, listener = -1;
	bool failure = false;
	struct responder_arg responder_arg = {.failure = &failure};

	/* every thread has the same filter */
	listener = filter_syscall(__NR_open);
	if (listener < 0)
		return 1;
	responder_arg.listener = listener;

	/* spawn n threads using that filter */
	for (i = 0; i < NUM_THREADS; i++) {
		if (pthread_create(&responders[i], NULL, responder, &responder_arg))
			goto out;
	}

	for (i = 0; i < NUM_THREADS; i++) {
		if (pthread_create(&tracees[i], NULL, tracee, &failure))
			goto out;
	}

	/* make sure the tracees die */
	for (i = 0; i < NUM_THREADS; i++) {
		pthread_join(tracees[i], NULL);
	}

	if (!failure)
		printf("all responses sent\n");

	close(listener);
	listener = -1;

	/* and the tracers we just leave them and exit(). lazy. */

	if (!failure)
		ret = 0;
out:
	if (listener >= 0)
		close(listener);
	exit(ret);
	return ret;
}
