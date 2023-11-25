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

#define LIST_SIZE 100000

int swap_counter = 0;
pthread_mutex_t swap_counter_mutex;

int reader_greater_counter = 0;
int reader_less_counter = 0;
int reader_equal_counter = 0;


void *lmonitor(void *arg) {
	printf("lmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

	while (1) {
		//printf("lmonitor: swap counter = %d		reader greater counter = %d		reader less counter = %d\n", swap_counter, reader_greater_counter, reader_less_counter);
		printf("lmonitor: swap counter = %d		reader greater counter = %d		reader less counter = %d		reader equal counter = %d\n", swap_counter, reader_greater_counter, reader_less_counter, reader_equal_counter);
		sleep(1);
	}

	return NULL;
}

// void set_cpu(int n) {
// 	int err;
// 	cpu_set_t cpuset;
// 	pthread_t tid = pthread_self();

// 	CPU_ZERO(&cpuset);
// 	CPU_SET(n, &cpuset);

// 	err = pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset);
// 	if (err) {
// 		printf("set_cpu: pthread_setaffinity failed for cpu %d\n", n);
// 		return;
// 	}

// 	printf("set_cpu: set cpu %d\n", n);
// }



void *reader_greater(void *arg) {
	int err;
	int counter = 0;
	list_t *l = (list_t *)arg;
	printf("reader [%d %d %d]\n", getpid(), getppid(), gettid());

	//set_cpu(1);

	node_t *current_node;
	while (1) {
		counter = 0;
		int prev_length = -1;
		for (int i = 0; i < l->count; ++i) {
			err = list_get(l, i, &current_node);
			if (err == 0) {
				printf("reader_greater: failed while finding the node\n");
				abort();
			}
			if (prev_length == -1) {
				prev_length = current_node->length;
				continue;
			}
			if (current_node->length > prev_length) {
				counter++;
				prev_length = current_node->length;
			}
		}
		reader_greater_counter++;
	}
	return NULL;
}

void *reader_less(void *arg) {
	int err;
	int counter = 0;
	list_t *l = (list_t *)arg;
	printf("reader [%d %d %d]\n", getpid(), getppid(), gettid());

	//set_cpu(1);

	node_t *current_node;
	while (1) {
		counter = 0;
		int prev_length = -1;
		for (int i = 0; i < l->count; ++i) {
			err = list_get(l, i, &current_node);
			if (err == 0) {
				printf("reader_less: failed while finding the node\n");
				abort();
			}
			if (prev_length == -1) {
				prev_length = current_node->length;
				continue;
			}
			if (current_node->length < prev_length) {
				counter++;
				prev_length = current_node->length;
			}
		}
		reader_less_counter++;
	}
	return NULL;
}

void *reader_equal(void *arg) {
	int err;
	int counter = 0;
	list_t *l = (list_t *)arg;
	printf("reader [%d %d %d]\n", getpid(), getppid(), gettid());

	//set_cpu(1);

	node_t *current_node;
	while (1) {
		counter = 0;
		int prev_length = -1;
		for (int i = 0; i < l->count; ++i) {
			err = list_get(l, i, &current_node);
			if (err == 0) {
				printf("reader_equal: failed while finding the node\n");
				abort();
			}
			if (prev_length == -1) {
				prev_length = current_node->length;
				continue;
			}
			if (current_node->length == prev_length) {
				counter++;
				prev_length = current_node->length;
			}
		}
		reader_equal_counter++;
	}
	return NULL;
}

void *swapper(void *arg) {
	int err;
	list_t *l = (list_t *) arg;
	printf("swapper [%d %d %d]\n", getpid(), getppid(), gettid());

	//set_cpu(2);

	while (1) {
		for (int i = 0; i < l->count - 1; ++i) {
			int will_be_swapped = rand() % 2;
			if (will_be_swapped == 1) {
				//printf("swapper %d will swap %d %d\n", gettid(), i, i+1);
				err = swap(l, i);
				if (err == 0) {
					printf("swapper [%d %d %d]: failed while swap\n", getpid(), getppid(), gettid());
				} else {
					pthread_mutex_lock(&swap_counter_mutex);
					swap_counter++;
					pthread_mutex_unlock(&swap_counter_mutex);
					//printf("swapper %d swapped sucessfully %d %d\n", gettid(), i, i+1);
					//print_list(l);
				}
			}
		}
	}

	return NULL;
}


int main() {
	srand(time(NULL));
	list_t *l;
	int err;
	printf("main [%d %d %d]\n", getpid(), getppid(), gettid());


	if (pthread_mutex_init(&swap_counter_mutex, NULL) != 0) {
		printf("main: failed while init mutex\n");
		return -1;
    }

	pthread_t lmonitor_tid;
	err = pthread_create(&lmonitor_tid, NULL, lmonitor, NULL);
	if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		abort();
	}

	l = list_init(LIST_SIZE);
	if (l == NULL) {
		printf("main: failed while list init\n");
		return -1;
	}
	err = list_fill(l);
	if (err == 0) {
		printf("main: failed while filling the list\n");
		return -1;
	}

	print_list(l);

	pthread_t swapper1, swapper2, swapper3;
	err = pthread_create(&swapper1, NULL, swapper, l);
	if (err) {
		printf("main: pthread_create() failed for swapper1: %s\n", strerror(err));
		return -1;
	}
	err = pthread_create(&swapper2, NULL, swapper, l);
	if (err) {
		printf("main: pthread_create() failed for swapper2: %s\n", strerror(err));
		return -1;
	}
	err = pthread_create(&swapper3, NULL, swapper, l);
	if (err) {
		printf("main: pthread_create() failed for swapper3: %s\n", strerror(err));
		return -1;
	}

	pthread_t reader_greater_tid, reader_less_tid, reader_equal_tid;
	err = pthread_create(&reader_greater_tid, NULL, reader_greater, l);
	if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}
	err = pthread_create(&reader_less_tid, NULL, reader_less, l);
	if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}
	err = pthread_create(&reader_equal_tid, NULL, reader_equal, l);
	if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}

	pthread_exit(NULL);

	return 0;
}
