#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <linux/bpf.h>
#include <sys/syscall.h>
#include <assert.h>
#include <stdlib.h>

static __u64 ptr_to_u64(void *ptr)
{
        return (__u64) (unsigned long) ptr;
}

static long bpf(int cmd, union bpf_attr *attr, size_t size)
{
	return syscall(__NR_bpf, cmd, attr, size);
}

int main(void) {
	union bpf_attr attr;
	int ret;

	attr.pathname = ptr_to_u64("/sys/fs/bpf//tc/globals/ipv4_map");
	attr.bpf_fd = 0;
	ret = bpf(BPF_OBJ_GET, &attr, sizeof(attr));
	if (ret < 0) {
		perror("bpf");
		return 1;
	}
	return 0;
}
