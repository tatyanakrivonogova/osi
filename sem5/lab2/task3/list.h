#ifndef __FITOS_LIST_H__
#define __FITOS_LIST_H__

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_STRING_SIZE 100

typedef struct _Node {
	char* string;
	int length;
	struct _Node *next;
	pthread_spinlock_t sync;
} node_t;

typedef struct _List {
	node_t *first;
	int count;
	int max_count;
	pthread_spinlock_t list_sync;
} list_t;

list_t* list_init(int max_count);
void list_destroy(list_t *l);
int list_add(list_t *l, char* string, int length);
int list_fill(list_t *l);
int list_get(list_t *l, int index, char **string);
int swap(list_t *l, int index);
int find_node(list_t *l, int index, node_t **node);
void print_list(list_t *l);
#endif		// __FITOS_LIST_H__
