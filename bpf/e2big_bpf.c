#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <linux/bpf.h>
#include <sys/syscall.h>
#include <assert.h>
#include <stdlib.h>

#include "bpf_insn.h"

static __u64 ptr_to_u64(void *ptr)
{
        return (__u64) (unsigned long) ptr;
}

long bpf(int cmd, union bpf_attr *attr, size_t size)
{
	return syscall(__NR_bpf, cmd, attr, size);
}

#define BIG_BUF_SIZE (sizeof(union bpf_attr) + 1)
int main(void) {
	char *attr_buf;
	long ret;

	attr_buf = malloc(BIG_BUF_SIZE);
	assert(attr_buf != NULL);
	memset(attr_buf, 0, BIG_BUF_SIZE);

	ret = bpf(BPF_PROG_LOAD, (union bpf_attr *)attr_buf, BIG_BUF_SIZE);
	perror("bpf");
	assert(ret < 0);
	// we should actually get to the bpf syscall and it doesn't like our
	// bogus union bpf_attr
	assert(errno == EINVAL);
}
