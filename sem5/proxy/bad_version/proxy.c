#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "http_parser.h"
#include "hashmap.h"

#define BUFFER_SIZE 1024
#define IP_ADDRESS_LENGTH 16
#define SERVER_PORT 80

hashmap_t *cache;

//proxy server info
typedef struct proxy {
    int proxy_fd;
} proxy_t;
proxy_t proxy;


//client request-response info
typedef struct client {
    int client_fd;
    size_t request_size;
    char* request;
    size_t request_index;

    size_t response_size;
    char* response;
    size_t response_index;
} client_t;


//handler with closing of proxy fd
void sigint_handler(int sig) {
    if (sig == SIGINT) {
        printf("SIGINT\n");
        close(proxy.proxy_fd);
        printf("sigint_hadler(): Proxy fd %d was closed\n", proxy.proxy_fd);
        hashmap_destroy(cache);
        printf("sigint_handler(): Cache was destroyed\n");
        exit(-1);
    }
}


int is_digit(char c) {
    return c >= '0' && c <= '9';
}


int hostname_to_ip(char * hostname , char* ip) {
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

void send_request_to_server(client_t* client) {
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
    //printf("method len: %ld path len: %ld num headers: %ld\n", method_len, path_len, num_headers);
    

    //finding hostname header in request
    char *host_name = NULL;
    for (size_t i = 0; i < num_headers; i++) {
        if (strncmp(headers[i].name, "Host", 4) == 0) {
            host_name = malloc(headers[i].value_len + 1);
            if (host_name == NULL) {
                disconnect_client(client);
                pthread_exit(NULL);
            }
            strncpy(host_name, headers[i].value, headers[i].value_len);
            break;
        }
    }

    //getting host ip by hostname
    char* server_ip = malloc(IP_ADDRESS_LENGTH);
    if (hostname_to_ip(host_name, server_ip) == -1) {
        printf("send_request_to_server(): can't find server\n");
        disconnect_client(client);
        pthread_exit(NULL);
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
    server_sd.sin_port = htons(SERVER_PORT);
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
        pthread_exit(NULL);
    }

    //getting response from server
    while (1) {
        memset(buffer, '\0', BUFFER_SIZE);
        read_bytes = recv(server_fd, buffer, BUFFER_SIZE, 0);
        //printf("read_bytes: %d\n", read_bytes);
        if (read_bytes <= 0) {
            printf("send_request_to_server(): server disconnected\n");
            break;
        } else {
            //printf("Part of response\n");
            if (client->response_size == 0) {
                //printf("Calloc\n");
                client->response_size = BUFFER_SIZE;
                client->response = (char *) calloc(client->response_size, sizeof(char));
                if (client->response == NULL) {
                    printf("send_request_to_server(): failed while calloc for response\n");
                    write(client->client_fd, "Can't read the response from server\n", sizeof("Can't read the response from server\n"));
                    disconnect_client(client);
                    pthread_exit(NULL);
                }
            }
            if (client->response_index + read_bytes >= client->response_size) {
                //printf("Realloc\n");
                client->response_size *= 2;
                client->response = realloc(client->response, client->response_size * sizeof(char));
                if (client->response == NULL) {
                    printf("send_request_to_server(): failed while realloc for response\n");
                    write(client->client_fd, "Can't read the response from server\n", sizeof("Can't read the response from server\n"));
                    disconnect_client(client);
                    pthread_exit(NULL);
                }
            }
            memcpy(&client->response[client->response_index], buffer, read_bytes);
            client->response_index += read_bytes;
        }
    }

    printf("send_request_to_server(): end of response. Close connection...\n");
    close(server_fd);
    printf("send_request_to_server(): connection closed.\n");

    //saving in cache
    err = hashmap_put(cache, client->request, client->response, client->request_size, client->response_size);
    if (err == -1) {
        printf("send_request_to_server(): failed to save record in cache :(\n");
    } else {
        printf("send_request_to_server(): record was saved in cache :)\n");
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
        read_bytes = recv(client->client_fd, buffer, BUFFER_SIZE, 0);
        printf("read_bytes: %d\n", read_bytes);
        if (read_bytes <= 0) {
            printf("Failed while receiving HTTP request from client\n");
            disconnect_client(client);
            pthread_exit(NULL);
        } else {
            //printf("Part of request\n");
            if (client->request_size == 0) {
                //printf("Calloc\n");
                client->request_size = BUFFER_SIZE;
                client->request = (char *) calloc(client->request_size, sizeof(char));
                if (client->request == NULL) {
                    printf("handle_client(): failed while calloc for request\n");
                    disconnect_client(client);
                    pthread_exit(NULL);
                }
            }
            if (client->request_index + read_bytes >= client->request_size) {
                //printf("Realloc\n");
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

    //find response for this request in cache or get and save it
    int in_cache;
    record_t* cache_record = malloc(sizeof(record_t));
    in_cache = hashmap_get(cache, client->request, client->request_size, &cache_record);
    if (in_cache == -1) {
        printf("handle_client(): going to send request to server...\n");
        send_request_to_server(client);
        in_cache = hashmap_get(cache, client->request, client->request_size, &cache_record);
        if (in_cache == -1) {
            printf("handle_client(): failed after saving cache record\n");
            disconnect_client(client);
            return NULL;
        }
    } else {
        printf("handle_client(): response in cache\n");
    }
//here was sending request to server
    

    //write(client->client_fd, client->response, client->response_size);
    write(client->client_fd, cache_record->response, cache_record->response_length);

    if (pthread_rwlock_unlock(&cache_record->record_sync) != 0) {
        printf("handle_client(): failed while rwlock_unlock() for cache record\n");
    }
    printf("****************************************unlocked record rd\n");

    disconnect_client(client);
    printf("handle_client(): client was disconnected.\n");


    //update last using time
	int err = pthread_rwlock_wrlock(&cache_record->record_sync);
	if (err != 0) {
		printf("hashmap_get(): failed while rwlock_wrlock() for record\n");

	}
	printf("****************************************locked record wr\n");
	cache_record->last_using = time(NULL);
	err = pthread_rwlock_unlock(&cache_record->record_sync);
	if (err != 0) {
		printf("hashmap_get(): failed while rwlock_unlock() for record\n");
	}
	printf("****************************************unlocked record wr\n");

    printf("####################################################### unlocked finilly\n");

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
            client_t* new_client = malloc(sizeof(client_t));
            if (new_client == NULL) {
                printf("main(): failed while malloc for new client\n");
                break;
            }

            new_client->client_fd = client_fd;
            new_client->request_size = 0;
            new_client->request = NULL;
            new_client->request_index = 0;

            new_client->response_size = 0;
            new_client->response = NULL;
            new_client->response_index = 0;

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
