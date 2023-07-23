#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {
	printf("pid: %d\n", getpid());
	sleep(10);

	pid_t pid = fork();
	if (pid == 0) {
		printf("child id: %d	parent id: %d\n", getpid(), getppid());
			
		sleep(5);
		exit(5);
	} else if (pid == -1) {
		printf("Failed fork\n");
	} else {
		sleep(20);
		int status;

		waitpid(pid, &status, 0);

		if (WIFEXITED(status)) {
			int exit_status = WEXITSTATUS(status);
			printf("Exit status of the child process: %d\n", exit_status);
		}
	}
	return 0;
}
