#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>


void create_dir(char* dir_name) {

    if (mkdir(dir_name, 0700) != 0) {
        printf("Directory already exists\n");
    } else {
	printf("Directory was created!\n");
    }
    return;
}

void print_dir(char* dir_name) {
    DIR *dir_pointer;
    struct dirent *current;

    if ((dir_pointer = opendir(dir_name)) == NULL) {
        fprintf(stderr, "Can`t open directory %s\n", dir_name);
        return;
    }

    chdir(dir_name);
    current = readdir(dir_pointer);
    while (current != NULL) {
	printf("%s\n", current->d_name);
	current = readdir(dir_pointer);
    }
    chdir("..");
    closedir(dir_pointer);
}

int dir_is_empty(DIR* dir_pointer) {
    int files_found = 0;
    struct dirent * dirent;
    while(dirent = readdir(dir_pointer)) {
        if(strcmp(dirent->d_name, ".") || strcmp(dirent->d_name, "..")) {
            files_found = 1;
            break;
        }
    }
    return !files_found;
}

void delete_dir(char* dir_name) {
    DIR* dir_pointer;
    struct dirent *current;
    struct stat stat_buf;

    if ((dir_pointer = opendir(dir_name)) == NULL) {
        fprintf(stderr, "Can`t open directory %s\n", dir_name);
        return;
    }

    chdir(dir_name);
    current = readdir(dir_pointer);
    while (!dir_is_empty(dir_pointer) && current != NULL) {
	stat(current->d_name, &stat_buf);
        if (S_ISDIR(stat_buf.st_mode)) {
            if (strcmp(current->d_name, ".") == 0 || strcmp(current->d_name, "..") == 0 )
                continue;
            delete_dir(current->d_name);
	} else {
	    if (remove(current->d_name) != 0) {
		printf("Can't delete file %s\n", current->d_name);
	    } else {
		printf("File %s was deleted\n", current->d_name);
	    }
	}
	current = readdir(dir_pointer);
    }

    chdir("..");
    closedir(dir_pointer);
    if (rmdir(dir_name) == -1) {
	printf("Can't delete directory %s\n", dir_name);
    } else {
	printf("Directory %s was deleted\n", dir_name);
    }
}

void create_file(char* file_name) {
    FILE* f = fopen(file_name, "w");
    fclose(f);
    printf("File %s created\n", file_name);
}

void print_file(char* file_name) {
    FILE* f = fopen(file_name, "r");
    if (f == NULL) {
	fprintf(stderr, "Impossible to read file %s\n", file_name);
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    for (size_t i = 0; i < size; ++i) {
	char cur;
	cur = getc(f);
	printf("%c", cur);
    }
    printf("\n");

    fclose(f);
}

void delete_file(char* file_name) {
    if (remove(file_name) != 0) {
	fprintf(stderr, "Impossible to delete file %s\n", file_name);
    } else {
	printf("File %s was deleted\n", file_name);
    }
}

void create_sym_link(char* file_name) {
    if (symlink(file_name, "symlink") != 0) {
	fprintf(stderr, "Impossible to create sym link %s\n", file_name);
    } else {
	printf("Sym link to file %s was created\n", file_name);
    }
}

void print_sym_link(char* link_name) {
    char buf[256];
    size_t size = readlink(link_name, buf, 256);
    if (size == -1) {
	fprintf(stderr, "Impossible to read sym link %s\n", link_name);
	return;
    }
    write(1, buf, size);
    printf("\n");
}

void print_file_sym_link(char* link_name) {
    FILE* f = fopen(link_name, "r");
    if (f == NULL) {
        fprintf(stderr, "Impossible to read file %s\n", link_name);
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    for (size_t i = 0; i < size; ++i) {
        char cur;
        cur = getc(f);
        printf("%c", cur);
    }
    printf("\n");

    fclose(f);
}

void delete_sym_link(char* link_name) {
    if (unlink(link_name) != 0) {
	fprintf(stderr, "Impossible to delete sym link %s\n", link_name);
    } else {
	printf("Sym link %s was deleted\n", link_name);
    }
}

void create_hard_link(char* file_name) {
    if (link(file_name, "hardlink") != 0) {
	fprintf(stderr, "Impossible to create hard link %s\n", file_name);
    } else {
	printf("Hrd link to %s was created\n", file_name);
    }
}

void delete_hard_link(char* link_name) {
    if (unlink(link_name) != 0) {
	fprintf(stderr, "Impossible to delete hard link %s\n", link_name);
    } else {
	printf("Hard link %s was deleted\n", link_name);
    }
}

void get_mode(char* file_name) {
    struct stat stat_buf;
    stat(file_name, &stat_buf);
    mode_t mode = stat_buf.st_mode;

    char str_mode[10];
    const char all_mode[] = "rwxrwxrwx";
    for (size_t i = 0; i < 9; ++i) {
        str_mode[i] = (mode & (1 << (8-i))) ? all_mode[i] : '-';
    }
    str_mode[9] = '\0';

    printf("mode: %s\n", str_mode);
    printf("number of hard links: %ld\n", stat_buf.st_nlink);
}

void change_file_mode(char* file_name, char* str_mode) {
    mode_t mode;
    mode = strtol(str_mode, 0, 8);

    struct stat stat_buf;
    if (stat(file_name, &stat_buf) != 0) {
	fprintf(stderr, "No such file or directory");
	return;
    }
    
    if (chmod(file_name, mode) < 0) {
        fprintf(stderr, "Impossible to set mode %s for file %s\n", str_mode, file_name);
        return;
    } else {
	printf("Mode for file %s was changed\n", file_name);
    }
}

int main(int argc, char** argv) {

    if (argc < 2) {
	fprintf(stderr, "Not enough arguments!\n");
	return 0;
    }

    if (strcmp(argv[0], "./create_dir") == 0) {	create_dir(argv[1]); return 0; }
    if (strcmp(argv[0], "./print_dir") == 0) { print_dir(argv[1]); return 0; }
    if (strcmp(argv[0], "./delete_dir") == 0) { delete_dir(argv[1]); return 0; }
    if (strcmp(argv[0], "./create_file") == 0) { create_file(argv[1]); return 0; }
    if (strcmp(argv[0], "./print_file") == 0) { print_file(argv[1]); return 0; }
    if (strcmp(argv[0], "./delete_file") == 0) { delete_file(argv[1]); return 0; }
    if (strcmp(argv[0], "./create_sym_link") == 0) { create_sym_link(argv[1]); return 0; }
    if (strcmp(argv[0], "./print_sym_link") == 0) { print_sym_link(argv[1]); return 0; }
    if (strcmp(argv[0], "./print_file_sym_link") == 0) { print_file_sym_link(argv[1]); return 0; }
    if (strcmp(argv[0], "./delete_sym_link") == 0) { delete_sym_link(argv[1]); return 0; }
    if (strcmp(argv[0], "./create_hard_link") == 0) { create_hard_link(argv[1]); return 0; }
    if (strcmp(argv[0], "./delete_hard_link") == 0) { delete_hard_link(argv[1]); return 0; }
    if (strcmp(argv[0], "./get_mode") == 0) { get_mode(argv[1]); return 0; }
    if (strcmp(argv[0], "./change_file_mode") == 0) { 
	if (argc < 3) {
	    fprintf(stderr, "Not enough arguments!\n");
            return 0;
	}
	change_file_mode(argv[1], argv[2]); }
    return 0;
}
