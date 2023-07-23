#include <stdio.h>
#include <unistd.h>

int main() {
	int pid;
	int pfd[2];
	int err;

	err = pipe(pfd);
	if (err == -1) {
		printf("Failed pipe\n");
		return -1;
	}

	pid = fork();
	if (pid == 0) {
		char buff[] = "hello from child";
		while (1) {
			printf("child: sending '%s'\n", buff);
			write(pfd[1], buff, sizeof(buff));
			//sleep(2);
		}
	} else if (pid > 0) {
		char buff[128];
		while (1) {
			read(pfd[0], buff, sizeof(buff));
			printf("parent recieved: '%s'\n", buff);
		}
	} else {
		printf("Failed fork\n");
	}
	return 0;
}
