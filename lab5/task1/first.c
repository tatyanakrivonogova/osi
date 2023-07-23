#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int global = 10;

int main() {
	int local = 5;
	printf("\nlocal var address: %p	value: %d\n", &local, local);
	printf("global var address: %p	value: %d\n", &global, global);

	printf("pid: %d\n", getpid());
	sleep(10);

	pid_t pid = fork();
	if (pid == 0) {
		printf("child id: %d	parent id: %d\n", getpid(), getppid());
		
		printf("before modification:\n");
		printf("\n<child> local var address: %p value: %d\n", &local, local);
        	printf("<child> global var address: %p        value: %d\n", &global, global);

		++local;
		++global;
		printf("after modification:\n");
		printf("\n<child> local var address: %p value: %d\n", &local, local);
                printf("<child> global var address: %p        value: %d\n", &global, global);
		
		sleep(5);
		//local = local / 0;
		exit(5);
	} else if (pid == -1) {
		printf("Failed fork\n");
	} else {
		sleep(5);
		printf("\n<parent> local var address: %p value: %d\n", &local, local);
                printf("<parent> global var address: %p        value: %d\n", &global, global);
		sleep(20);
		int status;

		waitpid(pid, &status, 0);

		if (WIFEXITED(status)) {
			int exit_status = WEXITSTATUS(status);
			printf("Exit status of the child process: %d\n", exit_status);
		}
		if (WIFSIGNALED(status)) {
			printf("Signal\n");
		}

	}
	return 0;
}
