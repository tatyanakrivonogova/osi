#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

struct parameter {
	int a;
	char* b;
};

void *mythread(void *arg) {
	printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
	struct parameter* param = (struct parameter*) arg;
	printf("param.a: %d param.b: %s\n", param->a, param->b);
	return NULL;
}

int main() {
	pthread_t tid;
	int err;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

	struct parameter param;
	param.a = 8;
	param.b = "abc";


	err = pthread_create(&tid, NULL, mythread, &param);
	if (err) {
	    printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}
	pthread_exit(0);
	return 0;
}

