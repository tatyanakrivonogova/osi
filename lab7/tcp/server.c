#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int server_fd;
    int client_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Error while creating socket");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error while binding");
        return -1;
    }

    if (listen(server_fd, 5) == -1) {
        perror("Error while starting listen");
        return -1;
    }

    printf("Server started!\n");

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("Error while accept");
            continue;
        }

        printf("\nClient connected\n");

        int result = fork();
	if (result == -1) {
		perror("Error while fork");
		return -1;
	} else if (result == 0) {
            close(server_fd);

            char buffer[BUFFER_SIZE];
            ssize_t count_bytes;

            while ((count_bytes = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
		printf("Received from client: ");
                for (size_t i = 0; i < count_bytes; ++i) {
			printf("%c", buffer[i]);
		}
                write(client_fd, buffer, count_bytes);
            }

            printf("\nClient disconnected\n");

            close(client_fd);
            return 0;
        }

        close(client_fd);
    }

    return 0;
}
