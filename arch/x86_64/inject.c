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
#include "utils/inject.h"
#include "utils/ptrace.h"
#include "utils/utils.h"

#define SO_LIBC_NAME "libc.so.6"
#define LIBC_DLOPEN_NAME	"__libc_dlopen_mode"
#define SO_LIBDL_NAME "libdl.so.2"
#define LIBDL_DLOPEN_NAME	"dlopen"

#define MAX_PATH_LENGTH		128
#define MAX_THREAD_COUNT	64

#define WORD_SIZE sizeof(long)
#define WORD_ALIGN(x)	ALIGN(x, WORD_SIZE)

// following externals are inside of inject-content.S
extern void inject_so_loader(void);
extern char inject_so_path[128];
extern long long inject_so_loader_ret;
extern long long inject_dlopen_addr;
extern const void inject_contents_start;
extern const void inject_contents_end;

unsigned int get_tids(pid_t pid, pid_t **tids)
{
	DIR *d = NULL;
	struct dirent *dir;
	char path[64] = {0,};
	char strpid[10] = {0,};
	unsigned int thread_count = 0;

	snprintf(strpid, sizeof(strpid), "%d", pid);
	snprintf(path, sizeof(path), "/proc/%d/task/", pid);
	d = opendir(path);
	*tids = xcalloc(sizeof(pid_t), MAX_THREAD_COUNT);

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

/*
 * While injecting a shared object, attaching to all the child threads
 * to puts them into a suspended state.
 */
void suspend_child_threads(pid_t pid, uintptr_t injected_addr,
			  pid_t **tids, uint32_t thread_count)
{
	pid_t tid;
	uint32_t index;

	for (index=0;index < thread_count;index++) {
		tid = (*tids)[index];
		ptrace_attach(tid);
	}
}

void release_child_threads(pid_t pid, uintptr_t injected_addr,
			  pid_t **tids, uint32_t thread_count)
{
	pid_t tid;
	uint32_t index;

	for (index=0;index < thread_count;index++) {
		tid = (*tids)[index];
		ptrace_detach(tid);
	}
}


int inject(char *libname, pid_t pid)
{
	struct user_regs_struct oldregs, regs;
	int lib_path_length, mypid;
	unsigned int thread_count;

#ifdef USE_LIBC_DLOPEN
	long my_libc_addr;
#else
	long my_libdl_addr;
#endif
	long target_lib_addr, dlopen_addr, dlopen_offset, addr;
	long target_dlopen_addr;
	char *injected_area;
	char *lib_path = NULL;
	pid_t target = 0;
	// array of child thread ids.
	pid_t *tids = NULL;

	/*
	 * figure out the size of contents to be injected.
	 * so, we know how big of a buffer to allocate.
	 */
	uintptr_t inject_end = (uintptr_t)&inject_contents_end;
	uintptr_t inject_begin = (uintptr_t)&inject_contents_start;
	uint32_t inject_size = inject_end - inject_begin + 1;
	pr_dbg("size of code to inject %d\n", inject_size);

	/*
	 * Ptrace reads the word size at once. so, make sure that
	 * the space to inject is large enough to be read from ptrace.
	 */
	inject_size = WORD_ALIGN(inject_size);

	lib_path = realpath(libname, NULL);
	if (!lib_path)
		pr_err("can't find file %s", libname);

	target = pid;
	lib_path_length = strlen(lib_path) + 1;
	mypid = getpid();

#ifdef USE_LIBC_DLOPEN
	my_libc_addr = get_libc_addr(mypid);
	dlopen_addr = get_function_addr(SO_LIBC_NAME, LIBC_DLOPEN_NAME);
	dlopen_offset = dlopen_addr - my_libc_addr;
	target_lib_addr = get_libc_addr(target);
#else
	my_libdl_addr = get_libdl_addr(mypid);
	dlopen_addr = get_function_addr(SO_LIBDL_NAME, LIBDL_DLOPEN_NAME);
	dlopen_offset = dlopen_addr - my_libdl_addr;
	target_lib_addr = get_libdl_addr(target);
#endif

	/*
	 * get the target process' libdl address and use it to find the
	 * address of the dlopen we want to use inside the target process
	 */

	if (target_lib_addr == 0)
		pr_err("Cannot inject library if target process does not load libdl.so");

	target_dlopen_addr = target_lib_addr + dlopen_offset;

	memset(&oldregs, 0, sizeof(struct user_regs_struct));
	memset(&regs, 0, sizeof(struct user_regs_struct));
	ptrace_attach(target);
	ptrace_getregs(target, &oldregs);
	memcpy(&regs, &oldregs, sizeof(struct user_regs_struct));

	inject_dlopen_addr = target_dlopen_addr;
	memset(inject_so_path, 0x0, sizeof(inject_so_path)-1);
	memcpy(inject_so_path, lib_path, lib_path_length);

	pr_dbg("Path of Shared object to be injected : %s\n", inject_so_path);

	/*
	 * since ELF align each section size by paging size, many section
	 * have padding at its tail. code be load shared object is
	 * injected to here.
	 */
	addr = get_inject_code_addr(target) - inject_size;

	inject_so_loader_ret = regs.rip;
	pr_dbg("Return address : %llx\n", inject_so_loader_ret);
	/*
	 * now that we have an address to copy code to, set the target's
	 * rip to it. we have to advance by 2 bytes here because rip gets
	 * incremented by the size of the current instruction, and the
	 * instruction at the start of the function to inject always
	 * happens to be 2 bytes long.
	 */
	regs.rip = addr + 2;
	pr_dbg("RIP Register : %llx\n", regs.rip);

	ptrace_setregs(target, &regs);

	/*
	 * set up a buffer to hold the code we're going to inject
	 * into the target process.
	 */
	injected_area = xmalloc(inject_size);
	memset(injected_area, 0, inject_size);

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
	thread_count = get_tids(pid, &tids);
	suspend_child_threads(target, addr, &tids, thread_count);

	/*
	 * now that the new code is in place, let the target run
	 * our injected code.
	 */
	ptrace_detach(target);
	release_child_threads(target, addr, &tids, thread_count);

	return 0;
}
