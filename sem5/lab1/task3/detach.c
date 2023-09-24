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
	while (1) {
		printf("param.a: %d param.b: %s\n", param->a, param->b);
		param->a = param->a + 1;
		sleep(1);
	}
	return NULL;
}

int main() {
	pthread_t tid;
	int err;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

	struct parameter param;
	param.a = 8;
	param.b = "abc";

	pthread_attr_t attr;
        err = pthread_attr_init(&attr);
        if (err != 0) {
                printf("main: pthread_attr_init() failed: %s\n", strerror(err));
                return -1;
        }
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	err = pthread_create(&tid, &attr, mythread, &param);
	if (err) {
	    printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}
	printf("main thread is exiting\n");
	pthread_exit(0);
	return 0;
}

