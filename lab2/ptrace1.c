#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
int main()
{
    int pid;        // PID отлаживаемого процесса
    int wait_val;   // Сюда wait записывает
                     // возвращаемое значение
    long long counter = 1; // Счетчик трассируемых инструкций
// Расщепляем процесс на два
// Родитель будет отлаживать потомка
// (обработка ошибок для наглядности опущена)
switch (pid = fork())
{
    case 0:         // Дочерний процесс (его отлаживают)
        // Папаша, ну-ка, потрассируй меня!
        ptrace(PTRACE_TRACEME, 0, 0, 0);
    // Вызываем программу, которую надо отрассировать
    // (для программ, упакованных шифрой, это не сработает)
        execl("/bin/ls", "ls", (char *)NULL);
        break;
    default:    // Родительский процесс (он отлаживает)
        // Ждем, пока отлаживаемый процесс
        // не перейдет в состояние останова
        wait(&wait_val);
    // Трассируем дочерний процесс, пока он не завершится
        while (WIFSTOPPED(wait_val) /* 1407 */)
        {
            if (ptrace(PTRACE_SINGLESTEP, pid, (caddr_t) 1, 0)) break;
            wait(&wait_val);
            counter++;
	    printf("*");
        }
    }

    printf("== %lld\n", counter);
    return 0;
}
