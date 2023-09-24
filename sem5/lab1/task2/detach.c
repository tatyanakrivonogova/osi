#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *arg) {
	printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
	pthread_detach(pthread_self());
	return NULL;
}

int main() {
	pthread_t tid;
	int err;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());
	
	int index = 1;
	while (1) {
		printf("%d ", index);
		++index;
		err = pthread_create(&tid, NULL, mythread, NULL);
		if (err) {
			printf("main: pthread_create() failed: %s\n", strerror(err));
			return -1;
		}
		/*err = pthread_join(tid, NULL);
		if (err != 0) {
			printf("main: pthread_join() failed: %s\n", strerror(err));
                        return -1;
                }*/
	}
	pthread_exit(0);
	return 0;
}

