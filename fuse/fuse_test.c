//#define FUSE_VERSION 26

#include <stdio.h>
#include <unistd.h>
#include <fuse.h>
#include <errno.h>
#include <sys/types.h>

static int fuse_test_getattr(const char *path, struct stat *sb)
{
	sleep(130);
	return -EBADSLT;
}

const struct fuse_operations fuse_test_ops = {
	.getattr = fuse_test_getattr,
};

int main(int argc, char *argv[])
{
	if (fuse_main(argc, argv, &fuse_test_ops)) {
		printf("fuse_main failed\n");
		return 1;
	}

	return 0;
}
