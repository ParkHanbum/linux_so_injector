/*
 * Linux module for injecting shared object to the process by using ptrace.
 *
 * copied from: https://github.com/gaffe23/linux-inject
 * Released under the GPL v2+.
 */
long get_inject_code_addr(pid_t pid);
long get_so_addr(pid_t pid, char *so_name);

long get_libc_addr(pid_t pid);
long get_libdl_addr(pid_t pid);

int check_loaded(pid_t pid, char *lib_name);
long get_function_addr(char *so_name, char *func_name);
unsigned char *find_ret(void *end_addr);
