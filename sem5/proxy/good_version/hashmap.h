#ifndef __FITOS_HASHMAP_H__
#define __FITOS_HASHMAP_H__

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define HASHMAP_SIZE 1024
#define BUFFER_SIZE 1024*1024

typedef struct _Response_part {
	char* part_of_response;
	size_t length;
	struct _Response_part *next_part;
	pthread_rwlock_t part_sync;
} response_part_t;


typedef struct _Record {
	time_t last_using;
	int is_full;
	char* request;
	size_t request_length;

	response_part_t* response;
    size_t response_length;
	size_t response_index;
	struct _Record *next;
	pthread_rwlock_t record_sync; //for synchronised writing and reading of cache-record
} record_t;

typedef struct _Bucket {
	struct _Record *first;
	pthread_rwlock_t bucket_sync; //for synchronised cleanup-thread and client-threads
} bucket_t;

typedef struct _Hashmap {
    bucket_t* buckets[HASHMAP_SIZE];
	pthread_t hcleanup_tid;
} hashmap_t;


hashmap_t* hashmap_init();
void hashmap_destroy(hashmap_t *l);

int hashmap_put(hashmap_t *h, char* request, size_t request_length, record_t** new_record, pthread_rwlock_t** bucket_rwlock);
int hashmap_get(hashmap_t* h, char* request, size_t request_length, record_t** record, pthread_rwlock_t** bucket_rwlock);

void hashmap_cleanup(hashmap_t* h);
void free_record(record_t* record);

#endif		// __FITOS_HASHMAP_H__
