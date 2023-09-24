#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

void signalHandler(int signal) {
	printf("\t\tSIGINT was received by thread with tid: %d\n", gettid());
	pthread_exit(0);
}

void *mythread1(void *arg) {
	sigset_t all_signals_set;
        sigfillset(&all_signals_set);
        if (pthread_sigmask(SIG_BLOCK, &all_signals_set, NULL) != 0) {
		printf("Setting of sigmask in mythread1 failed\n");
	}

	for (int i = 0; i < 10; ++i) {
		printf("Hello from mythread1 with tid: %d!\n", gettid());
		sleep(1);
	}
	return NULL;
}

void* mythread2(void* arg) {
	struct sigaction sa;
	sa.sa_handler = signalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);

	for (int i = 0; i < 10; ++i) {
	        printf("Hello from mythread2 with tid: %d!\n", gettid());
        	sleep(1);
	}

	return NULL;
}

void* mythread3(void* arg) {
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGQUIT);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	
	for (int i = 0; i < 2; ++i) {
		printf("Hello from mythread3 with tid: %d!\n", gettid());
        	sleep(1);
	}

	int sig;
	sigwait(&set, &sig);

	printf("\t\tSIGQUIT was received by thread with tid: %d\n", gettid());
	return NULL;
}

int main() {
	pthread_t tid1, tid2, tid3;
	int err;
	printf("Hello from main with tid: %d\n", gettid());
	
	err = pthread_create(&tid1, NULL, mythread1, NULL);
	if (err) {
		printf("main: pthread_create() failed for thread1: %s\n", strerror(err));
		return -1;
	}
	err = pthread_create(&tid2, NULL, mythread2, NULL);
        if (err) {
        	printf("main: pthread_create() failed for thread2: %s\n", strerror(err));
                return -1;
        }
	err = pthread_create(&tid3, NULL, mythread3, NULL);
        if (err) {
        	printf("main: pthread_create() failed for thread3: %s\n", strerror(err));
                return -1;
        }


	sleep(5);
	
	int result = pthread_kill(tid1, SIGINT);
    	if (result == 0) {
        	printf("\tSIGINT signal sent to the mythread1\n");
    	} else {
    		printf("\tFailed to send SIGINT signal to the mythread1\n");
    	}
	result = pthread_kill(tid1, SIGQUIT);
    	if (result == 0) {
        	printf("\tSIGQUIT signal sent to the mythread1\n");
    	} else {
        	printf("\tFailed to send SIGQUIT signal to the mythread1\n");
   	}


	result = pthread_kill(tid2, SIGINT);
        if (result == 0) {
                printf("\tSIGINT signal sent to the mythread2\n");
        } else {
                printf("\tFailed to send SIGINT signal to the mythread2\n");
        }


	result = pthread_kill(tid3, SIGQUIT);
        if (result == 0) {
                printf("\tSIGQUIT signal sent to the mythread3\n");
        } else {
                printf("\tFailed to send SIGQUIT signal to the mythread3\n");
        }


	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);
	pthread_join(tid3, NULL);

	return 0;
}

