/*
 * Linux module for injecting shared object to the process by using ptrace.
 *
 * copied from: https://github.com/gaffe23/linux-inject
 * Released under the GPL v2+.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/user.h>
#include <wait.h>
#include <sys/time.h>
#include <dirent.h>
#include <stdint.h>
#include "inject.h"
#include "ptrace.h"
#include "utils.h"

#define SO_LIBC_NAME "libc.so.6"
#define SO_LIBDL_NAME "libdl.so.2"

#define WORD_SIZE sizeof(long)
#define WORD_ALIGN(x) (x & ~(WORD_SIZE-1)) + ((x & (WORD_SIZE-1)) ? WORD_SIZE : 0)


// following externals are inside of inject-content.S
extern void inject_so_loader(void);
extern void inject_loop_enter(void);
extern void inject_loop_exit(void);
extern const void inject_so_loader_end;
extern const void inject_loop_enter_end;
extern const void inject_contents_start;
extern const void inject_contents_end;


unsigned int get_tids(pid_t pid, pid_t **tids)
{
	DIR *d = NULL;
	struct dirent *dir;
	char path[64] = {0,};
	char strpid[10] = {0,};
	unsigned int thread_count = 0;

	snprintf(strpid, 10, "%d", pid);
	sprintf(path, "/proc/%d/task/", pid);
	d = opendir(path);
	*tids = malloc(50 * sizeof(pid_t));

	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (!strncmp(dir->d_name, ".", 1)) {
				continue;
			} else if (!strncmp(dir->d_name, "..", 2)) {
				continue;
			} else if (!strncmp(dir->d_name, strpid,
						strlen(strpid))) {
				continue;
			}
			(*tids)[thread_count++] = atoi(dir->d_name);
		}
		closedir(d);
	}
	return thread_count;
}

// following definition will different in each architecture.
#define MACHINE_STACK_SIZE 8
void seal_thread(pid_t tid, uintptr_t injected_addr)
{
	/*
	uintptr_t rsp;
	uint32_t loop_enter_off = (uintptr_t)inject_loop_enter
				 - (uintptr_t)&inject_contents_start;

	struct user_regs_struct regs;
	memset(&regs, 0, sizeof(struct user_regs_struct));
	*/
	ptrace_attach(tid);
	/*
	ptrace_getregs(tid, &regs);

	// prepare sealing. store current PC to top of stack.
	rsp = regs.rsp;
	regs.rsp = regs.rsp - MACHINE_STACK_SIZE;
	ptrace_write(tid, rsp, &regs.rip, MACHINE_STACK_SIZE);

	// move PC to loop room to loop until injecting libmcount is done.
	regs.rip = injected_addr + loop_enter_off + 2;
	ptrace_setregs(tid, &regs);
	*/
}

int seal_threads(pid_t pid, uintptr_t injected_addr,
		pid_t **tids, uint32_t thread_count)
{
	pid_t tid;
	unsigned int index = 0;

	if (thread_count > 0) {
		for(index=0;index < thread_count;index++) {
			tid = (*tids)[index];
			seal_thread(tid, injected_addr);
		}
	}

	return thread_count;
}

void release_thread(pid_t tid)
{
	/*
	long unlock = 1;
	siginfo_t sig;
	struct user_regs_struct regs;

	memset(&regs, 0, sizeof(struct user_regs_struct));
	ptrace_getregs(tid, &regs);

	// top of stack must be 0 to represent lock.
	ptrace_write(tid, regs.rsp, &unlock, MACHINE_STACK_SIZE);
	pr_dbg("[%d] Write to [%llx] \n", tid, regs.rsp);
	ptrace_getsiginfo(tid, &sig);
	*/
	ptrace(PTRACE_CONT, tid, NULL, NULL);
	ptrace(PTRACE_DETACH, tid, NULL, NULL);
}

void release_threads(pid_t **tids, int thread_count)
{
	int index;
	pid_t id;

	if (thread_count > 0) {
		for(index=0;index < thread_count;index++) {
			id = (*tids)[index];
			release_thread(id);
		}
	}
}

