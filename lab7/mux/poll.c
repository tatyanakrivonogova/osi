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
#include <poll.h>

#define CLIENTS_COUNT 50
#define BUFFER_SIZE 128
#define PORT 8080

int main(void) {
	int srv_sock;
	int clt_sock = -1;
	struct sockaddr_in srv_sockaddr;
	struct sockaddr_in clt_sockaddr;
	int clt_addr_len = 0;
	int ret;
	char buffer[BUFFER_SIZE];
	
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

	struct pollfd fds[CLIENTS_COUNT+1];
	memset(fds, 0, sizeof(fds));
	fds[0].fd = srv_sock;
	fds[0].events = POLLIN;
	int nfds = 1;
	
	int tv = 5000;
	while (1) {
		ret = poll(fds, nfds, tv);
		if (ret == -1) {
			perror("poll error");
			return -1;
		}
		if (ret == 0) {
			printf("There was no activity on the socket for 5 seconds\n");
			continue;
		}

		for (int i = 0; i < nfds; ++i) {
			if (fds[i].revents & POLLIN) {
				if (fds[i].fd == srv_sock) {
					printf("Accepting new connection...\n");
					memset(&clt_sockaddr, 0, sizeof(struct sockaddr_in));
					clt_sock = accept(srv_sock, (struct sockaddr*) &clt_sockaddr, &clt_addr_len);
					if (clt_sock == -1) {
						printf("Accept failed: %s\n", strerror(errno));
						continue;
					}
					fds[nfds].fd = clt_sock;
					fds[nfds].events = POLLIN;
					nfds++;
				} else {
					int count_bytes = recv(fds[i].fd, buffer, sizeof(buffer), 0);
					if (count_bytes <= 0) {
						printf("Client disconnected\n");
						close(fds[i].fd);
						fds[i].fd = -1;
						continue;
					}
                			printf("Received from client: ");
                			for (size_t i = 0; i < count_bytes; ++i) {
                        			printf("%c", buffer[i]);
                			}
                			send(fds[i].fd, buffer, count_bytes, 0);
				}
			}
		}
		for (int i = nfds-1; i >= 0; --i) {
			if (fds[i].fd == -1) {
				nfds--;
				fds[i] = fds[nfds];
			}
		}
	}
	close(srv_sock);
	return 0;
}
