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

#define NUM_THREADS 10000
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
		perror("respond read");
		return -1;
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
		perror("write");
		return -1;
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

void *thread(void *arg)
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

int main(int argc, char **argv)
{
	pthread_t threads[NUM_THREADS];
	int i, ret = 1, listener = -1;
	bool failure = false;

	/* every thread has the same filter */
	listener = filter_syscall(__NR_open);
	if (listener < 0)
		return 1;

	/* spawn n threads using that filter */
	for (i = 0; i < NUM_THREADS; i++) {
		if (pthread_create(&threads[i], NULL, thread, &failure))
			goto out;
	}

	/* answer n queries from each thread */
	for (i = 0; i < NUM_THREADS * NUM_QUERIES; i++) {
		if (i % NUM_THREADS == 0) {
			printf(".");
			fflush(stdout);
		}
		if (failure || respond_with_pid(listener, __NR_open) < 0) {
			failure = true;
			break;
		}
	}
	printf("\n");

	if (!failure)
		printf("all responses sent\n");

	close(listener);
	listener = -1;

	/* make sure the threads die */
	for (i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	if (!failure)
		ret = 0;
out:
	if (listener >= 0)
		close(listener);
	exit(ret);
	return ret;
}
