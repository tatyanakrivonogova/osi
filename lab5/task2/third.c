#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс начал работу\n");
        printf("Дочерний процесс завершает работу\n");
        exit(0);
    } else if (pid > 0) {
        // Родительский процесс
        printf("Родительский процесс начал работу\n");
        sleep(10); // Даем дочернему процессу время на завершение
        printf("Родительский процесс завершает работу без ожидания дочернего процесса\n");
    } else {
        // Ошибка при создании дочернего процесса
        printf("Ошибка при создании дочернего процесса\n");
        exit(1);
    }
    return 0;
}