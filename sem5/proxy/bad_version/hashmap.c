#define _GNU_SOURCE
#include <pthread.h>

#include "hashmap.h"
#define ACTUAL_RECORD_TIME 60

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

	//h->records_count = 0;
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

	if (pthread_create(&h->hmonitor_tid, NULL, hmonitor, h) != 0) {
		printf("hashmap_init: pthread_create() failed\n");
		abort();
	}

	// if (pthread_rwlock_init(&h->hashmap_sync, NULL) != 0) {
	// 	printf("hashmap_init: failed while init rwlock\n");
	// 	free(h);
	// 	return NULL;
	// }
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
            free(current->request);
            free(current->response);
            if (pthread_rwlock_destroy(&current->record_sync) != 0) {
                printf("hashmap_destroy: failed while destroying rwlock for record\n");
            }
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

int hashmap_put(hashmap_t *h, char* request, char* response, size_t request_length, size_t response_length) {
	printf("--------------------------------Hashmap put started...\n");
	record_t *new = malloc(sizeof(record_t));
	if (!new) {
		printf("hashmap_put(): can't allocate memory for new record\n");
		abort();
	}

	new->last_using = time(NULL);
	printf("time: %ld\n", new->last_using);
    new->request_length = request_length;
    new->response_length = response_length;
	new->request = malloc(request_length);
	if (new->request == NULL) {
		printf("hashmap_put(): can't allocate memory for request\n");
		abort();
	}
	new->response = malloc(response_length);
	if (new->response == NULL) {
		printf("hashmap_put(): can't allocate memory for response\n");
		abort();
	}
	memcpy(new->request, request, request_length);
	memcpy(new->response, response, response_length);
	new->next = NULL;

	if (pthread_rwlock_init(&new->record_sync, NULL) != 0) {
		printf("hashmap_put: failed while init rwlock\n");
		free(new);
		return -1;
	}

    size_t hash = get_hash(request, request_length);
	printf("hash: %ld\n", hash);

    
	int err = pthread_rwlock_wrlock(&h->buckets[hash]->bucket_sync);
	if (err != 0) {
		printf("hashmap_put(): failed while rwlock_wrlock() for bucket\n");
		pthread_rwlock_destroy(&new->record_sync);
		free(new);
		return -1;
	}

    if (h->buckets[hash]->first == NULL) {
		h->buckets[hash]->first = new;
	} else {
		record_t* current = h->buckets[hash]->first;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = new;
	}
	err = pthread_rwlock_unlock(&h->buckets[hash]->bucket_sync);
	if (err != 0) {
		printf("hashmap_put(): failed while rwlock_unlock() for bucket\n");
		return -1;
	}

	//h->records_count++; ///probleeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeemmmmmmmmmmmmmmmmmmm
	printf("--------------------------------Hashmap put finished.\n");
	return 0;
}

int hashmap_get(hashmap_t* h, char* request, size_t request_length, record_t** record) {
	printf("--------------------------------Hashmap get started...\n");
	size_t request_hash = get_hash(request, request_length);
	int err = pthread_rwlock_rdlock(&h->buckets[request_hash]->bucket_sync);
	if (err != 0) {
		printf("hashmap_get(): failed while rwlock_rdlock() for bucket\n");
		return -1;
	}
	printf("****************************************locked bucket rd\n");

	if (h->buckets[request_hash]->first == NULL) {
		err = pthread_rwlock_unlock(&h->buckets[request_hash]->bucket_sync);
		if (err != 0) {
			printf("hashmap_get(): failed while rwlock_unlock() for bucket\n");
		}
		printf("****************************************unlocked bucket rd\n");
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
		printf("****************************************unlocked bucket rd\n");
		return -1;
	}
	

	// //update last using time
	// err = pthread_rwlock_wrlock(&current_record->record_sync);
	// if (err != 0) {
	// 	printf("hashmap_get(): failed while rwlock_wrlock() for record\n");
	// 	return -1;
	// }
	// printf("****************************************locked record wr\n");
	// current_record->last_using = time(NULL);
	// err = pthread_rwlock_unlock(&current_record->record_sync);
	// if (err != 0) {
	// 	printf("hashmap_get(): failed while rwlock_unlock() for record\n");
	// 	return -1;
	// }
	// printf("****************************************unlocked record wr\n");

	//lock necessary record for reading
	err = pthread_rwlock_rdlock(&current_record->record_sync);
	if (err != 0) {
		printf("hashmap_get(): failed while rwlock_rdlock() for record\n");
		return -1;
	}
	printf("****************************************locked record rd\n");

	err = pthread_rwlock_unlock(&h->buckets[request_hash]->bucket_sync);
	if (err != 0) {
		printf("hashmap_get(): failed while rwlock_unlock() for bucket\n");
		return -1;
	}
	printf("****************************************unlocked bucket rd\n");

	printf("--------------------------------Hashmap get finished.\n");
	return 0;
}

void free_record(record_t* record) {
	free(record->request);
	free(record->response);
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
			printf("hashmap_clean(): failed while rwlock_wrlock()\n");
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
				printf("--------------------------record was removed\n");
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