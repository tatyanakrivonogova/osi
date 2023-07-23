#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

void print_buffer(char* buff) {
	for (size_t i = 0; i < 100; ++i) {
                printf("%c(%d) ", buff[i], buff[i]);
        }
	printf("\n");
}

void f() {
	char* buff1 = (char*)malloc(sizeof(char)*100);
	buff1[0] = 'h';
	buff1[1] = 'e';
	buff1[2] = 'l';
	buff1[3] = 'l';
	buff1[4] = 'o';
	
	printf("\nbuffer1 after filling:\n");
	print_buffer(buff1);

	free(buff1);
	printf("\nbuffer1 after free:\n");
	print_buffer(buff1);

	char* buff2 = (char*)malloc(sizeof(char)*100);
        buff2[0] = 'w';
        buff2[1] = 'o';
        buff2[2] = 'r';
        buff2[3] = 'l';
        buff2[4] = 'd';

	printf("\nbuffer2 after filling:\n");
	print_buffer(buff2);

	buff2 += 50;
	free(buff2);

	printf("\nbuffer2 after free:\n");
	print_buffer(buff2);
}

int main() {
	char* env = getenv("MY_ENV");
	printf("old value of MY_ENV = %s\n", env);

	if (setenv("MY_ENV", "GOODBUY", 1) != 0) {
		printf("error during setenv\n");
	}
	env = getenv("MY_ENV");
	printf("new value of MY_ENV = %s\n", env);
	printf("\n");

	f();	
	return 0;
}
