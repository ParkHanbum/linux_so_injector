#include "asm.h"


/* indicate start of contents to inject */
GLOBAL(inject_contents_start)
END(inject_contents_start)


/*
 * The following function inject_so_loader will be injected at the end
 * of the text section of the target program. when we start executing
 * this function, we will pass the 4 arguments manually by use
 * registers rsi, rdi, rcx, rdx. each arguments are have value
 * like below.
 *
 * rdi = address of malloc() inside of target program.
 * rsi = address of free() inside of target program.
 * rdx = address of dlopen() inside of target program.
 * rcx = length of path that located at shared object which to be loaded.
 */
GLOBAL(inject_so_loader)
	/*
	 * The PC of the target program is changed by uftrace to reach here 
	 * to make load the shared object module the libmcount. uftrace use 
	 * ptrace to do this. but sometime target program have not jump to PC 
	 * which assigned by uftrace. there is some different between assigned 
	 * address and real jump address. maybe it cause by ptrace inner work. 
	 *
	 * [NOTICE]
	 * nop instructions has been added to mitigate this problem.
	 */
	nop
	nop
	nop

	/*
	 * since recently dlopen use movabs instruction,
	 * stack must aligned by 16byte.
	 */
	andq $-16, %rsp

	/*
	 * rsi is going to contain the address of free().
	 * it's going to get wiped out by the call to malloc(),
	 * so save it on the stack for later
	 */
	push %rsi
	/*
	 * same thing for rdx, which will contain the address
	 * of dlopen()
	 */
	push %rdx
	/*
	 * save previous value of r9,
	 * because we're going to use it to call malloc()
	 */
	push %r9
	/* now move the address of malloc() into r9 */
	mov %rdi,%r9
	/*
	 * choose the amount of memory to allocate with malloc()
	 * based on the size of the path to the shared library
	 * passed via rcx
	 */
	mov %rcx,%rdi
	/*
	 * indirect call by register. it will call the malloc()
	 */
	callq *%r9
	/*
	* before continue the execution, we restore r9 register first.
	* it is necessary to keep previous values.
	* because it could be used from target program.
	*/
	pop %r9

	/*
	* when need to confirm about calling inside of target program
	* function like malloc, free, dlopen then do 'trap' by using
	* instruction that offered from cpu. uftrace will read status
	* of target program to verify that target program have been trapped.
	*
	* for now, do 'trap' to pass the address which allocated
	* inside of target program to uftrace. uftrace will write
	* absoulte path of shared object to this address that will
	* be used by dlopen at afterwards.
	*
	* [NOTICE]
	* target program will stopped after trapped until uftrace have
	* make target program execution continue after uftrace done its job.
	*/
	int $3

	/*
	 * get the address of dlopen() off of
	 * the stack so we can call it
	 */
	pop %rdx
	/*
	* before continue the execution, we restore r9 register first.
	* it is necessary to keep previous values.
	* because it could be used from target program.
	*/
	push %r9
	/*
	 * copy the address of dlopen() into r9
	*/
	mov %rdx,%r9
	/*
	 * 1st argument to dlopen():
	 * filename = the address of the buffer returned by malloc()
	 */
	mov %rax,%rdi
	/*
	 * 2nd argument to dlopen(): flag = RTLD_LAZY
	*/
	movabs $1,%rsi
	/*
	* call dlopen()
	*/
	callq *%r9
	/*
	 * restore old r9 value
	 */
	pop %r9
	/*
	 * for now, do 'trap' to confirm that calling dlopen was success.
	 *
	 * [NOTICE]
	 * target program will stopped after trapped until uftrace have
	 * make target program execution continue after uftrace done its job.
	 */
	int $3

	/*
	 * at this point, rax should still contain our malloc()d
	 * buffer from earlier. we're going to free it, so move
	 * rax into rdi to make it the first argument to free().
	 */
	movq %rax,%rdi
	/*
	 * pop rsi so that we can get the address to free(),
	 * which we pushed onto the stack a while ago.
	 */
	popq %rsi
	/*
	 * save previous rbx value
	 */
	push %rbx
	/*
	 * load the address of free() into rbx
	 */
	movq %rsi,%rbx
	/*
	 * zero out rsi, because free() might think that it contains
	 * something that should be freed
	 */
	xor %rsi,%rsi

	/*
	 * break in so that we can check out the arguments right
	 * before making the call
	 *
	 * [NOTICE]
	 * target program will stopped after trapped until uftrace have
	 * make target program execution continue after uftrace done its job.
	 */
	int $3

	/*
	 * call free()
	 */
	callq *%rbx
	/*
	 * restore previous rbx value
	 */
	pop %rbx
END(inject_so_loader)

GLOBAL(inject_so_loader_end)
END(inject_so_loader_end)

GLOBAL(inject_loop_enter)
	nop
	nop
	nop
	push %rax
	push $0
loop:
	movq 0(%rsp), %rax
	test %rax, %rax
	je loop
	pop %rax
	pop %rax
	/*
	 * caller must put the return address that
	 * intended when injecting to top of stack.
	 * if not, it will crash in abnormal address.
	 */
	ret
END(inject_loop_enter)

GLOBAL(inject_loop_enter_end)
END(inject_loop_enter_end)

/* indicate end of contents to inject */
GLOBAL(inject_contents_end)
END(inject_contents_end)
