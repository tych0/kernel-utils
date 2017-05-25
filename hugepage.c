/* This program uses exactly one 2MB page. Mostly for use with testing mm/ and
 * its interactions with hugepages.
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#define MAP_HUGE_2MB    (21 << MAP_HUGE_SHIFT)
int main()
{
	char *mem;
	int fd, size = getpagesize(), flags;

	fd = open("/proc/self/exe", O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_HUGE_2MB;

	mem = mmap(NULL, size * 20, PROT_WRITE | PROT_READ, flags, fd, 0);
	if (mem == MAP_FAILED) {
		perror("mmap");
		return 1;
	}

	/* let's use the page */
	mem[0] = 0;
	system("grep -i Hugepage /proc/meminfo");
	munmap(mem, size * 20);

	return 0;
}
