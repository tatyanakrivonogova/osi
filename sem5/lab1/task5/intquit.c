#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

void *mythread1(void *arg) {
	sigset_t all_signals_set;
        sigfillset(&all_signals_set);
        pthread_sigmask(SIG_BLOCK, &all_signals_set, NULL);

	for (int i = 0; i < 10; ++i) {
		printf("mythread [%d %d %d]: Hello from mythread1!\n", getpid(), getppid(), gettid());
		sleep(1);
	}
	return NULL;
}

int main() {
	pthread_t tid;
	int err;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

	err = pthread_create(&tid, NULL, mythread1, NULL);
	if (err) {
	    printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}
	sleep(5);
	
	int result = pthread_kill(tid, SIGINT);
    	if (result == 0) {
        	printf("SIGINT signal sent to the second thread\n");
    	} else {
    		printf("Failed to send SIGINT signal to the second thread\n");
    	}
	result = pthread_kill(tid, SIGQUIT);
    	if (result == 0) {
        	printf("SIGQUIT signal sent to the second thread\n");
    	} else {
        	printf("Failed to send SIGQUIT signal to the second thread\n");
   	}

	pthread_join(tid, NULL);

	return 0;
}

