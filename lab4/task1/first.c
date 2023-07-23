#include <stdio.h>
#include <unistd.h>
#include <malloc.h>

const int global_constant = 1;
int global_not_inited;
int global_inited = 100;
int global_array_not_inited[1000];
int global_array_inited[1000] = {0};

void f() {
	int local_inited_in_function = 3;
	int local_not_inited_in_function;
	static int static_inited_in_function = 5;
	static int static_not_inited_in_function;
	const int constant_in_function = 10;
	int* malloc_array_in_function = (int*)malloc(sizeof(int)*1000);
	int static_array_not_inited_in_function[1000];
	int static_array_inited_in_function[1000] = {0};

	printf("\nin function:\n");
	printf("local inited:		 address: %p	value: %d\n", &local_inited_in_function, local_inited_in_function);
	printf("local not inited:   	 address: %p   value: %d\n", &local_not_inited_in_function, local_not_inited_in_function);
	printf("static inited:  	 address: %p   value: %d\n", &static_inited_in_function, static_inited_in_function);
	printf("static not inited:  	 address: %p   value: %d\n", &static_not_inited_in_function, static_not_inited_in_function);
	printf("constant:  		 address: %p   value: %d\n", &constant_in_function, constant_in_function);
	printf("malloc array:            address: %p   value: %d\n", malloc_array_in_function, malloc_array_in_function[900]);
	printf("static array inited:     address: %p   value: %d\n", static_array_inited_in_function, static_array_inited_in_function[900]);
	printf("static array not inited: address: %p   value: %d\n", static_array_not_inited_in_function, static_array_not_inited_in_function[900]);
	free(malloc_array_in_function);
}

int main() {
	
	printf("global constant:   	 address: %p   value: %d\n", &global_constant, global_constant);
	printf("global not_inited:   	 address: %p   value: %d\n", &global_not_inited, global_not_inited);
	printf("global inited:   	 address: %p   value: %d\n", &global_inited, global_inited);
	printf("global array inited:     address: %p   value: %d\n", &global_array_inited, global_array_inited[900]);
        printf("global array not inited: address: %p   value: %d\n", &global_array_not_inited, global_array_not_inited[900]);
	f();

	printf("pid: %d\n", getpid());
	sleep(150);
	return 0;
}
