#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_ADDRESS "127.0.0.1"

int main(int argc, char *argv[]) {
    int client_fd;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("Error while creating socket");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_ADDRESS, &server_addr.sin_addr);
    server_addr.sin_port = htons(PORT);
    
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error while connecting");
        return -1;
    }

    printf("Connected to server\n");

    char buffer[1024];
    ssize_t count_bytes;

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        count_bytes = send(client_fd, buffer, strlen(buffer), 0);
        if (count_bytes == -1) {
            perror("Error while sending");
            break;
        }

        count_bytes = recv(client_fd, buffer, sizeof(buffer), 0);
        if (count_bytes <= 0) {
            perror("Error while receiving");
            break;
        }

        printf("Received from server: ");
        for (size_t i = 0; i < count_bytes; ++i) {
		printf("%c", buffer[i]);
	}
    }

    close(client_fd);
    return 0;
}
