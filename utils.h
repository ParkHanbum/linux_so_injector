#ifndef TEST_H_INCLUDED
#define TEST_H_INCLUDED

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/wait.h>


#ifndef PR_FMT
# define PR_FMT  "injector"
#endif

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

extern void __pr_dbg(const char *fmt, ...);
extern void __pr_err(const char *fmt, ...);

#define pr_dbg(fmt, ...) 					\
({								\
	__pr_dbg(PR_FMT ": " fmt, ## __VA_ARGS__);		\
})

#define pr_err(fmt, ...)					\
	__pr_err(PR_FMT ": %s:%d:%s\n ERROR: " fmt,		\
	__FILE__, __LINE__, __func__, ## __VA_ARGS__)

#endif
