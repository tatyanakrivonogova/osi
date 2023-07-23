#include <unistd.h>
#include <sys/syscall.h>

void mywrite(int fd, const void *buf, size_t count) {
	syscall(SYS_write, fd, buf, count);
}


int main() {
        mywrite(1, "Hello syscall world!\n", 21);
        return 0;
}

