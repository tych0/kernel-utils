#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
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

#ifndef PTRACE_SECCOMP_GET_LISTENER
#define PTRACE_SECCOMP_GET_LISTENER 0x420d

#define SECCOMP_RET_USER_NOTIF 0x7fc00000U
struct seccomp_notif {
	__u32 id;
	pid_t pid;
	struct seccomp_data data;
};

struct seccomp_notif_resp {
	__u32 id;
	int error;
	long val;
};
#endif

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

	if (syscall(__NR_seccomp, SECCOMP_SET_MODE_FILTER, 0, &bpf_prog) < 0) {
		perror("prctl failed");
		return -1;
	}

	return 0;
}

static int get_seccomp_listener(pid_t pid)
{
	long ret = -1, ret2;

	if (ptrace(PTRACE_ATTACH, pid) < 0) {
		perror("ptrace attach failed");
		return -1;
	}

	if (waitpid(pid, NULL, 0) != pid) {
		perror("waitpid");
		goto out;
	}

	ret = ptrace(PTRACE_SECCOMP_GET_LISTENER, pid, 0);
	if (ret < 0) {
		perror("ptrace get listener failed");
		goto out;
	}

	ret2 = ptrace(PTRACE_SECCOMP_GET_LISTENER, pid, 0);
	if (ret2 >= 0 || errno != EBUSY) {
		perror("getting second listener succeded?");
		close(ret);
		close(ret2);
		goto out;
	}

out:
	if (ptrace(PTRACE_DETACH, pid, NULL, 0) < 0) {
		perror("ptrace detach failed");
	}
	return ret;
}

#define MAGIC 116983961184613L
static int respond_with_magic(int listener, int syscall)
{
	struct seccomp_notif req;
	struct seccomp_notif_resp resp;
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
		resp.val = MAGIC;
	}

	printf("parent: got notification, responding with %ld\n", resp.val);

	ret = write(listener, &resp, sizeof(resp));
	if (ret < 0) {
		perror("write");
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	pid_t pid;
	int sk_pair[2], sk, listener, listener2, ret, status;
	char c = 'B';

	if (socketpair(PF_LOCAL, SOCK_SEQPACKET, 0, sk_pair)) {
		perror("socketpair");
		return -1;
	}

	pid = fork();
	if (pid < 0) {
		perror("fork");
		return -1;
	}

	if (pid == 0) {
		long result;

		sk = sk_pair[1];
		close(sk_pair[0]);

		if (filter_syscall(__NR_getpid) < 0)
			_exit(1);

		printf("child: installed filter\n");

		if (syscall(__NR_getpid) > 0) {
			printf("child: successfully got pid?\n");
			_exit(1);
		} else if (errno != ENOSYS) {
			perror("child: getpid failed with not ENOSYS");
			_exit(1);
		} else {
			printf("child: getpid got ENOSYS without listener\n");
		}

		printf("child: writing success\n");
		if (write(sk, &c, 1) != 1) {
			perror("write");
			_exit(1);
		}

		if (read(sk, &c, 1) != 1) {
			perror("read");
			_exit(1);
		}
		printf("child: read success\n");

		errno = 0;
		result = syscall(__NR_getpid);
		if (result < 0) {
			perror("child: filtered getpid\n");
			_exit(1);
		} else if (result != MAGIC) {
			int err = errno;

			printf("child: huh? got return value %ld (errno: %d %s)\n", result, err, strerror(err));
			_exit(1);
		} else {
			printf("child: got MAGIC from getpid()\n");
		}

		_exit(0);
	}

	sk = sk_pair[0];
	close(sk_pair[1]);

	if (read(sk, &c, 1) != 1) {
		perror("read");
		goto err;
	}
	printf("parent: child installed filter correctly\n");

	printf("parent: getting listener for %d\n", pid);
	listener = get_seccomp_listener(pid);
	if (listener < 0) {
		fprintf(stderr, "getting listener failed\n");
		goto err;
	}

	printf("parent: got listener, telling child to execute\n");
	if (write(sk, &c, 1) != 1) {
		perror("write");
		_exit(1);
	}

	if (respond_with_magic(listener, __NR_getpid) < 0) {
		fprintf(stderr, "respond failed\n");
		goto err;
	}

	if (waitpid(pid, &status, 0) != pid) {
		perror("waitpid");
		goto err;
	}

	ret = 0;
	if (!WIFEXITED(status) || WEXITSTATUS(status)) {
		fprintf(stderr, "exited incorrectly: %d\n", status);
		ret = 1;
	}
err:
	kill(pid, SIGKILL);
	return ret;
}
