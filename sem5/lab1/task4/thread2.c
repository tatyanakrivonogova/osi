#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *arg) {
	int counter = 0;
	while (1) {
		//printf("%d Hello from mythread!\n", counter);
		counter++;
		pthread_testcancel();
		//sleep(1);
	}
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
	sleep(10);
	pthread_cancel(tid);
	pthread_exit(0);
	return 0;
}
