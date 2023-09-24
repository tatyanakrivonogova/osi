#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

void *mythread1(void *arg) {
	printf("mythread1 [%d %d %d]: Hello from mythread1!\n", getpid(), getppid(), gettid());
	return (void*)42;
}

void *mythread2(void *arg) {
        printf("mythread2 [%d %d %d]: Hello from mythread2!\n", getpid(), getppid(), gettid());
	
	int length = sizeof("Hello world!");
        char* string = malloc(length);
	strncpy(string, "Hello world!", length);
	return (void*) string;
}

int main() {
	pthread_t tid1;
	pthread_t tid2;
	int err;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

	err = pthread_create(&tid1, NULL, mythread1, NULL);
	if (err) {
	    printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}

	err = pthread_create(&tid2, NULL, mythread2, NULL);
        if (err) {
            printf("main: pthread_create() failed: %s\n", strerror(err));
                return -1;
        }

	int retval;
	err = pthread_join(tid1, (void**) &retval);
	if (err != 0) {
		printf("main: pthread_join() failed: %s\n", strerror(err));
                return -1;
	}
	printf("retval for thread1: %d\n", retval);

	void* ret_string;
	err = pthread_join(tid2, &ret_string);
	if (err != 0) {
                printf("main: pthread_join() failed: %s\n", strerror(err));
                return -1;
        }
	char* string = (char*) ret_string;
        printf("retval for thread2: %s\n", string);
	free(string);

	return 0;
}

