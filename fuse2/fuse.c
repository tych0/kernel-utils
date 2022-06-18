#define FUSE_USE_VERSION 31
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>

static int sleepy_open(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

static void *nested_fsync(void *)
{
	int fd;

	fd = open("/home/tycho/packages/kernel-utils/fuse2/mount/b", O_RDONLY);
	write(fd, "foo", 3);
	printf("doing nested fsync\n");
	fsync(fd);
	printf("nested fsync completed\n");
	return NULL;
}

static int sleepy_write(const char *path, const char *buf, size_t size,
			off_t offset, struct fuse_file_info *fi)
{
	int fd;
	pthread_t t;

	pthread_create(&t, NULL, nested_fsync, NULL);

	return 0;
}

static int sleepy_flush(const char *path, struct fuse_file_info *fi)
{
	printf("eating a flush(%s)\n", path);
	while (1) sleep(100);
	return 0;
}

static int sleepy_opendir(const char *path, struct fuse_file_info *fi)
{
	printf("opendir for %s\n", path);
	return 0;
}

static int sleepy_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			  off_t offset, struct fuse_file_info *fi,
			  enum fuse_readdir_flags)

{
	if (strcmp(path, "/") != 0)
		return -ENOENT;

	if (filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS))
		return -ENOMEM;
	if (filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS))
		return -ENOMEM;
	if (filler(buf, "a", NULL, 0, FUSE_FILL_DIR_PLUS))
		return -ENOMEM;
	if (filler(buf, "b", NULL, 0, FUSE_FILL_DIR_PLUS))
		return -ENOMEM;

	return 0;
}

static int sleepy_getattr(const char *path, struct stat *sb, struct fuse_file_info *fi)
{
	struct timespec now;

	if (clock_gettime(CLOCK_REALTIME, &now) < 0)
		return -EINVAL;
	sb->st_uid = sb->st_gid = 0;
	sb->st_atim = sb->st_mtim = sb->st_ctim = now;
	sb->st_size = 0;

	if (strcmp(path, "/") == 0) {
		sb->st_mode = S_IFDIR | 00755;
		sb->st_nlink = 2;
	} else if (strcmp(path, "/a") == 0 || strcmp(path, "/b") == 0) {
		sb->st_nlink = 1;
		sb->st_mode = S_IFREG | 00755;
	} else {
		return -ENOENT;
	}

	return 0;
}

static void *sleepy_init(struct fuse_conn_info *info, struct fuse_config *config)
{
	config->intr = 0;
	return NULL;
}

const struct fuse_operations sleepy_fuse = {
	.open = sleepy_open,
	.opendir = sleepy_opendir,
	.readdir = sleepy_readdir,
	.getattr = sleepy_getattr,
	.write = sleepy_write,
	.flush = sleepy_flush,
	.init = sleepy_init,
};

int main(int argc, char *argv[])
{
	if (fuse_main(argc, argv, &sleepy_fuse, NULL)) {
		printf("fuse_main failed\n");
		exit(1);
	}

	printf("fuse main exited\n");
	exit(1);
}
