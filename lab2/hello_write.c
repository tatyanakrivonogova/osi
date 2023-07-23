#include <unistd.h>

int main() {
	write(1, "Hello write world!\n", 19);
	return 0;
}
