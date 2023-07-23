#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/mman.h>
size_t i = 0;
void f() {
	printf("%ld\n", i);
	++i;
	char buffer[4096];
	sleep(1);
	f();
}

int main() {

	printf("pid: %d\n", getpid());
	sleep(10);
	f();

	return 0;
}

