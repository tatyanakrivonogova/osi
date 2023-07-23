#include <stdio.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#define SIZE 128

int main(int apgc, char** argv) {
	int sh_fd;
	void* sh_mem;
	char* buff;
	char val = 0;

	sh_fd = open("./shared-file", O_RDWR | O_CREAT, 0660);

	sh_mem = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, sh_fd, 0);
	close(sh_fd);

	buff = (char*) sh_mem;

	while (1) {
		for (int i = 0; i < SIZE / sizeof(val); ++i) {
			printf("%d ", buff[i]);
			fflush(stdout);
			if (buff[i] != val) printf("Failure! the numbers are out of order\n");
			++val;
			//sleep(1);
		}
	}
	if (munmap(sh_mem, SIZE) == -1) {
		perror("Failed mmap\n");
	}
	return 0;
}

