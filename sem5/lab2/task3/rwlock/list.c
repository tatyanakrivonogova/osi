#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>

#include "list.h"
#include <stdatomic.h>


list_t* list_init(int max_count) {

	list_t *l = malloc(sizeof(list_t));
	if (!l) {
		printf("Cannot allocate memory for a list\n");
		abort();
	}

	int err = pthread_rwlock_init(&l->list_sync, 0);
	if (err != 0) {
		printf("list_init: failed while init rwlock\n");
		free(l);
		return NULL;
	}


	l->first = NULL;
	l->count = 0;
	l->max_count = max_count;
	return l;
}

void list_destroy(list_t *l) {
	node_t *first = l->first;
	if (first == NULL) return;
	l->first = l->first->next;

	int err = pthread_rwlock_destroy(&first->sync);
	if (err != 0) {
		printf("list_destroy: failed while destroy node\n");
	}

	free(first->string);
	free(first);
	list_destroy(l);
}

int list_add(list_t *l, char* string, int length) {
	assert(l->count <= l->max_count);
	if (l->count == l->max_count) {
		return 0;
	}

	node_t *new = malloc(sizeof(node_t));
	if (!new) {
		printf("Cannot allocate memory for new node\n");
		abort();
	}

	int err = pthread_rwlock_init(&new->sync, 0);
	if (err != 0) {
		printf("list_init: failed while init rwlock\n");
		free(l);
		return 0;
	}

	new->string = string;
	new->length = length;
	new->next = NULL;


	if (l->first == NULL) {
		l->first = new;
	} else {
		node_t* current = l->first;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = new;
	}

	l->count++;
	return 1;
}

int list_fill(list_t *l) {
	printf("list_fill\n");
	int err;
	for (int i = 0; i < l->max_count; ++i) {
		int length = rand() % (MAX_STRING_SIZE-1) + 1;
		//printf("len: %d\n", length);
		char* new = (char *) malloc((length + 1) * sizeof(char));
		if (!new) {
			printf("list_fill: failed while malloc for string\n");
			return 0;
		}
		for (int j = 0; j < length; j++) {
			new[j] = 'A' + rand() % 26;
		}
		new[length] = '\0';
		err = list_add(l, new, length);
		if (err == 0) {
			printf("list_fill: failed while add new node to list\n");
			return 0;
		}
		//printf("str: %s\n", new);
	}
    printf("list was filled!\n");
	return 1;
}

int lock_node(list_t *l, int index, node_t **node) {
	pthread_rwlock_t *current_rwlock = NULL;
	pthread_rwlock_t *prev_rwlock = NULL;

	int err;
	int cur_index = 0;
	prev_rwlock = &l->list_sync;
	err = pthread_rwlock_wrlock(prev_rwlock);
	if (err != 0) {
		printf("err\n");
		exit(-1);
	}


	node_t *current = l->first;
	current_rwlock = &current->sync;
	err = pthread_rwlock_wrlock(current_rwlock);
	if (err != 0) {
		printf("err\n");
		exit(-1);
	}


	while (cur_index < index) {

		if (current->next == NULL) {
			printf("find_node %d: failed while reading of list %d\n", gettid(), cur_index);
			exit(-1);
		}

		if (prev_rwlock != NULL) {
			err = pthread_rwlock_unlock(prev_rwlock);
			if (err != 0) {
				printf("err\n");
				exit(-1);
			}
		}

		prev_rwlock = current_rwlock;
		current_rwlock = &current->next->sync;
		err = pthread_rwlock_wrlock(current_rwlock);
		if (err != 0) {
			printf("err\n");
			exit(-1);
		}

		current = current->next;
		++cur_index;
	}


	if (prev_rwlock != NULL) {
		err = pthread_rwlock_unlock(prev_rwlock);
		if (err != 0) {
			printf("err\n");
			exit(-1);
		}
	}

	*node = current;
	return 1;
}



int list_get(list_t *l, int index, node_t **val) {
	assert(l->count >= 0);
	if (l->count == 0) {
		printf("Failed while reading of list. List is empty\n");
		return 0;
	}

	if (index < 0 || index >= l->count) {
		printf("Wrong index of list\n");
		return 0;
	}

	int err;
	int cur_index = 0;
	pthread_rwlock_t *current_rwlock = NULL;
	pthread_rwlock_t *prev_rwlock = NULL;


	prev_rwlock = &l->list_sync;
	err = pthread_rwlock_rdlock(prev_rwlock);
	if (err != 0) {
		printf("err\n");
		exit(-1);
	}

	node_t *current = l->first;

	current_rwlock = &current->sync;
	err = pthread_rwlock_rdlock(current_rwlock);
	if (err != 0) {
		printf("err\n");
		exit(-1);
	}

	while (cur_index < index) {

		if (current == NULL) {
			printf("print_list: failed while getting the node of list\n");
			break;
		}


		if (current->next == NULL) {
			printf("find_node %d: failed while getting the node of list %d\n", gettid(), cur_index);
			abort();
		}

		if (prev_rwlock != NULL) {
			err = pthread_rwlock_unlock(prev_rwlock);
			if (err != 0) {
				printf("err\n");
				exit(-1);
			}
		}

		prev_rwlock = current_rwlock;
		current_rwlock = &current->next->sync;

		err = pthread_rwlock_rdlock(current_rwlock);
		if (err != 0) {
			printf("err\n");
			exit(-1);
		}

		current = current->next;
		++cur_index;
	}

	if (current == NULL) {
		printf("print_list: failed while reading of list\n");
	}
	*val = current;


	if (prev_rwlock != NULL) {
		err = pthread_rwlock_unlock(prev_rwlock);
		if (err != 0) {
			printf("err\n");
			exit(-1);
		}
	}

	if (current_rwlock != NULL) {
		err = pthread_rwlock_unlock(current_rwlock);
		if (err != 0) {
			printf("err\n");
			exit(-1);
		}
	}
	return 1;
}


