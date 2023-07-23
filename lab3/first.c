#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>


size_t find_name_begin(char* name, size_t name_size) {
    size_t pos = 0;
    while (pos < name_size && name[pos] != '/') {
        ++pos;
    }
    if (name[pos] == '/') ++pos;
    if (pos == name_size) pos = 0;
    return pos;
}

size_t find_name_end(char* name, int name_begin, size_t name_size) {
    size_t pos = name_begin;
    while (pos < name_size && name[pos] != '.') {
        ++pos;
    }
    
    return pos;
}

void reverse(char* begin, char* end) {
    for (; begin < end; ++begin, --end) {
        char tmp = *begin;
        *begin = *end;
        *end = tmp;
    }
}

void get_reverse_file_name(char* file_name, char** reverse_name) {
    size_t name_size = strlen(file_name);
    (*reverse_name) = (char*)malloc(name_size+1);
    strcpy((*reverse_name), file_name);
    
    size_t name_begin = 0;
    size_t name_end = find_name_end(file_name, name_begin, strlen(file_name));
    reverse((*reverse_name), (*reverse_name) + name_end - 1);
    (*reverse_name)[name_size] = '\0';
}

void write_reverse_file(FILE* fin, char* reverse_name) {
    FILE* fout = fopen(reverse_name, "w");

    fseek(fin, 0, SEEK_END);
    int size = ftell(fin);
   
    while ((--size) >= 0) {
	fseek(fin, size, SEEK_SET);
	char cur = fgetc(fin);
	fputc(cur, fout);
    }
   
    fclose(fout);
}

void create_reverse_dir(char *path_name) {
    size_t path_name_size = strlen(path_name);
    size_t name_begin = find_name_begin(path_name, path_name_size);
    size_t name_end = find_name_end(path_name, name_begin, path_name_size);
    size_t format = path_name_size - name_end;
   
    DIR *source_dir_pointer;
    struct dirent *current;
    struct stat stat_buf;

    if (stat(path_name, &stat_buf) != 0 || !S_ISDIR(stat_buf.st_mode)) {	
	fprintf(stderr, "Specified directory doesn't exist\n");
	return;
    }
 
    if ((source_dir_pointer = opendir(path_name)) == NULL) {
        fprintf(stderr, "Can`t open directory %s\n", path_name);
 	return;
    }
    
    if (chdir(path_name) == -1) printf("Directory %s unavailable\n", path_name);
    chdir("..");

    size_t dir_name_size = path_name_size - name_begin+1;
    char* dir_name = (char*)malloc(dir_name_size);
    char* reverse_dir_name = (char*)malloc(dir_name_size);
    for (size_t i = 0; i < dir_name_size; ++i) {
	reverse_dir_name[i] = path_name[name_begin+i];
	dir_name[i] = path_name[name_begin+i];
    }
    reverse_dir_name[dir_name_size] = '\0';
   
    reverse(reverse_dir_name, reverse_dir_name+dir_name_size-2-format);
    reverse_dir_name[dir_name_size] = '\0';
   

    if (mkdir(reverse_dir_name, 0700) != 0) {
        printf("Directory %s already exists\n", reverse_dir_name);
    } else {
        printf("Directory %s was created!\n", reverse_dir_name);
    }
    
    if (chdir(dir_name) == -1) fprintf(stderr, "Directory %s unavailable\n", dir_name);
    current = readdir(source_dir_pointer);
    while (current != NULL) {
	stat(current->d_name, &stat_buf);
        if (S_ISREG(stat_buf.st_mode)) {
            char* reverse_name;
	    get_reverse_file_name(current->d_name, &reverse_name);
	    
            FILE* fin = fopen(current->d_name, "r");
            if (fin == NULL) {
                perror(current->d_name);
                exit(-1);
            }
             
            chdir("..");
            chdir(reverse_dir_name);
	    
	    write_reverse_file(fin, reverse_name);
	    fclose(fin);
            if (reverse_name != NULL) free(reverse_name);
            chdir("..");
            if (chdir(dir_name) == -1);
        }
        current = readdir(source_dir_pointer);
    }
    chdir("..");
    closedir(source_dir_pointer);
}

int main(int argc, char** argv) {

    if (argc < 2) {
        fprintf(stderr, "Directory name not specified\n");
        return 0;
    }

    create_reverse_dir(argv[1]);
    return 0;
}

