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
		close(pfd[0]);
		int val = 0;
		while (1) {
			printf("child: sending '%d'\n", val);
			write(pfd[1], &val, sizeof(val));
			++val;
			//sleep(2);
		}
		close(pfd[1]);

	} else if (pid > 0) {
		close(pfd[1]);
		int val;
		int check_val = 0;
		while (1) {
			read(pfd[0], &val, sizeof(val));
			if (val == check_val) {
				printf("parent recieved: '%d'\n", val);
			} else {
				printf("Failure! the numbers are out of order\n");
			}
			++check_val;
		}
		close(pfd[0]);

	} else {
		printf("Failed fork\n");
	}
	return 0;
}
