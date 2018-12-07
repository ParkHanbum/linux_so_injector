#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

#define PR_FMT		"dynamic"
#define PR_DOMAIN	DBG_DYNAMIC
#include "utils/inject.h"
#include "utils/utils.h"


// define max line that read from /proc/<pid>/map
#define PATH_MAX 4096

long get_inject_code_addr(pid_t pid)
{
	FILE *fp;
	char filename[30];
	char line[PATH_MAX];
	long start, end, result = 0;
	char str[20];
	char perms[5];
	char *fmt = "/proc/%d/maps";

	snprintf(filename, sizeof(filename), fmt, pid);
	fp = fopen(filename, "r");
	if (fp == NULL)
		pr_err("cannot open /proc/%d/maps", pid);
	while (fgets(line, sizeof(line), fp) != NULL) {
		sscanf(line, "%lx-%lx %s %*s %s %*d", &start, &end, perms, str);
		if (strstr(perms, "x") != NULL) {
			result = end;
			break;
		}
	}
	fclose(fp);
	return result;
}

long get_so_addr(pid_t pid, char *so_name)
{
	FILE *fp;
	char filename[30];
	char line[PATH_MAX];
	long addr, result=0;

	snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
	fp = fopen(filename, "r");
	if (fp == NULL)
		pr_err("cannot open /proc/%d/maps", pid);
	while (fgets(line, PATH_MAX, fp) != NULL) {
		sscanf(line, "%lx-%*x %*s %*s %*s %*d", &addr);
		if (strstr(line, so_name) != NULL) {
			result = addr;
			break;
		}
	}
	fclose(fp);
	return result;
}

long get_libdl_addr(pid_t pid)
{
	return get_so_addr(pid, "libdl-");
}

int check_loaded(pid_t pid, char *libname)
{
	return get_so_addr(pid, libname) != 0;
}

long get_function_addr(char *so_name, char *func_name)
{
	void *so_addr = dlopen(so_name, RTLD_LAZY);
	void *func_addr = dlsym(so_addr, func_name);

	return (long)func_addr;
}
