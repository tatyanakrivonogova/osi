#include <stdio.h>

int* f() {
	int a = 10;
	int* p = &a;
	return p;
}

int main() {
	int* result = f();

	printf("\naddress:	%p value:	%d\n", result, (*result));
	return 0;
}
