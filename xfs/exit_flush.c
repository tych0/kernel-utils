#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <asm-generic/ioctl.h>

#define MB (1024L * 1024L)
#define GB (MB * 1024L)

#define FS_IOC_TYCHO           _IOW ('X', 107, uint32_t)

int main(void)
{
	int disk, ret = 1, fd, sk_pair[2], dir, status, child;
	struct statvfs sb;
	char c = 'c';
	bool fail = false;
	unsigned long used;

	disk = open("/tmp/foo", O_CREAT | O_WRONLY, 666);
	if (fd < 0) {
		perror("open(foo)");
		return 1;
	}

	if (socketpair(PF_LOCAL, SOCK_SEQPACKET, 0, sk_pair) < 0) {
		perror("socketpair");
		return 1;
	}

	ret = 1;
	if (ftruncate(disk, 1 * GB) < 0) {
		perror("failed to set disk size");
		close(fd);
		goto out_rm;
	}

	if (system("mkfs.xfs -m reflink=1 -i projid32bit=1 -L data /tmp/foo")) {
		printf("mkfs failed\n");
		goto out_rm;
	}

	if (mkdir("bar", 0755) < 0) {
		perror("mkdir bar");
		goto out_rm;
	}

	if (system("mount -o loop,attr2 /tmp/foo bar")) {
		printf("mount failed\n");
		goto out_rm;
	}

	// make a dir
	if (mkdir("bar/dir", 0755) < 0) {
		perror("mkdir overlay/foo");
		goto out_umount_overlay;
	}

	dir = open("bar/dir", O_DIRECTORY);
	if (dir < 0) {
		perror("open directory from child");
		goto out_umount_overlay;
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		goto out_umount_overlay;
	}

	if (!child) {
		int fd;
		struct stat sb;

		close(sk_pair[0]);

		// create a file and unlink it, but don't close it
		fd = openat(dir, "baz", O_CREAT|O_RDWR);
		if (fd < 0) {
			perror("openat(baz)");
			exit(1);
		}

		if (write(fd, "hello", 5) != 5) {
			perror("write(hello)");
			exit(1);
		}
		if (fstat(fd, &sb) < 0) {
			perror("fstat");
			exit(1);
		}
		printf("data inode: %lu\n", sb.st_ino);

		if (unlinkat(dir, "baz", 0) < 0) {
			perror("unlinkat(baz)");
			exit(1);
		}

		printf("notifying parent...\n");

		// tell the parent we are done fiddling with the fs
		if (write(sk_pair[1], &c, 1) != 1) {
			perror("write to parent");
			exit(1);
		}
		printf("notified parent, sleeping\n");

		printf("child exiting...\n");
		exit(0);
	}

	close(sk_pair[1]);

	if (read(sk_pair[0], &c, 1) != 1) {
		perror("read from child");
		goto out_umount_overlay;
	}

	if (waitpid(child, &status, 0) != child) {
		fprintf(stderr, "got weird wait\n");
		goto out_umount_overlay;
	}

	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		fprintf(stderr, "child exited with %x\n", status);
		goto out_umount_overlay;
	}

	ret = 0;

out_umount_overlay:
	kill(child, SIGTERM);
	close(dir);
out_umount:
	if (umount("bar") < 0)
		perror("failed to umount(bar)");
out_rm:
	if (unlink("/tmp/foo") < 0)
		perror("failed to unlink foo");
	if (rmdir("bar") < 0 && errno != ENOENT)
		perror("failed to rmdir bar");
	return ret;
}
