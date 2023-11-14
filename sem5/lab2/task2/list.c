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

        l->first = NULL;
        l->count = 0;
        l->max_count = max_count;
        return l;
}

void list_destroy(list_t *l) {
	node_t *first = l->first;
	if (first == NULL) return;
	l->first = l->first->next;
	free(first->string);
	free(first);
	if (pthread_mutex_destroy(&first->sync) != 0) {
        	printf("list_destroy: failed while destroying mutex\n");
    	}
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

	//strncpy(new->val, val, MAX_STRING_SIZE);
	new->string = string;
	new->length = length;
	new->next = NULL;
	/*
	pthread_mutex_t *mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    	if (mutex == NULL) {
        	perror("Ошибка выделения памяти под мьютекс");
        	return 1;
    	}*/
    	if (pthread_mutex_init(&new->sync, NULL) != 0) {
        	printf("list_add: failed while init mutex\n");
        	return 0;
    	}

	if (!l->first)
		l->first = new;
	else {
		node_t* last = l->first;
		while (last->next != NULL) {
			last = last->next;
		}
		last->next = new;
	}

	l->count++;
	return 1;
}

int fill_list(list_t *l) {
	int err;
        for (int i = 0; i < l->max_count; ++i) {
                int length = rand() % (MAX_STRING_SIZE-1) + 1;
		//printf("len: %d\n", length);
                char* new = (char *) malloc((length + 1) * sizeof(char));
                for (int j = 0; j < length; j++) {
                        new[j] = 'A' + rand() % 26;
                }
                new[length] = '\0';
                err = list_add(l, new, length);
		if (err == 0) {
			printf("fill_list: failed while add new node to list\n");
			return 0;
		}
		//printf("str: %s\n", new);
        }
        printf("list was filled!\n");
	return 1;
}

int find_node(list_t * l, int index, node_t **node) {
	int cur_index = 0;
	node_t *tmp = l->first;
        while (cur_index < index) {
                if (tmp->next == NULL) {
                        printf("find_node: failed while reading of list %d\n", cur_index);
                        return 0;
                }
                tmp = tmp -> next;
                ++cur_index;
        }
	*node = tmp;
	return 1;
}

int list_read(list_t *l, int index, char **val) {
	assert(l->count >= 0);
	if (l->count == 0) {
		printf("Failed while reading of list\n");
		return 0;
	}

	if (index < 0 || index >= l->count) {
		printf("Wrong index of list\n");
		return 0;
	}
/*
	node_t *tmp = l->first;
	int cur_index = 0;
	while (cur_index < index) {
		if (tmp->next == NULL) {
			printf("Failed while reading of list\n");
			return 0;
		}
		tmp = tmp -> next;
		++cur_index;
	}
*/
	node_t *node;
	find_node(l, index, &node);
	*val = node->string;
	return 1;
}

int swap(list_t *l, int index) {
	int err;
	if (index < 0 || index >= l->count-1) {
		printf("swap: wrong value of index\n");
		return 0;
	}

	node_t *node1;
	node_t *node2;
	node_t *node_prev;
	if (index != 0) {
		err = find_node(l, index - 1, &node_prev);
                if (err == 0) {
                        printf("swap: failed while finding the previous node\n");
                        return 0;
                }
		printf("swap: try to lock mutex for node_prev %d\n", index-1);
		pthread_mutex_lock(&node_prev->sync);
		printf("swap: mutex lock for node_prev %d\n", index-1);
	}


	err = find_node(l, index, &node1);
	if (err == 0) {
		printf("swap: failed while finding the node\n");
		return 0;
	}
	printf("swap: try to lock mutex for node1 %d\n", index);
	pthread_mutex_lock(&node1->sync);
	printf("swap: mutex lock for node1 %d\n", index);



	if (node1->next == NULL) {
		printf("swap: failed while getting the next node\n");
		return 0;
	}
	node2 = node1->next;
	printf("swap: try to lock mutex for node2 %d\n", index+1);
	pthread_mutex_lock(&node2->sync);
	printf("swap: mutex lock for node2 %d\n", index+1);



	node1->next = node2->next;
	node2->next = node1;
	if (index == 0) {
		l->first = node2;
	} else {
		node_prev->next = node2;
	}



	if (index != 0) {
		pthread_mutex_unlock(&node_prev->sync);
		printf("swap: mutex unlock for node_prev %d\n", index-1);
	} else {
		printf("swar: mutex free for node_prev\n");
	}

	pthread_mutex_unlock(&node1->sync);
	printf("swap: mutex unlock for node1 %d\n", index);
	pthread_mutex_unlock(&node2->sync);
	printf("swap: mutex unlock for node2 %d\n", index+1);	
	return 1;
}

