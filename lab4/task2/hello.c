#include <stdio.h>
#include <unistd.h>

int main() {
	printf("pid: %d\n", getpid());
	sleep(1);
	execl("./hello", "./hello", NULL);
	printf("Hello world!\n");

	return 0;
}

