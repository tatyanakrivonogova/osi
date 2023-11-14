#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
#include <sched.h>

#include "list.h"

#define LIST_SIZE 10

void set_cpu(int n) {
	int err;
	cpu_set_t cpuset;
	pthread_t tid = pthread_self();

	CPU_ZERO(&cpuset);
	CPU_SET(n, &cpuset);

	err = pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset);
	if (err) {
		printf("set_cpu: pthread_setaffinity failed for cpu %d\n", n);
		return;
	}

	printf("set_cpu: set cpu %d\n", n);
}

void print_list(list_t *l) {
	int err;
	for (int i = 0; i < LIST_SIZE; ++i) {
                char* current;
                err = list_read(l, i, &current);
                if (err == 0) {
                        printf("print_list: failed while reading of list\n");
                }
                printf("list[%d]: %s\n", i, current);
        }
	printf("\n\n");
}
/*
void *reader(void *arg) {
	int expected = 0;
	queue_t *q = (queue_t *)arg;
	printf("reader [%d %d %d]\n", getpid(), getppid(), gettid());

	set_cpu(1);

	while (1) {
		int val = -1;
		int ok = queue_get(q, &val);
		if (!ok)
			continue;

		if (expected != val)
			printf(RED"ERROR: get value is %d but expected - %d" NOCOLOR "\n", val, expected);

		expected = val + 1;
	}

	return NULL;
}
*/
void *swapper(void *arg) {
	int err;
	list_t *l = (list_t *) arg;
	printf("swapper [%d %d %d]\n", getpid(), getppid(), gettid());

	//set_cpu(2);

	while (1) {
		int index = rand() % (l->count - 1);
		err = swap(l, index);
        	if (err == 0) {
                	printf("swapper [%d %d %d]: failed while swap\n", getpid(), getppid(), gettid());
        	}
	}

	return NULL;
}


int main() {
	srand(time(NULL));
	list_t *l;
	int err;

	printf("main [%d %d %d]\n", getpid(), getppid(), gettid());


	l = list_init(LIST_SIZE);
	err = fill_list(l);
	if (err == 0) {
		printf("main: failed while filling the list\n");
		return 0;
	}

	//print_list(l);	

	pthread_t swapper1, swapper2, swapper3;
	err = pthread_create(&swapper1, NULL, swapper, l);
	if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}
	err = pthread_create(&swapper2, NULL, swapper, l);
        if (err) {
                printf("main: pthread_create() failed: %s\n", strerror(err));
                return -1;
        }
	err = pthread_create(&swapper3, NULL, swapper, l);
        if (err) {
                printf("main: pthread_create() failed: %s\n", strerror(err));
                return -1;
        }

	//sched_yield();
/*
	err = pthread_create(&tid2, NULL, writer, q);
	if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}

	int retval;
	err = pthread_join(tid1, (void**)&retval);
	if (err) {
		printf("main: pthread_join() failed: %s\n", strerror(err));
		return -1;
	}
	err = pthread_join(tid2, (void**)&retval);
        if (err) {
                printf("main: pthread_join() failed: %s\n", strerror(err));
                return -1;
        }
	*/
	pthread_exit(NULL);

	return 0;
}
