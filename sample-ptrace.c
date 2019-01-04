#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/inject.h"
#include "utils/ptrace.h"
#include "utils/utils.h"

int main(int argc, char** argv)
{
	struct user_regs_struct oldregs, regs;
	siginfo_t sig;
	long read = 0L;

	pid_t pid = atoi(argv[1]);

	printf("Trying to attach process %d\n", pid);
	ptrace_attach(pid);
	ptrace_getregs(pid, &oldregs);
	ptrace_setregs(pid, &oldregs);

	read = ptrace(PTRACE_PEEKTEXT, pid, oldregs.rip, 0L);
	printf("Read from process %lx\n", read);

	printf("Trying to write process %lx\n", read);
	ptrace(PTRACE_POKETEXT, pid, oldregs.rip, read);

	ptrace(PTRACE_GETSIGINFO, pid, 0L, &sig);
	printf("SIGINFO sigcode %d, signum : %s %d\n",
			sig.si_code,
			strsignal(sig.si_signo), sig.si_signo);


	printf("Trying to cont process %d\n", pid);
	ptrace(PTRACE_CONT, pid, 0L, 0L);

	ptrace(PTRACE_GETSIGINFO, pid, 0L, &sig);
	printf("SIGINFO sigcode %d, signum : %s %d\n",
			sig.si_code,
			strsignal(sig.si_signo), sig.si_signo);



	printf("Trying to detach process %d\n", pid);
	ptrace(PTRACE_DETACH, pid, 0L, 0L);

	getchar();
}
