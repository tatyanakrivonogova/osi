#include <stdio.h>
#include <unistd.h>

void try_to_open(char* file_name) {
	FILE* file = fopen(file_name, "r");
        if (file == NULL) {
                perror("Impossible to open file\n");
                return;
        } else {
		printf("Good!\n");
	}
	fclose(file);
}

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("File name is not specified\n");
		return -1;
	}

	printf("uid: %d		euid: %d\n", getuid(), geteuid());
	try_to_open(argv[1]);

	if (setuid(getuid()) != 0) {
		perror("Failed to set uid\n");
		return -1;
	}
	
	printf("uid: %d         euid: %d\n", getuid(), geteuid());
	try_to_open(argv[1]);

	return 0;
}
