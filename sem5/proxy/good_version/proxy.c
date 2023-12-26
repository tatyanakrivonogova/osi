#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <semaphore.h>

#include "http_parser.h"
#include "hashmap.h"

#define IP_ADDRESS_LENGTH 16

hashmap_t *cache;

//proxy server info
typedef struct proxy {
    int proxy_fd;
} proxy_t;

//global variable for proxy
proxy_t proxy;


//client request-response info
typedef struct client {
    // semaphore for waiting of downloading start
    sem_t downloader_semaphore;
    int client_fd;
    size_t request_size;
    char* request;
    size_t request_index;
} client_t;


pthread_t downloader_tid;


//handler with closing of proxy fd
void sigint_handler(int sig) {
    if (sig == SIGINT) {
        write(1, "SIGINT\n", sizeof("SIGINT\n"));
        close(proxy.proxy_fd);
        write(1, "sigint_hadler(): Proxy was closed\n", sizeof("sigint_hadler(): Proxy was closed\n"));
        hashmap_destroy(cache);
        write(1, "sigint_handler(): Cache was destroyed\n", sizeof("sigint_handler(): Cache was destroyed\n"));
        exit(-1);
    }
}


int is_digit(char c) {
    return c >= '0' && c <= '9';
}


int hostname_to_ip(char* hostname , char* ip) {
    struct hostent *he;
    struct in_addr **addr_list;
    if ((he = gethostbyname(hostname)) == NULL) {
        printf("Failed while getting ip by host name\n");
        return -1;
    }
    addr_list = (struct in_addr **) he->h_addr_list;
    for (int i = 0; addr_list[i] != NULL; i++) {
        strcpy(ip, inet_ntoa(*addr_list[i]) );
        return 0;
    }
    return -1;
}


int check_port(char* port) {
    int index = 0;
    while (port[index] != '\0') {
        if (!is_digit(port[index])) {
            return -1;
        }
        index++;
    }
    return 0;
}