void print_list(list_t *l) {

	printf("l->count %d\n", l->count);
	int err;
	int cur_index = 0;
	int count = l->count;
	pthread_rwlock_t *current_rwlock = NULL;
	pthread_rwlock_t *prev_rwlock = NULL;

	prev_rwlock = &l->list_sync;
	err = pthread_rwlock_rdlock(prev_rwlock);
	if (err != 0) {
		printf("err\n");
		exit(-1);
	}

	node_t *current = l->first;

	current_rwlock = &current->sync;
	err = pthread_rwlock_rdlock(current_rwlock);
	if (err != 0) {
		printf("err\n");
		exit(-1);
	}

	while (cur_index < (count-1)) {

		if (current == NULL) {
			printf("print_list: failed while reading of list\n");
			break;
		}
		printf("list[%d]: %s\n", cur_index, current->string);


		if (current->next == NULL) {
			printf("find_node %d: failed while reading of list %d\n", gettid(), cur_index);
			abort();
		}

		if (prev_rwlock != NULL) {
			err = pthread_rwlock_unlock(prev_rwlock);
			if (err != 0) {
				printf("err\n");
				exit(-1);
			}
		}

		prev_rwlock = current_rwlock;
		current_rwlock = &current->next->sync;

		err = pthread_rwlock_rdlock(current_rwlock);
		if (err != 0) {
			printf("err\n");
			exit(-1);
		}

		current = current->next;
		++cur_index;
	}

	if (current == NULL) {
		printf("print_list: failed while reading of list\n");
	}
	printf("list[%d]: %s\n", cur_index, current->string);


	if (prev_rwlock != NULL) {
		err = pthread_rwlock_unlock(prev_rwlock);
		if (err != 0) {
			printf("err\n");
			exit(-1);
		}
	}

	if (current_rwlock != NULL) {
		err = pthread_rwlock_unlock(current_rwlock);
		if (err != 0) {
			printf("err\n");
			exit(-1);
		}
	}
}

int swap(list_t *l, int index) {
	int err;
	if (index < 0 || index >= (l->count-1)) {
		printf("swap: wrong value of index\n");
		return 0;
	}

	node_t *node1 = NULL;
	node_t *node2 = NULL;
	node_t *node_prev = NULL;
	pthread_rwlock_t *rwlock_prev = NULL;
	pthread_rwlock_t *rwlock1 = NULL;
	pthread_rwlock_t *rwlock2 = NULL;
	pthread_rwlock_t *rwlock_list = NULL;

	if (index != 0) {
		err = lock_node(l, index - 1, &node_prev);
		if (err == 0) {
			printf("swap: !!! failed while finding the previous node\n");
			exit(-1);
		}
		rwlock_prev = &node_prev->sync;
	} else {
		rwlock_list = &l->list_sync;
		err = pthread_rwlock_wrlock(rwlock_list);
		if (err != 0) {
			printf("err\n");
			exit(-1);
		}
	}


	if (index != 0) {
		if (node_prev->next == NULL) {
			printf("swap %d: !!! node_prev->next is null %d\n", gettid(), index);
			exit(-1);
		}
		node1 = node_prev->next;
	} else {
		node1 = l->first;
		if (l->first == NULL) {
			printf("swap %d: !!! failed while finding the first node\n", gettid());
			exit(-1);
		}
	}
	
	rwlock1 = &node1->sync;
	err = pthread_rwlock_wrlock(rwlock1);
	if (err != 0) {
		printf("err\n");
		exit(-1);
	}


	if (node1->next == NULL) {
		printf("swap %d: !!! failed while getting the second node %d\n", gettid(), index);
		print_list(l);
		exit(-1);
	}
	node2 = node1->next;
	rwlock2 = &node2->sync;

	err = pthread_rwlock_wrlock(rwlock2);
	if (err != 0) {
		printf("err\n");
		exit(-1);
	}

	if (index == 0) {
		l->first = node2;
	} else {
		node_prev->next = node2;
	}
	node1->next = node2->next;
	node2->next = node1;

	
	err = pthread_rwlock_unlock(rwlock1);
	if (err != 0) {
		printf("err\n");
		exit(-1);
	}

	err = pthread_rwlock_unlock(rwlock2);
	if (err != 0) {
		printf("err\n");
		exit(-1);
	}

	if (index != 0) {
		err = pthread_rwlock_unlock(rwlock_prev);
		if (err != 0) {
			printf("err\n");
			exit(-1);
		}
	} else {
		err = pthread_rwlock_unlock(rwlock_list);
		if (err != 0) {
			printf("err\n");
			exit(-1);
		}
	}

	return 1;
}