int inject(char *libname, pid_t pid)
{
	struct user_regs_struct oldregs, regs;
	struct user_regs_struct malloc_regs;
	struct user_regs_struct dlopen_regs;
	int lib_path_length, mypid;
	unsigned int thread_count;
	long my_libc_addr, my_libdl_addr;
	long malloc_addr, free_addr, dlopen_addr;
	long malloc_offset, free_offset, dlopen_offset;
	long target_libc_addr, target_libdl_addr;
	long target_malloc_addr, target_free_addr;
	long target_dlopen_addr, addr;
	unsigned long long target_buf, lib_addr;
	char *injected_area, *backup;
	char *lib_path = NULL;
	pid_t target = 0;
	// array of child thread ids.
	pid_t *tids = NULL;

	lib_path = realpath(libname, NULL);

	/*
	 * figure out the size of contents to be injected.
	 * so, we know how big of a buffer to allocate.
	 */
	uint32_t inject_size = (uintptr_t)&inject_contents_end
		- (uintptr_t)&inject_contents_start
		+ 1;

	/*
	 * Ptrace reads the word size at once. so, make sure that
	 * the space to inject is large enough to be read from ptrace.
	 */
	inject_size = WORD_ALIGN(inject_size);
	pr_dbg("to inject need size : %d\n", inject_size);

	if (!lib_path)
		pr_err("can't find file %s", libname);

	target = pid;
	lib_path_length = strlen(lib_path) + 1;
	mypid = getpid();
	my_libc_addr = get_libc_addr(mypid);
	my_libdl_addr = get_libdl_addr(mypid);

	/*
	 * find the addresses of the syscalls that we'd like to use
	 * inside the target, as loaded inside THIS process
	 * (i.e. NOT the target process)
	 */
	malloc_addr = get_function_addr(SO_LIBC_NAME, "malloc");
	free_addr = get_function_addr(SO_LIBC_NAME, "free");
	dlopen_addr = get_function_addr(SO_LIBDL_NAME, "dlopen");

	/*
	 * use the base address of libc to calculate offsets for
	 * the syscalls we want to use
	 */
	malloc_offset = malloc_addr - my_libc_addr;
	free_offset = free_addr - my_libc_addr;
	dlopen_offset = dlopen_addr - my_libdl_addr;


	/*
	 * get the target process' libc address and use it to find the
	 * addresses of the syscalls we want to use inside the
	 * target process
	 */
	target_libc_addr = get_libc_addr(target);
	target_libdl_addr = get_libdl_addr(target);
	if (target_libdl_addr == 0)
		pr_err("Libdl does not loaded from target process");
	target_malloc_addr = target_libc_addr + malloc_offset;
	target_free_addr = target_libc_addr + free_offset;
	target_dlopen_addr = target_libdl_addr + dlopen_offset;

	memset(&oldregs, 0, sizeof(struct user_regs_struct));
	memset(&regs, 0, sizeof(struct user_regs_struct));
	ptrace_attach(target);
	ptrace_getregs(target, &oldregs);
	memcpy(&regs, &oldregs, sizeof(struct user_regs_struct));

	/*
	 * since ELF align each section size by paging size, many section
	 * have padding at its tail. code be load shared object is
	 * injected to here.
	 */
	addr = get_inject_code_addr(target) - inject_size;
	pr_dbg("Address to inject : %llx\n", addr);

	/*
	 * now that we have an address to copy code to, set the target's
	 * rip to it. we have to advance by 2 bytes here because rip gets
	 * incremented by the size of the current instruction, and the
	 * instruction at the start of the function to inject always
	 * happens to be 2 bytes long.
	 */
	regs.rip = addr + 2;

	/*
	 * pass arguments to function inject_so_loader by loading
	 * them into the right registers. note that this will definitely
	 * only work on x64, because it relies on the x64 calling
	 * convention, in which arguments are passed via registers rdi,
	 * rsi, rdx, rcx, r8, and r9.
	 */
	regs.rdi = target_malloc_addr;
	regs.rsi = target_free_addr;
	regs.rdx = target_dlopen_addr;
	regs.rcx = lib_path_length;
	ptrace_setregs(target, &regs);

	// back up whatever data used to be at the address we want to modify.
	backup = malloc(inject_size * sizeof(char));
	ptrace_read(target, addr, backup, inject_size);

	/*
	 * set up a buffer to hold the code we're going to inject
	 * into the target process.
	 */
	injected_area = malloc(inject_size * sizeof(char));
	memset(injected_area, 0, inject_size * sizeof(char));

	// copy the contents of inject_contents to a buffer.
	memcpy(injected_area, &inject_contents_start, inject_size - 1);

	/*
	 * copy inject_so_loader inner assemly code to the target address
	 * inside the target process' address space.
	 */
	ptrace_write(target, addr, injected_area, inject_size);

	/*
	 * ptrace continue will make child thread get wake-up.
	 * this can be harmful during injection.
	 * therefore, make all child threads enter the loop
	 * until injecting done.
	 */
	// thread_count = get_tids(pid, &tids);
	// seal_threads(target, addr, &tids, thread_count);

	regs.rdi = target_malloc_addr;
	regs.rsi = target_free_addr;
	regs.rdx = target_dlopen_addr;
	regs.rcx = lib_path_length;
	ptrace_setregs(target, &regs);

	/*
	 * now that the new code is in place, let the target run
	 * our injected code.
	 */
	ptrace_cont(target);

	/*
	 * at this point, the target should have run malloc().
	 * check its return value to see if it succeeded, and
	 * bail out cleanly if it didn't.
	 */
	memset(&malloc_regs, 0, sizeof(struct user_regs_struct));
	ptrace_getregs(target, &malloc_regs);
	target_buf = malloc_regs.rax;

	if (target_buf == 0) {
		restore_state_and_detach(target, addr, backup, inject_size, oldregs);
		free(backup);
		free(injected_area);
		pr_err("malloc() failed to allocate memory");
	}

	/*
	 * if we get here, then malloc likely succeeded, so now we need
	 * to copy the path to the shared library we want to inject into
	 * the buffer that the target process just malloc'd. this is
	 * needed so that it can be passed as an argument to
	 * dlopen later on.
	 *
	 * read the current value of rax, which contains malloc's
	 * return value, and copy the name of our shared library to
	 * that address inside the target process.
	 */
	ptrace_write(target, target_buf, lib_path, lib_path_length);

	// continue the target's execution again in order to call dlopen.
	ptrace_cont(target);

	// check out what the registers look like after calling dlopen.
	memset(&dlopen_regs, 0, sizeof(struct user_regs_struct));
	ptrace_getregs(target, &dlopen_regs);
	lib_addr = dlopen_regs.rax;

	// if rax is 0 here, then dlopen failed, and we should bail out cleanly.
	if (lib_addr == 0) {
		restore_state_and_detach(target, addr, backup,
					 inject_size, oldregs);
		free(backup);
		free(injected_area);
		pr_err("dlopen() failed to load %s", libname);
	}


	// now check /proc/pid/maps to see whether injection was successful.
	if (check_loaded(target, libname))
		pr_dbg("'%s' successfully injected\n", libname);
	else
		pr_err("could not inject '%s'", libname);

	/*
	 * as a courtesy, free the buffer that we allocated inside the target
	 * process. we don't really care whether this succeeds, so don't
	 * bother checking the return value.
	 */
	ptrace_cont(target);
	pr_dbg("free was successfully ended. \n");

	/* release all child threads */
	// release_threads(&tids, thread_count);

	/*
	 * at this point, if everything went according to plan, we've loaded
	 * the shared library inside the target process, so we're done.
	 * restore the old state and detach from the target.
	 */
	restore_state_and_detach(target, addr, backup, inject_size, oldregs);
	free(backup);
	free(injected_area);
	return 0;
}

int main(int argc, char** argv)
{
	char *libname = argv[1];
	pid_t pid = atoi(argv[2]);

	inject(libname, pid);
}