int start_proxy(int proxy_port) {
    printf("Port: %d\n", proxy_port);
    int proxy_fd = 0;
    struct sockaddr_in proxy_sd;

    //creating of socket
    if((proxy_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Failed to create socket\n");
        exit(-1);
    }
    printf("Proxy created\n");

    //binding
    memset(&proxy_sd, 0, sizeof(proxy_sd));
    proxy_sd.sin_family = AF_INET;
    proxy_sd.sin_port = htons(proxy_port);
    proxy_sd.sin_addr.s_addr = INADDR_ANY;

    if ((bind(proxy_fd, (struct sockaddr*) &proxy_sd,sizeof(proxy_sd))) < 0) {
        printf("Failed to bind a socket");
        exit(-1);
    }

    if ((listen(proxy_fd, SOMAXCONN)) < 0) {
        printf("Failed to listen");
        exit(-1);
    }
    return proxy_fd;
}

//disconnect client and free all resources
void disconnect_client(client_t* client) {
    int fd = client->client_fd;
    close(client->client_fd);
    free(client->request);
    free(client);
    printf("Client %d was disconnected\n", fd);
}

response_part_t* create_new_part_of_response(client_t* client) {
    response_part_t* new_part = malloc(sizeof(response_part_t));
    if (new_part == NULL) {
        printf("send_request_to_server(): failed while malloc for new part of response\n");
        write(client->client_fd, "Can't read the response from server\n", sizeof("Can't read the response from server\n"));
        disconnect_client(client);
        pthread_exit(NULL);
    }
    new_part->part_of_response = (char *) calloc(BUFFER_SIZE, sizeof(char));
    if (new_part->part_of_response == NULL) {
        printf("send_request_to_server(): failed while calloc for new part of response\n");
        write(client->client_fd, "Can't read the response from server\n", sizeof("Can't read the response from server\n"));
        disconnect_client(client);
        pthread_exit(NULL);
    }
    new_part->next_part = NULL;
    new_part->length = 0;
    if (pthread_rwlock_wrlock(&new_part->part_sync) != 0) {
        printf("send_request_to_server(): failed while rwlock_wrlock() for new part of response\n");
        write(client->client_fd, "Can't read the response from server\n", sizeof("Can't read the response from server\n"));
        disconnect_client(client);
        pthread_exit(NULL);
    }
    return new_part;
}

void* send_request_to_server(void* args) {
    client_t* client = (client_t*) args;
    int read_bytes;
    char buffer[BUFFER_SIZE];

    //parsing of request
    char* method;
    char* path;
    size_t method_len, path_len;
    int minor_version;
    size_t num_headers = 100;
    struct phr_header headers[num_headers];
    int err = phr_parse_request(client->request, client->request_size, (const char **) &method, &method_len, (const char **) &path, &path_len, &minor_version, headers, &num_headers, 0);
    if (err == -1 || err == -2) {
        printf("send_request_to_server(): failed while parse HTTP request, error %d\n", err);
        write(client->client_fd, "Failed while parse HTTP resuest. Change and try again\n", sizeof("Failed while parse HTTP resuest. Change and try again\n"));
        disconnect_client(client);
        pthread_exit(NULL);
    }


    //finding hostname header in request
    char *host = NULL;
    int host_len = 0;
    for (size_t i = 0; i < num_headers; i++) {
        if (strncmp(headers[i].name, "Host", 4) == 0) {
            host = malloc(headers[i].value_len + 1);
            if (host == NULL) {
                disconnect_client(client);
                pthread_exit(NULL);
            }
            strncpy(host, headers[i].value, headers[i].value_len);
            host_len = headers[i].value_len;
            break;
        }
    }

    char* colon = strchr(host, ':');
    char* host_name;
    char* port;
    
    if (colon != NULL) {
        host_name = (char*)malloc(colon - host + 1);
        strncpy(host_name, host, colon - host);
        host_name[colon - host] = '\0';
        
        port = (char*)malloc(strlen(colon + 1) + 1);
        strcpy(port, colon + 1);
    } else {
        host_name = (char*)malloc(strlen(host) + 1);
        strcpy(host_name, host);
        port = (char*)malloc(3);
        strcpy(port, "80");
    }

    printf("Host_name: %s\n", host_name);
    printf("Port: %s\n", port);
    

    //getting host ip by hostname
    char* server_ip = malloc(IP_ADDRESS_LENGTH);
    if (hostname_to_ip(host_name, server_ip) == -1) {
        printf("send_request_to_server(): can't find server\n");
        disconnect_client(client);
        pthread_exit(NULL);
    }

    //check port
    if (check_port(port) == -1) {
        printf("Wrong PORT. Try again\n");
        free(port);
        port = (char*)malloc(3);
        strcpy(port, "80");
    }


    //creating socket for connection with server
    int server_fd = 0;
    struct sockaddr_in server_sd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        printf("send_request_to_server(): failed while socket creating\n");
        disconnect_client(client);
        pthread_exit(NULL);
    }
    printf("send_request_to_server(): server socket created\n");

    
    memset(&server_sd, 0, sizeof(server_sd));
    server_sd.sin_family = AF_INET;
    server_sd.sin_port = htons(atoi(port));
    server_sd.sin_addr.s_addr = inet_addr(server_ip);


    //connecting to server
    if ((connect(server_fd, (struct sockaddr *)&server_sd, sizeof(server_sd))) < 0) {
        printf("send_request_to_server(): failed while connect\n");
        disconnect_client(client);
        pthread_exit(NULL);
    }
    printf("send_request_to_server(): server socket connected\n");

    //sending request to server
    read_bytes = write(server_fd, client->request, client->request_size);
    if (read_bytes <= 0) {
        printf("send_request_to_server(): failed to send request to server\n");
        write(client->client_fd, "Can't send request to server\n", sizeof("Can't send request to server\n"));
        disconnect_client(client);
        close(server_fd);
        pthread_exit(NULL);
    }

    record_t* new_record = malloc(sizeof(record_t));
    if (new_record == NULL) {
        printf("send_request_to_server(): can't allocate memory for new cache record\n");
        close(server_fd);
        disconnect_client(client);
        pthread_exit(NULL);
    }

    pthread_rwlock_t* bucket_rwlock = malloc(sizeof(pthread_rwlock_t));
    if (bucket_rwlock == NULL) {
        printf("send_request_to_server(): can't allocate memory for bucket rwlock\n");
        abort();
    }

    err = hashmap_put(cache, client->request, client->request_size, &new_record, &bucket_rwlock);
    if (err != 0) {
        printf("send_request_to_server(): failed while put new cache record\n");
        close(server_fd);
        disconnect_client(client);
        pthread_exit(NULL);
    }

    err = sem_post(&client->downloader_semaphore);
    if (err != 0) {
        printf("send_request_to_server(): failed while sem_post()\n");
    }

    response_part_t* current_part = new_record->response;

    //getting response from server
    printf("start downloading...\n");
    while (1) {
        memset(buffer, '\0', BUFFER_SIZE);
        read_bytes = read(server_fd, buffer, BUFFER_SIZE);

        //printf("read_bytes: %d\n", read_bytes);
        if (read_bytes <= 0) {
            printf("send_request_to_server(): server disconnected\n");
            break;
        } else {
            //printf("Part of response\n");
            if (new_record->response_index + read_bytes > new_record->response_length) {
                //printf("new part\n");
                new_record->response_length += BUFFER_SIZE;
                response_part_t* new_part = create_new_part_of_response(client);
                current_part->next_part = new_part;

                size_t part1_size = BUFFER_SIZE - current_part->length;
                size_t part2_size = read_bytes - part1_size;

                // copy in the current until it's full
                memcpy(&current_part->part_of_response[current_part->length], buffer, part1_size);
                current_part->length += part1_size;
                new_record->response_index += part1_size;
                if (pthread_rwlock_unlock(&current_part->part_sync) != 0) {
                    printf("send_request_to_server(): failed while rwlock_unlock() for ready part of response\n");
                }

                current_part = current_part->next_part;

                // copy in the next
                memcpy(&current_part->part_of_response[current_part->length], buffer, part2_size);
                current_part->length += part2_size;
                new_record->response_index += part2_size;

            } else {

                memcpy(&current_part->part_of_response[current_part->length], buffer, read_bytes);
                current_part->length += read_bytes;
                new_record->response_index += read_bytes;
            }
        }
    }

    if (pthread_rwlock_unlock(&current_part->part_sync) != 0) {
        printf("send_request_to_server(): failed while rwlock_unlock() for ready part of response\n");
    }

    printf("send_request_to_server(): end of response. Close connection...\n");
    new_record->response_length = new_record->response_index;
    new_record->is_full = 1;
    close(server_fd);
    printf("send_request_to_server(): connection closed.\n");

    if (pthread_rwlock_unlock(bucket_rwlock) != 0) {
        printf("send_request_to_server(): failed while rwlock_unlock() fro bucket\n");
    }
}

