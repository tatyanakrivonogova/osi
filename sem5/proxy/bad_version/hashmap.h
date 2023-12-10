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

typedef struct _Record {
	time_t last_using;
	char* request;
    char* response;
    size_t request_length;
    size_t response_length;
	struct _Record *next;
	pthread_rwlock_t record_sync; //for synchronised writing and reading of cache-record
} record_t;

typedef struct _Bucket {
	struct _Record *first;
	pthread_rwlock_t bucket_sync; //for synchronised cleanup-thread and client-threads
} bucket_t;

typedef struct _Hashmap {
    bucket_t* buckets[HASHMAP_SIZE];
	pthread_t hmonitor_tid;
	//size_t records_count;
	//pthread_rwlock_t hashmap_sync; //for destroy hashmap safety
} hashmap_t;

hashmap_t* hashmap_init();
void hashmap_destroy(hashmap_t *l);
int hashmap_put(hashmap_t *h, char* request, char* response, size_t request_length, size_t response_length);
int hashmap_get(hashmap_t* h, char* request, size_t request_length, record_t** record);
void hashmap_cleanup(hashmap_t* h);
#endif		// __FITOS_HASHMAP_H__
