#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

void signalHandler(int signal) {
    printf("Signal %d received\n", signal);
}

void* threadFunction(void* arg) {
    // Установка обработчика сигналов
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);

    // Бесконечный цикл вывода строки
    while (1) {
        printf("Hello from the second thread!\n");
        sleep(1);
    }

    return NULL;
}

int main() {
    pthread_t thread;
    pthread_create(&thread, NULL, threadFunction, NULL);

    printf("Press Enter to send signals...\n");
    getchar();

    // Отправка сигналов во второй поток
    int result = pthread_kill(thread, SIGINT);
    if (result == 0) {
        printf("SIGINT signal sent to the second thread\n");
    } else {
        printf("Failed to send SIGINT signal to the second thread\n");
    }

    result = pthread_kill(thread, SIGQUIT);
    if (result == 0) {
        printf("SIGQUIT signal sent to the second thread\n");
    } else {
        printf("Failed to send SIGQUIT signal to the second thread\n");
    }

    pthread_join(thread, NULL);

    return 0;
}