//update last using time
void update_last_using_time(record_t* cache_record) {
	int err = pthread_rwlock_wrlock(&cache_record->record_sync);
	if (err != 0) {
		printf("hashmap_get(): failed while rwlock_wrlock() for record\n");

	}

	cache_record->last_using = time(NULL);
	err = pthread_rwlock_unlock(&cache_record->record_sync);
	if (err != 0) {
		printf("hashmap_get(): failed while rwlock_unlock() for record\n");
	}
}


void* handle_client(void *args) {
    client_t* client = (client_t*) args;
    char buffer[BUFFER_SIZE];
    int read_bytes = 0;
    printf("handle_client(): %d\n", client->client_fd);


    //getting of client's request
    while (1) {
        memset(buffer, '\0', BUFFER_SIZE);
        read_bytes = read(client->client_fd, buffer, BUFFER_SIZE);
        printf("read_bytes: %d\n", read_bytes);
        if (read_bytes <= 0) {
            printf("Failed while receiving HTTP request from client\n");
            disconnect_client(client);
            pthread_exit(NULL);
        } else {
            if (client->request_size == 0) {
                client->request_size = BUFFER_SIZE;
                client->request = (char *) calloc(client->request_size, sizeof(char));
                if (client->request == NULL) {
                    printf("handle_client(): failed while calloc for request\n");
                    disconnect_client(client);
                    pthread_exit(NULL);
                }
            }
            if (client->request_index + read_bytes >= client->request_size) {
                client->request_size *= 2;
                client->request = realloc(client->request, client->request_size * sizeof(char));
                if (client->request == NULL) {
                    printf("handle_client(): failed while realloc for request\n");
                    disconnect_client(client);
                    pthread_exit(NULL);
                }
            }
            memcpy(&client->request[client->request_index], buffer, read_bytes);
            client->request_index += read_bytes;
            if (strstr(client->request, "\n\n") != NULL) {
                printf("handle_client(): full request was got\n");
                client->request_size = client->request_index;
                break;
            }
        }
    }

    //parsing of request
    char* method;
    char* path;
    size_t method_len, path_len;
    int minor_version;
    size_t num_headers = 100;
    struct phr_header headers[num_headers];
    int err = phr_parse_request(client->request, client->request_size, (const char **) &method, &method_len, (const char **) &path, &path_len, &minor_version, headers, &num_headers, 0);
    if (err == -1 || err == -2) {
        printf("send_request_to_server(): failed while parse HTTP request, error %d\n", err);
        write(client->client_fd, "Failed while parse HTTP resuest. Change and try again\n", sizeof("Failed while parse HTTP resuest. Change and try again\n"));
        disconnect_client(client);
        pthread_exit(NULL);
    }
    
    // check method of request
    char* supported_method = "GET";
    if (strncmp(method, supported_method, method_len) != 0) {
        printf("handle_client(): not GET request\n");
        write(client->client_fd, "not GET request", sizeof("not GET request"));
        disconnect_client(client);
        return NULL;
    }

    // check version of HTTP
    if (minor_version != 0) {
        printf("handle_client(): not HTTP/1.0 request\n");
        write(client->client_fd, "not HTTP/1.0 request", sizeof("not HTTP/1.0 request"));
        disconnect_client(client);
        return NULL;
    }

    //find response for this request in cache or get and save it
    int in_cache;
    record_t* cache_record = malloc(sizeof(record_t));
    pthread_rwlock_t* bucket_rwlock = malloc(sizeof(pthread_rwlock_t));
    if (bucket_rwlock == NULL) {
        printf("handle_client(): can't allocate memory for bucket rwlock\n");
        abort();
    }
    in_cache = hashmap_get(cache, client->request, client->request_size, &cache_record, &bucket_rwlock);

    //response not in cache
    if (in_cache == -1) {
        printf("handle_client(): going to send request to server...\n");
        sem_init(&client->downloader_semaphore, 0, 0);
        int err = pthread_create(&downloader_tid, NULL, send_request_to_server, client);
        if (err != 0) {
            printf("handle_client(): failed while pthread_create() for downloading the response\n");
            write(client->client_fd, "Can't download resource from server", sizeof("Can't download resource from server"));
            disconnect_client(client);
            return NULL;
        }
        err = sem_wait(&client->downloader_semaphore);
        if (err != 0) {
            printf("handle_client(): failed while sem_wait()\n");
        }
        printf("handle_client(): cache record was added in cache :)\n");
    }

    // record must be in cache here
    in_cache = hashmap_get(cache, client->request, client->request_size, &cache_record, &bucket_rwlock);
    if (in_cache == -1) {
        printf("handle_client(): cache record downloading started but it's not in cache\n");
        return NULL;
    }


    // waiting for appearing of record in cache
    // if (in_cache == -1) {
    //     while (1) {
    //         in_cache = hashmap_get(cache, client->request, client->request_size, &cache_record, &bucket_rwlock);
    //         if (in_cache != -1) {
    //             printf("cache record was added\n");
    //             break;
    //         }
    //     }
    // }

    //writing response to client
    response_part_t* current_part = cache_record->response;
    int write_bytes;
    while (1) {
        if (current_part == NULL) {
            break;
        }
        int err = pthread_rwlock_rdlock(&current_part->part_sync);
        if (err != 0) {
            printf("failed while rwlock_rdlock() for reading cache record\n");
        }
        write_bytes = write(client->client_fd, current_part->part_of_response, current_part->length);
        if (write_bytes <= 0) {
            printf("handle_client(): client disconnected\n");
            break;
        }
        //printf("------------------------------write %ld\n", current_part->length);
        current_part = current_part->next_part;
    }

    // finishing of connection
    printf("handle_client(): all response was sent\n");
    if (pthread_rwlock_unlock(&cache_record->record_sync) != 0) {
        printf("handle_client(): failed while rwlock_unlock() for cache record\n");
    }
    if (pthread_rwlock_unlock(bucket_rwlock) != 0) {
        printf("handle_client(): failed while rwlock_unlock() for bucket\n");
    }
    disconnect_client(client);
    printf("handle_client(): client was disconnected.\n");

    // update last using time of cache record
    update_last_using_time(cache_record);
    return NULL;
}


