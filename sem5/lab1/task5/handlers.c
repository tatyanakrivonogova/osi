#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

void signalHandler1(int signal) {
	printf("\t\tSignal handler1 %d was received by thread with tid: %d\n", signal, gettid());
}

void signalHandler2(int signal) {
        printf("\t\tSignal handler2 %d was received by thread with tid: %d\n", signal, gettid());
}

void signalHandler3(int signal) {
        printf("\t\tSignal handler3 %d was received by thread with tid: %d\n", signal, gettid());
}

void* mythread1(void* arg) {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGINT);
        pthread_sigmask(SIG_UNBLOCK, &set, NULL);

        struct sigaction sa;
        sa.sa_handler = signalHandler1;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, NULL);

        sleep(50);

        return NULL;
}

void* mythread2(void* arg) {
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);

	struct sigaction sa;
	sa.sa_handler = signalHandler2;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);

	sleep(50);

	return NULL;
}

void* mythread3(void* arg) {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGINT);
        pthread_sigmask(SIG_UNBLOCK, &set, NULL);

        struct sigaction sa;
        sa.sa_handler = signalHandler3;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, NULL);

        sleep(50);

        return NULL;
}

int main() {
	pthread_t tid1, tid2, tid3;
	int err;
	printf("Hello from main with tid: %d\n", gettid());

	sigset_t all_signals_set;
        sigfillset(&all_signals_set);
        if (pthread_sigmask(SIG_BLOCK, &all_signals_set, NULL) != 0) {
                printf("Setting of sigmask in main failed\n");
        }

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


	int result;	
	result = pthread_kill(tid1, SIGINT);
        if (result == 0) {
                printf("\tSIGINT signal sent to the mythread1\n");
        } else {
                printf("\tFailed to send SIGINT signal to the mythread3\n");
        }

	result = pthread_kill(tid2, SIGINT);
        if (result == 0) {
                printf("\tSIGINT signal sent to the mythread2\n");
        } else {
                printf("\tFailed to send SIGINT signal to the mythread3\n");
        }

	result = pthread_kill(tid3, SIGINT);
        if (result == 0) {
                printf("\tSIGINT signal sent to the mythread3\n");
        } else {
                printf("\tFailed to send SIGINT signal to the mythread3\n");
        }

	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);
	pthread_join(tid3, NULL);

	return 0;
}

