#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {
	printf("pid: %d\n", getpid());
	sleep(10);

	pid_t pid = fork();
	if (pid == 0) {	
		printf("I am child-parent: %d my parent: %d\n", getpid(), getppid());
		pid_t pid1 = fork();
		if (pid1 == 0) {
			printf("I am child of child: %d my parent: %d\n", getpid(), getppid());
			//sleep(20);
			for (int i = 0; i < 20; ++i) {
				printf("I am child of child: %d my parent: %d\n", getpid(), getppid());
				sleep(1);
			}
			exit(5);
		} else if (pid1 == -1) {
			printf("Failed fork\n");
		} else {
			//child-parent
			sleep(5);
			exit(5);
		}
	} else if (pid == -1) {
		printf("Failed fork\n");
	} else {
		printf("I am parent of parent, id: %d\n", getpid());
		sleep(10);
		int status;

                waitpid(pid, &status, 0);

                if (WIFEXITED(status)) {
                        int exit_status = WEXITSTATUS(status);
                        printf("Exit status of the child-parent process: %d\n", exit_status);
                }
	}
	return 0;
}
