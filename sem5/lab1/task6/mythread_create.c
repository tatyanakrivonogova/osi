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
#define PAGE 4096
#define STACK_SIZE PAGE * 8
#define THREADS_NUM 10

typedef void *(*start_routine_t)(void*);

typedef struct _mythread {
		int mythread_id;
		start_routine_t start_routine;
		void* arg;
		void* retval;
		volatile int joined;
		volatile int exited;

		volatile int canceled;
		ucontext_t before_start_routine;
} mythread_struct_t;

typedef mythread_struct_t* mythread_t;
mythread_t structs[THREADS_NUM];
int tids[THREADS_NUM];

int mythread_startup(void* arg) { 
	mythread_struct_t* mythread = (mythread_struct_t*) arg;
	getcontext(&(mythread->before_start_routine));
	printf("mythread_startup: canceled %d\n", mythread->canceled);
	if (!mythread->canceled) {
		mythread->retval = mythread->start_routine(mythread->arg);
	}
	mythread->exited = 1;
	
	while(!mythread->joined) {
		sleep(1);
	}
	return 0;
}

void* create_stack(off_t size, int thread_num) {
	int stack_fd;
	void* stack;

	char* stack_file = malloc(sizeof("stack-***"));
	if (stack_file == NULL) return NULL;

	int result = snprintf(stack_file, sizeof(stack_file), "stack-%d", thread_num);
	if (result == 0) {
		free(stack_file);
		return NULL;
	}

	stack_fd = open(stack_file, O_RDWR | O_CREAT, 0660);
	if (stack_fd == -1) {
		free(stack_file);
		return NULL;
	}
	free(stack_file);

	ftruncate(stack_fd, 0);
	ftruncate(stack_fd, size);

	stack = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, stack_fd, 0);
	if (stack == NULL) {
		close(stack_fd);
		return NULL;
	}

	close(stack_fd);
	memset(stack, 0x7f, size);
	return stack;
}

int mythread_create(mythread_t* tid, start_routine_t start_routine, void* arg) {
	static int thread_num = 0;
	thread_num++;

	void* child_stack = create_stack(STACK_SIZE + sizeof(mythread_struct_t), thread_num);
	if (child_stack == NULL) return -1;
	printf("stack for thread %d : %p %p\n", thread_num, child_stack, child_stack + STACK_SIZE);
	mythread_struct_t* mythread = (mythread_struct_t*) (child_stack + STACK_SIZE);
	mythread->mythread_id = thread_num;
	mythread->start_routine = start_routine;
	mythread->arg = arg;
	mythread->exited = 0;
	mythread->joined = 0;
	mythread->canceled = 0;

	int child_pid = clone(mythread_startup, child_stack + STACK_SIZE, CLONE_VM | CLONE_FILES | CLONE_THREAD | CLONE_SIGHAND | SIGCHLD, (void*)mythread);
	if (child_pid == -1) {
		printf("clone failed: %s\n", strerror(errno));	
		munmap(child_stack, STACK_SIZE + sizeof(mythread_struct_t));
		return -1;
	}
	tids[thread_num] = child_pid;
	*tid = mythread;
	structs[thread_num] = mythread;
	return 0;
}

mythread_t mythread_self(void) {
        int id = gettid();
        int tid = 0;
        for (int i = 1; i < THREADS_NUM; ++i) {
                if (tids[i] == id) tid = i;
        }
        return structs[tid];
}

int mythread_join(mythread_t mytid, void** retval) {
	mythread_struct_t* mythread = mytid;
	
	while (!mythread->exited) {
		sleep(1);
	}
	*retval = mythread->retval;
	mythread->joined = 1;

	return 0;
}

void mythread_cancel(mythread_t tid) {
	mythread_struct_t* thread = tid;

	printf("thread cancel: set cancel for thread %d\n", thread->mythread_id);
	thread->retval = "canceled";
	thread->canceled = 1;
}

void mythread_testcancel(void) {
	mythread_struct_t* thread = mythread_self();
	//printf("thread_testcancel: check cancel %d\n", thread->canceled);
	if (thread->canceled) {
		setcontext(&(thread->before_start_routine));
	}
}
 
void* mythread(void* arg) {
	printf("mythread function is executing\n");
	char* str = (char*) arg;

	for (size_t i = 0; i < 5; ++i) {
		char string[128];
		snprintf(string, sizeof(string), "%s\n", str);
		write(1, string, sizeof(string));
		mythread_testcancel();
		sleep(1);
	}
	
	int tid = gettid();	
	return "bye";
}

int main() {
	printf("pid %d\n", getpid());
	mythread_t tid1;
	mythread_t tid2;

	int err = mythread_create(&tid1, mythread, "hello from thread1");
	if (err != 0) {
		printf("mythread_create failed for thread1\n");
		return -1;
	}
	err = mythread_create(&tid2, mythread, "hello from thread2");
        if (err != 0) {
                printf("mythread_create failed for thread2\n");
                return -1;
        }

	sleep(2);
	mythread_cancel(tid1);

	void* retval;
	err = mythread_join(tid1, (void**) &retval);
	if (err != 0) {
		printf("mythread_join failed for thread1\n");
		return -1;
	}
	printf("retval of thread1 in main: %s\n", (char*)retval);
	err = mythread_join(tid2, (void**) &retval);
        if (err != 0) {
                printf("mythread_join failed for thread2\n");
                return -1;
        }
        printf("retval of thread2 in main: %s\n", (char*)retval);
	
	return 0;
}
