#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/mman.h>

void f() {
	char* pointer_buffer[100] = {0};
	for (size_t i = 0; i < 100; ++i) {
		printf("%ld\n", i);
		char* buffer = (char*)malloc(sizeof(char)*4096);
		pointer_buffer[i] = buffer;
		sleep(1);
	}

	for (size_t i = 0; i < 100; ++i) {
		if (pointer_buffer[i] != 0) free(pointer_buffer[i]);
	}

}

int main() {

	printf("pid: %d\n", getpid());
	sleep(10);
	f();

	return 0;
}

