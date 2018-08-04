#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <wait.h>
#include <time.h>

#include "ptrace.h"
#include "utils.h"

void ptrace_attach(pid_t target)
{
	int status;

	if (ptrace(PTRACE_ATTACH, target, NULL, NULL) == -1)
		pr_err("ptrace(PTRACE_ATTACH) failed");

	if (waitpid(target, &status, WUNTRACED) != target)
		pr_err("waitpid(%d) failed", target);
}

void ptrace_detach(pid_t target)
{
	if (ptrace(PTRACE_DETACH, target, NULL, NULL) == -1)
		pr_err("ptrace(PTRACE_DETACH) failed");
}

void ptrace_getregs(pid_t target, struct REG_TYPE *regs)
{
	if (ptrace(PTRACE_GETREGS, target, NULL, regs) == -1)
		pr_err("ptrace(PTRACE_GETREGS) failed");
}

void ptrace_cont(pid_t target)
{
	struct timespec sleeptime = {2, 00000000};

	if (ptrace(PTRACE_CONT, target, NULL, NULL) == -1)
		pr_err("ptrace(PTRACE_CONT) failed");

	/*
	 * need enough time to finish libmount startup.
	 * this will need more time if user use verbose option like '-vvv'.
	 */
	nanosleep(&sleeptime, NULL);
	// make sure the target process received SIGTRAP after stopping.
	check_target_sig(target);
}

void ptrace_setregs(pid_t target, struct REG_TYPE *regs)
{
	if (ptrace(PTRACE_SETREGS, target, NULL, regs) == -1)
		pr_err("ptrace(PTRACE_SETREGS) failed");
}

void ptrace_getsiginfo(pid_t target, siginfo_t *targetsig)
{
	if (ptrace(PTRACE_GETSIGINFO, target, NULL, targetsig) == -1)
		pr_err("ptrace(PTRACE_GETSIGINFO) failed");
}

void ptrace_read(int pid, unsigned long addr, void *vptr, int len)
{
	int bytesRead = 0;
	int i = 0;
	long word = 0;
	long *ptr = (long *) vptr;

	while (bytesRead < len) {
		word = ptrace(PTRACE_PEEKTEXT, pid, addr + bytesRead, NULL);
		if (word == -1) {
			pr_err("ptrace(PTRACE_PEEKTEXT) failed %lx",
					addr + bytesRead, bytesRead);
		}
		bytesRead += sizeof(word);
		ptr[i++] = word;
	}
}

void ptrace_write(int pid, unsigned long addr, void *vptr, int len)
{
	int byteCount = 0;
	long word = 0;

	while (byteCount < len) {
		memcpy(&word, vptr + byteCount, sizeof(word));
		word = ptrace(PTRACE_POKETEXT, pid, addr + byteCount, word);
		if (word == -1)
			pr_err("ptrace(PTRACE_POKETEXT) failed");
		byteCount += sizeof(word);
	}
}

void check_target_sig(int pid)
{
	siginfo_t targetsig;

	// check the signal that the child stopped with.
	ptrace_getsiginfo(pid, &targetsig);

	/*
	 * if it wasn't SIGTRAP, then something bad happened
	 * (most likely a segfault).
	 */
	if (targetsig.si_signo != SIGTRAP) {
		if (targetsig.si_signo == SIGPROF) {
			pr_dbg("Received SIGPROF\n");

		}
		else {
			pr_dbg("instead of expected SIGTRAP, target stopped with "
				"signal %d: %s\n", targetsig.si_signo,
				strsignal(targetsig.si_signo));
			pr_dbg("sending process %d a SIGSTOP signal for debugging "
				"purposes\n", pid);
			ptrace(PTRACE_CONT, pid, NULL, SIGSTOP);
			pr_err("EXIT");
		}
	}
}

void restore_state_and_detach(pid_t target, unsigned long addr, void *backup,
			      int datasize, struct REG_TYPE oldregs)
{
	ptrace_setregs(target, &oldregs);
	ptrace_detach(target);
}
