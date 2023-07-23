#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>

#define CLIENTS_COUNT 50
#define BUFFER_SIZE 128
#define PORT 8080

int main(void) {
	int srv_sock;
	int clt_socks[CLIENTS_COUNT];
	int s = 0;
	struct sockaddr_in srv_sockaddr;
	struct sockaddr_in clt_sockaddr;
	int len = 0;
	//int val = 0;
	int err;
	int ret;
	char buffer[BUFFER_SIZE];
	
	memset(clt_socks, -1, sizeof(clt_socks));

	srv_sock = socket(AF_INET, SOCK_STREAM, 0);
    	if (srv_sock == -1) {
        	perror("Error while creating socket");
        	return -1;
    	}

	memset(&srv_sockaddr, 0, sizeof(struct sockaddr_in));
    	srv_sockaddr.sin_family = AF_INET;
    	srv_sockaddr.sin_port = htons(PORT);
    	srv_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    	
	if (bind(srv_sock, (struct sockaddr *)&srv_sockaddr, sizeof(srv_sockaddr)) == -1) {
        	perror("Error while binding");
		close(srv_sock);
        	return -1;
    	}

    	if (listen(srv_sock, 5) == -1) {
        	perror("Error while starting listen");
        	return -1;
    	}

	while (1) {
		int pid;
		fd_set rfds;
		struct timeval tv;
		int ret;
		int max_fd;

		FD_ZERO(&rfds);
		FD_SET(srv_sock, &rfds);
		max_fd = srv_sock;

		for (int i = 0; i < CLIENTS_COUNT; ++i) {
			if (clt_socks[i] != -1) {
				printf("add %d\n", clt_socks[i]);
				FD_SET(clt_socks[i], &rfds);
				max_fd = clt_socks[i] > max_fd ? clt_socks[i] : max_fd;
			}
		}
		tv.tv_sec = 5;
		tv.tv_usec = 0;

		ret = select(max_fd+1, &rfds, NULL, NULL, &tv);
		if (ret < 0) {
			perror("select error");
			break;
		} else if (ret == 0) {
			printf("There was no activity on the socket for 5 seconds\n");
			continue;
		} else if (ret > 0) {
			if (FD_ISSET(srv_sock, &rfds)) {
				printf("Accepting new connection\n");
				memset(&clt_sockaddr, 0, sizeof(struct sockaddr_in));
				clt_socks[s] = accept(srv_sock, (struct sockaddr*) &clt_sockaddr, &len);
				if (clt_socks[s] == -1) {
					printf("Accept failed: %s\n", strerror(errno));
					continue;
				}
				max_fd = clt_socks[s] > max_fd ? clt_socks[s] : max_fd;
				s++;
				
			} else {
				for (int k = 0; k < s; ++k) {
					if (FD_ISSET(clt_socks[k], &rfds)) {
						int count_bytes = recv(clt_socks[k], buffer, sizeof(buffer), 0);
						if (count_bytes <= 0) {
							close(clt_socks[k]);
							clt_socks[k] = -1;
							printf("Client disconnected\n");
							continue;
						}
                				printf("Received from client: ");
                				for (size_t i = 0; i < count_bytes; ++i) {
                        				printf("%c", buffer[i]);
                				}
                				send(clt_socks[k], buffer, count_bytes, 0);
					}
				}
			}
		}
	}
	close(srv_sock);
	return 0;
}
