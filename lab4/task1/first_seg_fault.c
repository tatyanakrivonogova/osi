#include <stdio.h>

int* f() {
	int a = 10;
	return &a;
}

int main() {
	int* result = f();

	printf("\naddress:	%p\n", result);
	printf("value:       %d\n", (*result));
	return 0;
}