int main(int argc,char *argv[]) {
    pthread_t tid;
    char proxy_port[100];

    //check number of args
    if (argc < 2) {
        printf("Usage: ./proxy PROXY_PORT\n");
        return 0;
    }

    //check port
    strcpy(proxy_port, argv[1]);
    printf("proxy port is %s\n", proxy_port);
    if (check_port(proxy_port) == -1) {
        printf("Wrong PROXY PORT. Try again\n");
        exit(-1);
    }

    //set SIGINT handler
    struct sigaction sig_act = { 0 };
    sig_act.sa_handler = sigint_handler;
    sigemptyset(&sig_act.sa_mask);
    if (sigaction(SIGINT, &sig_act, NULL) != 0) {
        printf("main(): failed while set sigaction\n");
        exit(-1);
    }

    //create cache
    cache = hashmap_init();
	if (cache == NULL) {
		printf("main(): failed while hashmap init\n");
		return -1;
	}
    
    //start proxy
    proxy.proxy_fd = start_proxy(atoi(proxy_port));
    printf("main(): waiting for connection..\n");

    //listening port and accepting the clients
    int client_fd = 0;
    while(1) {
        client_fd = accept(proxy.proxy_fd, (struct sockaddr*)NULL, NULL);
        printf("main(): Client %d connected\n", client_fd);
        if (client_fd > 0) {
            int flag = 1;
            if (setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0) {
                perror("main(): Failed while setting NODELAY for new client");
                break;
            }

            client_t* new_client = malloc(sizeof(client_t));
            if (new_client == NULL) {
                printf("main(): failed while malloc for new client\n");
                break;
            }

            new_client->client_fd = client_fd;
            new_client->request_size = 0;
            new_client->request = NULL;
            new_client->request_index = 0;

            //create thread for handle of client
            if (pthread_create(&tid, NULL, handle_client, (void*) new_client) != 0) {
                free(new_client);
                printf("main(): failed while pthread_create()\n");
                break;
            }
        } else {
            printf("main(): failed while accept client\n");
        }
    }

    close(proxy.proxy_fd);
    return 0;
}
