#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/mount.h>
#include <fcntl.h>

#define MB (1024L * 1024L)
#define GB (MB * 1024L)

#define xstr(s) str(s)
#define str(s) #s

int main(void)
{
	int disk, ret, fd;
	struct statvfs sb;
	char cmd[4096];

	disk = open("foo", O_CREAT | O_WRONLY, 666);
	if (fd < 0) {
		perror("open(foo)");
		return 1;
	}

	ret = 1;
	if (ftruncate(disk, 1 * GB) < 0) {
		perror("failed to set disk size");
		close(fd);
		goto out_rm;
	}

	if (system("mkfs.xfs -m reflink=1 -i projid32bit=1 -L data foo")) {
		printf("mkfs failed\n");
		goto out_rm;
	}

	if (mkdir("bar", 0755) < 0) {
		perror("mkdir bar");
		goto out_rm;
	}

	// mount foo to bar with prjquota flag
	if (system("mount -o loop,attr2,prjquota foo bar")) {
		printf("mount failed\n");
		goto out_rm;
	}

	// create a dir and set large quota on it
	if (mkdir("bar/proj", 0755) < 0) {
		perror("mkdir proj");
		goto out_umount;
	}

	if (system("xfs_quota -x -c 'project -s -p bar/proj 100' bar")) {
		printf("failed to initialize xfs project\n");
		goto out_umount;
	}

#define BLOCK_LIMIT (2 * GB)
	printf("block limit %ld\n", BLOCK_LIMIT);
	snprintf(cmd, sizeof(cmd), "xfs_quota -x -c 'limit -p bsoft=%ld bhard=%ld 100' bar", BLOCK_LIMIT, BLOCK_LIMIT);
	if (system(cmd)) {
		printf("failed to initialize xfs project\n");
		goto out_umount;
	}

	// write some random data to the parent
	if (system("dd if=/dev/random of=bar/data bs=1M count=700")) {
		printf("writing random data failed\n");
		goto out_umount;
	}

	// write some random data inside the quota
	if (system("dd if=/dev/random of=bar/proj/data bs=1M count=100")) {
		printf("writing random data failed\n");
		goto out_umount;
	}

	// statvfs the project dir and see that a small amount of data is used
	if (statvfs("bar/proj", &sb) < 0) {
		perror("statvfs");
		goto out_umount;
	}

	printf("statvfs before resize, free space %luMB\n", sb.f_bfree * sb.f_bsize / MB);
	system("df -h bar/proj");
	system("df -h bar");

	// resize the disk
	if (ftruncate(disk, 10 * GB) < 0) {
		perror("failed to resize disk");
		goto out_umount;
	}

	if (system("losetup -c /dev/loop0")) {
		printf("failed to re-read loop size");
		goto out_umount;
	}

	// grow the fs
	if (system("xfs_growfs bar")) {
		printf("xfs growfs failed");
		goto out_umount;
	}

	// statvfs to see that there is no space used now
	if (statvfs("bar/proj", &sb) < 0) {
		perror("statvfs");
		goto out_umount;
	}

	printf("statvfs after resize, free space %luMB\n", sb.f_bfree * sb.f_bsize / MB);
	system("df -h bar/proj");
	system("df -h bar");
	ret = 0;

out_umount:
	if (umount("bar") < 0)
		perror("failed to umount(bar)");
out_rm:
	if (unlink("foo") < 0)
		perror("failed to unlink foo");
	if (rmdir("bar") < 0 && errno != ENOENT)
		perror("failed to rmdir bar");
	return ret;
}
