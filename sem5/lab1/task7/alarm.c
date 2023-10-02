#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sched.h>
#include <errno.h>
#include <signal.h>
#include <ucontext.h>
#include <assert.h>
#define PAGE 4096
#define STACK_SIZE PAGE * 8
#define NAME_SIZE 256
#define MAX_THREADS 10

typedef struct uthread {
	char name[NAME_SIZE];
	void (*thread_func)(void*);
	void* arg;
	ucontext_t uctx;
} uthread_t;

uthread_t* uthreads[MAX_THREADS];

int uthread_count = 0;
int uthread_cur = 0;


void* create_stack(off_t size, char* file_name) {
	void* stack;

	if (file_name) {
		int stack_fd;
		stack_fd = open(file_name, O_RDWR | O_CREAT, 0660);
		if (stack_fd == -1) {
			return NULL;
		}

		ftruncate(stack_fd, 0);
		ftruncate(stack_fd, size);

		stack = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_STACK, stack_fd, 0);
		if (stack == NULL) {
			close(stack_fd);
			return NULL;
		}

		close(stack_fd);
	} else {
		stack = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0);
		if (stack == NULL) {
			return NULL;
		}
	}
	memset(stack, 0x7f, size);
	return stack;
}

void start_thread(int arg) {
	uthread_t* uthread = uthreads[arg];
	//uthread = (uthread_t*) ((((long long int)arg1) << 32) | ((long long int) arg2));
	printf("\t\t\t struct address: %p\n", uthread);
	
	printf("start_thread: name: '%s' thread_func: %p; arg: %p\n",uthread->name, uthread->thread_func, uthread->arg);
        uthread->thread_func(uthread->arg);
}

void uthread_create(uthread_t** ut, char* name, void (*thread_func)(void*), void* arg) {
	char* stack;
	uthread_t* new_ut;
	int err;

	assert(name);

	stack = create_stack(STACK_SIZE, name);
	new_ut = (uthread_t*) (stack + STACK_SIZE - sizeof(uthread_t));

	err = getcontext(&new_ut->uctx);
	if (err == -1) {
		printf("uthread_create: getcontext() failed: %s\n", strerror(errno));
		exit(2);
	}

	new_ut->uctx.uc_stack.ss_sp = stack;
	new_ut->uctx.uc_stack.ss_size = STACK_SIZE - sizeof(uthread_t);
	new_ut->uctx.uc_link = NULL;

	uthreads[uthread_count] = new_ut;

	printf("\t\t\t\t struct size: %ld %p\n", sizeof(&new_ut), &new_ut);
	makecontext(&new_ut->uctx, start_thread, 1, uthread_count);

	new_ut->thread_func = thread_func;
	new_ut->arg = arg;
	strncpy(new_ut->name, name, NAME_SIZE);

	//uthreads[uthread_count] = new_ut;
	uthread_count++;
	*ut = new_ut;
}


void schedule(void) {
	int err;
	ucontext_t* cur_ctx, * next_ctx;

	cur_ctx = &(uthreads[uthread_cur]->uctx);
	uthread_cur = (uthread_cur + 1) % uthread_count;
	next_ctx = &(uthreads[uthread_cur]->uctx);

	err = swapcontext(cur_ctx, next_ctx);
	if (err == -1) {
		printf("schedule: swapcontext() failed: %s\n", strerror(errno));
		exit(1);
	}
}


void uthread_scheduler(int sig, siginfo_t* si, void* u) {
	printf("uthread_scheduler: \n");
	alarm(1);
	schedule();
}

void mythread1(void* arg) {
	int i;
	char* myarg = (char*) arg;

	printf("mythread1: started: arg '%s', %p %d %d %d\n", myarg, &i, getpid(), getppid(), gettid());

	for (int i = 0; i < 100; i++) {
		printf("mythread1: arg '%s' %p\n", myarg, &i);
		sleep(1);
	}

	printf("mythread1: finished\n");
}

void mythread2(void* arg) {
        int i;
        char* myarg = (char*) arg;

        printf("mythread2: started: arg '%s', %p %d %d %d\n", myarg, &i, getpid(), getppid(), gettid());

        for (int i = 0; i < 100; i++) {
                printf("mythread2: arg '%s' %p\n", myarg, &i);
                sleep(1);
        }

        printf("mythread2: finished\n");
}

void mythread3(void* arg) {
        int i;
        char* myarg = (char*) arg;

        printf("mythread3: started: arg '%s', %p %d %d %d\n", myarg, &i, getpid(), getppid(), gettid());

        for (int i = 0; i < 100; i++) {
                printf("mythread3: arg '%s' %p\n", myarg, &i);
                sleep(1);
        }

        printf("mythread3: finished\n");
}

int main() {
	uthread_t* ut[3];
	char* arg[] = {"arg 11111", "arg 22222", "arg 33333"};
	struct sigaction act;
	//stack_t stk;
	int err;

	uthread_t main_thread;
	uthreads[0] = &main_thread;
	uthread_count = 1;

	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_ONSTACK | SA_SIGINFO;
	act.sa_sigaction = uthread_scheduler;
	//sigaction(SIGUSR1, &act, NULL);
	sigaction(SIGALRM, &act, NULL);
	alarm(1);

	printf("main: started: %d %d %d\n", getpid(), getppid(), gettid());

	uthread_create(&ut[0], "user-thread-1", mythread1, (void*) arg[0]);
	uthread_create(&ut[1], "user-thread-2", mythread2, (void*) arg[1]);
	uthread_create(&ut[2], "user-thread-3", mythread3, (void*) arg[2]);

	while (1) {
		schedule();
		//printf("main: while: %d %d %d\n", getpid(), getppid(), gettid());
	}

	printf("main: finished: %d %d %d\n", getpid(), getppid(), gettid());
	return 0;
}
