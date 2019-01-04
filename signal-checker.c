#include <stdio.h>
#include <dlfcn.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

struct sigaction act;

void sighandler(int signum, siginfo_t *info, void *ptr)
{
	fprintf(stderr, "Received signal %d\n", signum);
	fprintf(stderr, "Signal originates from process %lu\n",
			(unsigned long)info->si_pid);
}

void setup_sighandler()
{
	printf("I am %lu\n", (unsigned long)getpid());

	memset(&act, 0, sizeof(act));

	act.sa_sigaction = sighandler;
	act.sa_flags = SA_SIGINFO;

	for(int i = 0; i < 32; i++) {
		sigaction(i, &act, NULL);
	}
}

void hello()
{
	fprintf(stderr, "Hello, I'm loaded!!\n");
}


/*
 * loadMsg()
 *
 * This function is automatically called when the sample library is injected
 * into a process. It calls hello() to output a message indicating that the
 * library has been loaded.
 *
 */
__attribute__((constructor))
void loadMsg()
{
	setup_sighandler();
	hello();
}
