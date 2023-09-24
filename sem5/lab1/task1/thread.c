#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#define THREADS_NUMBER 5

int global = 5;

void *mythread(void *arg) {
	int local = 1;
	static int static_local = 2;
	const int const_local = 3;

	printf("mythread [%d %d %d %lu]: Hello from mythread!\n", getpid(), getppid(), gettid(), pthread_self());
	printf("\t\tlocal:		%d %p\n\t\tstatic_local: 	%d %p\n\t\tconst_local: 	%d %p\n\t\tglobal: 	%d %p\n", local, &local, static_local, &static_local, const_local, &const_local, global, &global);
	
	local = gettid();
	static_local = gettid();
	global = gettid();

	printf("\t\t*local:		%d %p\n\t\t*static_local: 	%d %p\n\t\t*const_local:	%d %p\n\t\t*global: 	%d %p\n", local, &local, static_local, &static_local, const_local, &const_local, global, &global);
	sleep(100);
	return NULL;
}

int main() {
	pthread_t tid[THREADS_NUMBER];
	int err;

	printf("main [%d %d %d %lu]: Hello from main!\n", getpid(), getppid(), gettid(), pthread_self());

	for (int i = 0; i < THREADS_NUMBER; ++i) {
		err = pthread_create(tid + i*sizeof(pthread_t), NULL, mythread, NULL);
		if (err) {
			printf("main: pthread_create() failed: %s\n", strerror(err));
			return -1;
		} else {
			printf("main: thread %lu was created\n", *(tid + i*sizeof(pthread_t)));
		}
	}
	sleep(100);
	pthread_exit(0);
	return 0;
}

