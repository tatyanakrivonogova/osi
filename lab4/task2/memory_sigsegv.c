#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>

void handler(int signum) {
	write(2, "\nSignal intercepted\n", 20);
	signal(signum, SIG_DFL);
	exit(0);
}

int main() {

	signal(SIGSEGV, handler);

	printf("pid: %d\n", getpid());

	size_t pagesize = getpagesize();
	printf("pagesize: %ld \n", pagesize);
	
	char* new_region = mmap(0, pagesize*10, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0 );
	
	if (new_region == MAP_FAILED) {
		perror("Failed fo mmap");
		return -1;
	}
	printf("new region: %p\n", new_region);

        for (size_t i = 0; i < 10; ++i) {
                printf("*%d ", (int) new_region[i]);
        }
	printf("good\n");

	return 0;
}

