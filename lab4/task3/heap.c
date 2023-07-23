#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

const int MAX_POOLS = 1000;
const long int BUF_SIZE = 104890368;
 
static char* BUFFER;
static unsigned long UNUSED = BUF_SIZE;
static char* pools[MAX_POOLS];
static unsigned int POOLS_COUNT = 1;
static unsigned int pools_size[MAX_POOLS];

static char* blocks[MAX_POOLS];
static unsigned int BLOCKS_COUNT = 0;
static unsigned int block_size[MAX_POOLS];
static unsigned int RIGHT_BLOCK;

static int ERROR = 0;

#define NO_MEMORY 1
#define BLOCK_NOT_FOUND 2


void defrag(void) {
        char* p = BUFFER;
        char* t;
        char* tmp;

        for (unsigned long int i = 0; i < RIGHT_BLOCK; ++i) {
		t = blocks[i];
                if (t == BUFFER) {
			if (block_size[i] == BUF_SIZE) return;
                        p = (char* )(blocks[i] + block_size[i] + 1);
                        continue;
                }
		
                tmp = p;
                t = blocks[i];
                for (unsigned long int k = 0; k < block_size[i]; ++k) {
                        *p = *t;
                        p++;
                        t++;
                }
                blocks[i] = tmp;
        }

        POOLS_COUNT = 1;
        pools[0] = p;
        UNUSED = BUF_SIZE - (unsigned long)(p - BUFFER);
        pools_size[0] = UNUSED;
        RIGHT_BLOCK = 0;
        return;
}

void alloc_init(void) {
	size_t pagesize = getpagesize();
	size_t size = (BUF_SIZE % pagesize == 0) ? (BUF_SIZE) : (BUF_SIZE + BUF_SIZE - BUF_SIZE % pagesize);
	BUFFER = (char* )mmap(0, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (BUFFER == MAP_FAILED) {
		printf("Mmap error\n");
		_exit(-1);
	}
	pools[0] = BUFFER;
	pools_size[0] = UNUSED;
}

char* my_malloc(unsigned long size) {
	char* p;
 
	if(size > UNUSED) {
		defrag();
	}
	if (size > UNUSED) {
		ERROR = NO_MEMORY;
		return 0;     
	}
   
	p = 0;
	unsigned long int k;
	for (unsigned long int i = 0; i < POOLS_COUNT; ++i) {
		if (size <= pools_size[i]) {
			p = pools[i];
			k = i;
			break;
		}
	}
 
	if (p == 0) {
		ERROR = NO_MEMORY;
		return 0;
	}   
 
	blocks[BLOCKS_COUNT] = p;
	block_size[BLOCKS_COUNT] = size;
	++BLOCKS_COUNT;
	++RIGHT_BLOCK;
	pools[k] = (char*)(p + size + 1);
	pools_size[k] = pools_size[k]-size;

	UNUSED -= size;
	return p;  
}

 
int my_free(char* block) {
	char* p = 0;
	unsigned int k;
	for (unsigned long int i = 0; i < RIGHT_BLOCK; ++i) {
		if (block == blocks[i]) {
			p = blocks[i];
			k = i;
			break;
		}
	}

	if (p == 0) {
		ERROR = BLOCK_NOT_FOUND;
		return BLOCK_NOT_FOUND;
	}
 
	blocks[k] = 0;  
	--BLOCKS_COUNT;
	pools[POOLS_COUNT] = block;
	pools_size[POOLS_COUNT] = block_size[k];
	++POOLS_COUNT;
	UNUSED += block_size[k];
	return 0;
}

void print_status(char *str) {	
	sprintf(str,"Status:\nAvailable: %ld of %ld bytes\nCount of blocks: %d\n", UNUSED, BUF_SIZE, BLOCKS_COUNT);
}

int check_error(void) {
	int error = ERROR;
	ERROR = 0;
	return error;
}

void print_error(int error_code) {
	if (error_code == 1) {
                printf("Not enough memory\n");
        }
	if (error_code == 2) {
                printf("Invalid pointer for free\n");
        }
}

int main() {
	alloc_init();

	char* array1 = my_malloc(104890368);
	printf("pointer for array1: %p\n", array1);
	int error_code = check_error();
	printf("error code: %d\n", error_code);
	if (error_code != 0) {
		print_error(error_code);
	}

	char* array2 = my_malloc(100);
	printf("pointer for array2: %p\n", array2);
	error_code = check_error();
        printf("error code: %d\n", error_code);
        if (error_code != 0) {
                print_error(error_code);
        }

	my_free(array1);
	error_code = check_error();
	if (error_code != 0) {
                print_error(error_code);
        }
	return 0;
}
