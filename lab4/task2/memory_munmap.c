#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

int main() {

	printf("pid: %d\n", getpid());
	sleep(20);

	size_t pagesize = getpagesize();
	printf("pagesize: %ld \n", pagesize);
	
	printf("\nmmaping...\n");
	char* new_region = mmap(0, pagesize*10, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0 );
	
	if (new_region == MAP_FAILED) {
		perror("Failed while mmap");
		return -1;
	}
	printf("new region: %p\n", new_region);
/*
	sleep(10);
	printf("\nreading...\n");
	for (size_t i = 0; i < 10; ++i) {
                printf("*%d ", (int) new_region[i]);
        }
	
	printf("\nwriting...\n");
	for (size_t i = 0; i < 10; ++i) {
		new_region[i] = (char) i;
	}

	printf("\nreading...\n");
        for (size_t i = 0; i < 10; ++i) {
                printf("*%d ", (int) new_region[i]);
        }
*/
	sleep(20);

	printf("\nunmaping...\n");
	if (munmap(new_region+pagesize*3, pagesize*3) != 0) {
		perror("Failed while munmap");
	}
	sleep(20);	

	return 0;
}

