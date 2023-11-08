#include <stdio.h>

int write_proc_tycho(int val)
{
	FILE *f;

	f = fopen("/proc/tycho", "w");
	if (!f) {
		perror("open /proc/tycho");
		return -1;
	}

	if (fprintf(f, "%d\n", val) < 0) {
		perror("write");
		return -1;
	}

	if (fclose(f) < 0) {
		perror("close");
		return -1;
	}

	return 0;
}
