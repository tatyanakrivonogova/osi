#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

void free_memory(void* arg) {
	free(arg);
    	printf("Memory is free\n");
}

void *mythread(void *arg) {
	char* str = malloc(sizeof("Hello world"));
	strncpy(str, "Hello world", sizeof("Hello world"));

	pthread_cleanup_push(free_memory, str);

	while (1) {
		printf("%s\n", str);
	}
	pthread_cleanup_pop(1);
	return NULL;
}

int main() {
	pthread_t tid;
	int err;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

	err = pthread_create(&tid, NULL, mythread, NULL);
	if (err) {
	    printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}
	sleep(3);
	pthread_cancel(tid);
	pthread_exit(0);
	return 0;
}
