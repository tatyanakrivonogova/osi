#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int client_fd;
    struct sockaddr_un server_addr;

    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("Error while creating socket");
        return -1;
    }

    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, "./my_socket", sizeof(server_addr.sun_path) - 1);
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error while connecting");
        return -1;
    }

    printf("Connected to server\n");

    char buffer[1024];
    ssize_t count_bytes;

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        count_bytes = write(client_fd, buffer, strlen(buffer));
        if (count_bytes == -1) {
            perror("Error while writing");
            break;
        }

        count_bytes = read(client_fd, buffer, sizeof(buffer));
        if (count_bytes == -1) {
            perror("Error while reading");
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
