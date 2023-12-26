#define _GNU_SOURCE
#include <pthread.h>

#include "hashmap.h"
#define ACTUAL_RECORD_TIME 40

void *hmonitor(void *arg) {
	hashmap_t *h = (hashmap_t *)arg;

	printf("hmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

	while (1) {
		hashmap_cleanup(h);
		sleep(10);
	}

	return NULL;
}


hashmap_t* hashmap_init() {
	printf("--------------------------------Hashmap init started...\n");
	hashmap_t *h = malloc(sizeof(hashmap_t));
	if (!h) {
		printf("hashmap_init(): can't allocate memory for hashmap\n");
		abort();
	}

    for (size_t i = 0; i < HASHMAP_SIZE; ++i) {
		h->buckets[i] = malloc(sizeof(bucket_t));
		if (h->buckets[i] == NULL) {
			printf("hashmap_init(): failed while malloc for bucket\n");
			abort();
		}
        h->buckets[i]->first = NULL;
		if (pthread_rwlock_init(&h->buckets[i]->bucket_sync, NULL) != 0) {
			printf("hashmap_init: failed while init bucket %ld rwlock\n", i);
			free(h);
			abort();
		}
    }

	if (pthread_create(&h->hcleanup_tid, NULL, hmonitor, h) != 0) {
		printf("hashmap_init: pthread_create() failed\n");
		abort();
	}

	printf("--------------------------------Hashmap init finished.\n");
	return h;
}

void hashmap_destroy(hashmap_t *h) {
	printf("--------------------------------Hashmap destroy started...\n");
	int err;

    for (size_t i = 0; i < HASHMAP_SIZE; ++i) {
		err = pthread_rwlock_wrlock(&h->buckets[i]->bucket_sync);
		if (err != 0) {
			printf("hasmap_destroy(): failed while rwlock_wrlock()\n");
		}
        while (h->buckets[i]->first != NULL) {
            record_t* current = h->buckets[i]->first;
            h->buckets[i]->first = current->next;
			free_record(current);
        }
		err = pthread_rwlock_destroy(&h->buckets[i]->bucket_sync);
		if (err != 0) {
			printf("hasmap_destroy(): failed while rwlock_destroy()\n");
		}
    }

	printf("--------------------------------Hashmap destroy finished.\n");
}

size_t get_hash(char *str, size_t length) {
    size_t hash = 5381;
    size_t index = 0;
    while (index < length) {
        hash = (((hash << 5) + hash) ^ str[index]) % HASHMAP_SIZE;
        index++;
    }
    return hash;
}

int hashmap_put(hashmap_t *h, char* request, size_t request_length, record_t** new_record, pthread_rwlock_t** bucket_rwlock) {
	printf("--------------------------------Hashmap put started...\n");

	record_t *new = malloc(sizeof(record_t));
	if (!new) {
		printf("hashmap_put(): can't allocate memory for new record\n");
		abort();
	}

	// init new cache record
	new->last_using = time(NULL);
	new->is_full = 0;
	printf("time: %ld\n", new->last_using);
    new->request_length = request_length;
    new->response_index = 0;
	new->response_length = BUFFER_SIZE;
	new->request = malloc(request_length);
	if (new->request == NULL) {
		printf("hashmap_put(): can't allocate memory for request\n");
		abort();
	}

	// init response
	response_part_t* first_part = malloc(sizeof(response_part_t));
	if (first_part == NULL) {
		printf("hashmap_put(): can't allocate memory for first part of response\n");
		abort();
	}
	first_part->length = 0;
	first_part->part_of_response = malloc(BUFFER_SIZE);
	if (first_part->part_of_response == NULL) {
		printf("hashmap_put(): can't allocate memory for first part of response\n");
		abort();
	}
	first_part->next_part = NULL;
	if (pthread_rwlock_init(&first_part->part_sync, NULL)) {
		printf("hashmap_put(): failed while rwlock_init\n");
	}


	// lock begin of response for writing in order to avoid reading
	if (pthread_rwlock_wrlock(&first_part->part_sync)) {
		printf("hashmap_put(): failed while rwlock_wrlock\n");
	}

	new->response = first_part;

	// copy request
	memcpy(new->request, request, request_length);
	new->next = NULL;

	if (pthread_rwlock_init(&new->record_sync, NULL) != 0) {
		printf("hashmap_put: failed while init rwlock\n");
		free(new);
		return -1;
	}

	// counting hash
    size_t hash = get_hash(request, request_length);
	printf("hash: %ld\n", hash);

    
	int err = pthread_rwlock_wrlock(&h->buckets[hash]->bucket_sync);
	if (err != 0) {
		printf("hashmap_put(): failed while rwlock_wrlock() for bucket\n");
		pthread_rwlock_destroy(&new->record_sync);
		free(new);
		return -1;
	}

	// find place for new record
    if (h->buckets[hash]->first == NULL) {
		h->buckets[hash]->first = new;
	} else {
		record_t* current = h->buckets[hash]->first;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = new;
	}
	*new_record = new;

	// relock bucket for reading
	err = pthread_rwlock_unlock(&h->buckets[hash]->bucket_sync);
	if (err != 0) {
		printf("hashmap_put(): failed while rwlock_unlock() for bucket\n");
	}
	err = pthread_rwlock_rdlock(&h->buckets[hash]->bucket_sync);
	if (err != 0) {
		printf("hashmap_put(): failed while rwlock_wrlock() for bucket\n");
	}

	// save bucket rwlock for future unlock
	*bucket_rwlock = &h->buckets[hash]->bucket_sync;

	printf("--------------------------------Hashmap put finished.\n");
	return 0;
}

int hashmap_get(hashmap_t* h, char* request, size_t request_length, record_t** record, pthread_rwlock_t** bucket_rwlock) {
	
	size_t request_hash = get_hash(request, request_length);
	int err = pthread_rwlock_rdlock(&h->buckets[request_hash]->bucket_sync);
	if (err != 0) {
		printf("hashmap_get(): failed while rwlock_rdlock() for bucket\n");
		return -1;
	}
	*bucket_rwlock = &h->buckets[request_hash]->bucket_sync;

	if (h->buckets[request_hash]->first == NULL) {
		err = pthread_rwlock_unlock(&h->buckets[request_hash]->bucket_sync);
		if (err != 0) {
			printf("hashmap_get(): failed while rwlock_unlock() for bucket\n");
		}
		*bucket_rwlock = NULL;
		return -1;
	}

	record_t* current_record = h->buckets[request_hash]->first;
	*record = NULL;

	while (current_record != NULL) {
		//if hashes are equals, check all request
		if (request_length == current_record->request_length && strncmp(request, current_record->request, request_length) == 0) {
			*record = current_record;
			break;
		}
		current_record = current_record->next;
	}

	//no record in cache for request
	if (*record == NULL) {
		err = pthread_rwlock_unlock(&h->buckets[request_hash]->bucket_sync);
		if (err != 0) {
			printf("hashmap_get(): failed while rwlock_unlock() for bucket\n");
		}
		return -1;
	}
	
	//lock necessary record for reading
	err = pthread_rwlock_rdlock(&current_record->record_sync);
	if (err != 0) {
		printf("hashmap_get(): failed while rwlock_rdlock() for record\n");
		return -1;
	}

	return 0;
}


void free_record(record_t* record) {
	free(record->request);
	response_part_t* response_part = record->response;
	response_part_t* next_part = NULL;
	while (response_part != NULL) {
		next_part = response_part->next_part;
		if (pthread_rwlock_destroy(&response_part->part_sync) != 0) {
			printf("free_record(): failed while rwlock_destroy()\n");
		}
		free(response_part->part_of_response);
		free(response_part);
		response_part = next_part;
	}
	if (pthread_rwlock_destroy(&record->record_sync) != 0) {
		printf("free_record(): failed while rwlock_destroy()\n");
	}
}


void hashmap_cleanup(hashmap_t* h) {
	time_t current_time = time(NULL);
	int err;
	printf("hashmap_cleanup() started...\n");

	for (size_t i = 0; i < HASHMAP_SIZE; ++i) {
		err = pthread_rwlock_wrlock(&h->buckets[i]->bucket_sync);
		if (err != 0) {
			printf("hashmap_cleanup(): failed while rwlock_wrlock()\n");
			continue;
		}
		record_t* current = h->buckets[i]->first;
		record_t* previous = NULL;
		while (current != NULL) {
			if (current_time - current->last_using > ACTUAL_RECORD_TIME) {
				if (previous == NULL) {
					h->buckets[i]->first = NULL;
					free_record(current);
				} else {
					previous->next = current->next;
					free_record(current);
				}
				printf("--------------------------record with hash %ld was removed\n", i);
			}
			previous = current;
			current = current->next;
		}
		err = pthread_rwlock_unlock(&h->buckets[i]->bucket_sync);
		if (err != 0) {
			printf("hashmap_cleanup(): failed while rwlock_unlock()\n");
		}
	}
	printf("hashmap_cleanup() finished.\n");
}