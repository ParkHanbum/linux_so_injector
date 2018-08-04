/*
 * Linux module for injecting shared object to the process by using ptrace.
 *
 * copied from: https://github.com/gaffe23/linux-inject
 * Released under the GPL v2+.
 */
#include <sys/ptrace.h>
#include <sys/user.h>
#include <wait.h>
#include <time.h>

#ifdef ARM
	#define REG_TYPE user_regs
#else
	#define REG_TYPE user_regs_struct
#endif

void ptrace_attach(pid_t target);
void ptrace_detach(pid_t target);
void ptrace_getregs(pid_t target, struct REG_TYPE *regs);
void ptrace_cont(pid_t target);
void ptrace_setregs(pid_t target, struct REG_TYPE *regs);
void ptrace_getsiginfo(pid_t target, siginfo_t *targetsig);
void ptrace_read(int pid, unsigned long addr, void *vptr, int len);
void ptrace_write(int pid, unsigned long addr, void *vptr, int len);
void check_target_sig(int pid);
void restore_state_and_detach(pid_t target, unsigned long addr, void *backup, int datasize, struct REG_TYPE oldregs);
