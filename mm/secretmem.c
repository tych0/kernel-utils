#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#ifndef MFD_SECRET
#define MFD_SECRET             0x0008U
#endif

#define MFD_SECRET_IOCTL '-'
#define MFD_SECRET_EXCLUSIVE   _IOW(MFD_SECRET_IOCTL, 0x13, unsigned long)
#define MFD_SECRET_UNCACHED    _IOW(MFD_SECRET_IOCTL, 0x14, unsigned long)

#define PAGE_SIZE 4096

int main()
{
	int fd, err;
	void *mapping;
	char *cmapping;

	fd = memfd_create("zomg", MFD_SECRET);
	if (fd < 0) {
		perror("memfd_create");
		exit(1);
	}

	if (ioctl(fd, MFD_SECRET_EXCLUSIVE) < 0) {
		perror("ioctl");
		exit(1);
	}

	mapping = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (mapping == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
	printf("mapping %p\n", mapping);
	cmapping = mapping;

	*cmapping = '1';

	close(fd);

	exit(0);
}
