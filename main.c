#include <stdio.h>
#include <stdlib.h>

extern int inject(char *libname, pid_t pid);

int main(int argc, char** argv)
{
	char *libname = argv[1];
	pid_t pid = atoi(argv[2]);

	inject(libname, pid);
}
