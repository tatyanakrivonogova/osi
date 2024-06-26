#include <sys/ptrace.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include </usr/include/x86_64-linux-gnu/sys/user.h>

void child() {
	ptrace(PTRACE_TRACEME, 0, 0, 0);
	execl("/bin/echo", "/bin/echo", "Hello, world!", NULL);
	//perror("execl");
}

void parent(pid_t pid) {
	int status;
	waitpid(pid, &status, 0);
	ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD);

	while (!WIFEXITED(status)) {

		struct user_regs_struct state;

		ptrace(PTRACE_SYSCALL, pid, 0, 0);
		waitpid(pid, &status, 0);

		// at syscall
		if (WIFSTOPPED(status) && (WSTOPSIG(status) & 0x80)) {
			ptrace(PTRACE_GETREGS, pid, 0, &state);
			printf("SYSCALL %lld at %08llx\n", state.orig_rax, state.rip);

			// skip after syscall
			ptrace(PTRACE_SYSCALL, pid, 0, 0);
			waitpid(pid, &status, 0);
		}
	}
}

int main(int argc, char *argv[]) {
	pid_t pid = fork();
	if (pid)
		parent(pid);
	else
		child();
	return 0;
}
