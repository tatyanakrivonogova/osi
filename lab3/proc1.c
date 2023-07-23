#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define PAGE_SIZE 0x1000

static void print_page(uint64_t address, uint64_t data) {
    printf("0x%-16lx : pfn %-16lx soft-dirty %ld file-page/shared-anon %ld swapped %ld present %ld\n", address, data & 0x7fffffffffffff, 
		    (data >> 55) & 1, (data >> 61) & 1, (data >> 62) & 1, (data >> 63) & 1);
}

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "PID is not specified\n");
        return 0;
    }

    char file_name[100];
    errno = 0;
    int pid;
    if (strcmp(argv[1], "self") == 0) {
        pid = getpid();
    } else {
        pid = (int) strtol(argv[1], NULL, 0);
    }
    if (errno) {
        perror("strtol");
        return 1;
    }
    snprintf(file_name, sizeof(file_name), "/proc/%d/pagemap", pid);

    int fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    uint64_t start_address = strtoul(argv[2], NULL, 0);
    uint64_t end_address = strtoul(argv[3], NULL, 0);

    for (uint64_t i = start_address; i < end_address; i += PAGE_SIZE) {
        uint64_t data;
        uint64_t index = (i / PAGE_SIZE) * sizeof(data);
        if (pread(fd, &data, sizeof(data), index) != sizeof(data)) {
            perror("pread");
            break;
        }

        print_page(i, data);
    }

    close(fd);
    return 0;
}
