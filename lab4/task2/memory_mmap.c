#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

int main() {

	printf("pid: %d\n", getpid());

	size_t pagesize = getpagesize();
	printf("pagesize: %ld \n", pagesize);
	
	char* new_region = mmap(0, pagesize*10, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0 );
	
	if (new_region == MAP_FAILED) {
		perror("Failed fo mmap");
		return -1;
	}
	printf("new region: %p\n", new_region);
	
	sleep(20);

	mprotect(new_region, 10, PROT_WRITE);
	printf("\nreading...\n");
	for (size_t i = 0; i < 10; ++i) {
                printf("*%d ", (int) new_region[i]);
        }
	sleep(20);
	mprotect(new_region, 10, PROT_READ);
	printf("\nwriting...\n");
	for (size_t i = 0; i < 10; ++i) {
		new_region[i] = (char) i;
	}
	sleep(10);

	return 0;
}

