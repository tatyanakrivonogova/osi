#include <stdio.h>
#include <stdlib.h>
#include <linux/sched.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define STACK_SIZE 5*1024

void new_func(int depth) {
        char array[] = {'h','e','l','l','o',' ','w','o','r','l','d'};
	printf("%d\n", depth);
	if (depth <= 10) new_func(depth+1);
}

int child_func(void* arg) {
	printf("new_func address: %p\n", child_func);
	int depth = 1;
	new_func(depth);

	return 0;
}


int main() {
	int child_pid;
	int a = 5;
	void* child_stack;

	printf("parent: pid %d ppid %d\n", getpid(), getppid());

	const char *filepath = "stack1.txt";
        int fd = open(filepath, O_RDWR | O_CREAT, 0660);
        if (fd < 0){
                printf("Failed to open file %s\n", filepath);
                exit(-1);
        }

	ftruncate(fd, 0);
        ftruncate(fd, STACK_SIZE);

        child_stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (child_stack == MAP_FAILED) {
		printf("mmap failed: '%s'\n", strerror(errno));
		return -1;
	}
	close(fd);
	//memset(child_stack, 0xCC, STACK_SIZE);

	printf("child stack: %p\n", child_stack);

	child_pid = clone(child_func, child_stack+STACK_SIZE, CLONE_VM | SIGCHLD, NULL);
	if (child_pid == -1) {
		printf("clone failed\n");
		exit(-1);
	}

	sleep(5);
	int status;
	wait(&status);
	printf("Status: %d\n", status);
	int result = munmap(child_stack, STACK_SIZE);
	if (result != 0) {
                perror("Unmap error\n");
                exit(-1);
        }
	return 0;
}
