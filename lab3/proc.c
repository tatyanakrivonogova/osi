#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <limits.h>

#define PM_ENTRY_BYTES		8
#define PM_PFRAME_BITS		55
#define PM_PFRAME_MASK		((1LL << PM_PFRAME_BITS) - 1)
#define PM_PFRAME(x)		((x) & PM_PFRAME_MASK)
#define MAX_SWAPFILES_SHIFT	5
#define PM_SWAP_OFFSET(x)	(((x) & PM_PFRAME_MASK) >> MAX_SWAPFILES_SHIFT)
#define PM_SOFT_DIRTY		(1ULL << 55)
#define PM_MMAP_EXCLUSIVE	(1ULL << 56)
#define PM_FILE			(1ULL << 61)
#define PM_SWAP			(1ULL << 62)
#define PM_PRESENT		(1ULL << 63)


int str2i(int *buf, char *str, int base) {
	char *endptr;

	errno = 0;
	*buf = (int) strtol(str, &endptr, base);
	if (errno == ERANGE || *endptr != '\0' || endptr == str) {
		return -1;
	}

	return 0;
}

int str2ul(unsigned long *buf, char *str, int base) {
	char *endptr;

	errno = 0;
	*buf = strtoul(str, &endptr, base);
	if (errno == ERANGE || *endptr != '\0' || endptr == str) {
		return -1;
	}

	return 0;
}


int read_pagemap(char *pmpath){
	printf("READ\n");
	FILE *fd;
	off_t offset;
	uint64_t read_val;
	int r;

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	printf("Big endian\n");
#else
	printf("Little endian\n");
#endif

	fd = fopen(pmpath, "rb");
	if (fd == NULL) {
		perror("fopen(3)");
		return -1;
	}
	
	fseek(fd, 0, SEEK_END);
        size_t size = ftell(fd);
        fseek(fd, 0, SEEK_SET);
	printf("size %ld\n", size);

	char ch;
	for (size_t i = 0; i < size; ++i) {
            ch = getc(fd);
            if (ch != EOF) {
                putchar(ch);
            }
        }
        putchar('\n');


	// Shifting by virt-addr-offset number of bytes
	// and multiplying by the size of an address (the size of an entry in pagemap file)
/*	offset = (vaddr / getpagesize()) * PM_ENTRY_BYTES;

	printf("Vaddr: 0x%lx, Page_size: %d, Entry_size: %d\n", vaddr, getpagesize(), PM_ENTRY_BYTES);
	printf("Reading %s at 0x%llx\n", pmpath, (unsigned long long) offset);

	r = fseek(fd, offset, SEEK_SET);
	if (r < 0) {
		perror("fseek(3)");
		return -1;
	}

	// TODO: Endianess?
	fread(&read_val, PM_ENTRY_BYTES, 1, fd);
	if (ferror(fd)) {
		perror("fread(3)");
		return -1;
	}

	printf("\n");
	printf("Result: 0x%llx\n", (unsigned long long) read_val);

	if (read_val & PM_PRESENT) {
		printf("PFN: 0x%llx\n", (unsigned long long) PM_PFRAME(read_val));
	} else {
		printf("Page not present\n");
	}

	if (read_val & PM_SWAP) {
		printf("Page swapped\n");
	}
*/
	fclose(fd);
	printf("READ\n");
	return 0;
}

int main(int argc, char **argv){
	char pmpath[PATH_MAX] = { 0 };
	pid_t pid;
	int r;
	printf("START\n");

	if (argc != 2) {
		fprintf(stderr, "usage: %s PID\n", argv[0]);
		goto fail;
	}

	if (strcmp(argv[1], "self") == 0) {
		pid = getpid();
	} else {
		r = str2i(&pid, argv[1], 10);
		if (r < 0 || pid < 0) {
			fprintf(stderr, "PID must be a positive number or 'self'\n");
			goto fail;
		}
	}
	printf("PID\n");
/*
	r = str2ul(&vaddr, argv[2], 16);
	if (r < 0) {
		fprintf(stderr, "Could not parse VADDR\n");
		goto fail;
	}
*/
	sprintf(pmpath, "/proc/%u/pagemap", pid);
	printf("BEFORE\n");
	r = read_pagemap(pmpath);
	/*
	if (r < 0) {
		goto fail;
	}
*/
	return EXIT_SUCCESS;
fail:
	return EXIT_FAILURE;
}
