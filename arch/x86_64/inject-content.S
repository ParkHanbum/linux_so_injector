#include "utils/asm.h"

	.file	"inject.c"
	.section .data
	.globl	inject_contents_start
	.type	inject_contents_start, @function
inject_contents_start:
	.size	inject_contents_start, .-inject_contents_start

// inject_so_loader
	.text
	.globl	inject_so_loader
	.section .data
	.type	inject_so_loader, @function
inject_so_loader:
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
	 * save registers what used in below codes
	 */
	pushq %rax
	pushq %rsi
	pushq %rdi
	pushq %r9
	/* create new stack frame to save registers */
	pushq %rsp
	pushq %rbp
	movq %rsp, %rbp

	/*
	 * since recently dlopen use movabs instruction,
	 * stack must be aligned by 16byte.
	 */
	andq $-16, %rsp

	/*
	 * 1st argument to dlopen():
	 * absolute path of shared object to loaded.
	 */
	leaq inject_so_path(%rip), %rdi
	/*
	 * 2nd argument
	 * 0x80000000 : __RTLD_DLOPEN
	 * 0x00000001 : RTLD_NOW
	 */
	movabs $0x80000001, %rsi
	movq inject_dlopen_addr(%rip), %r9

	/*
	 * call dlopen()
	 */
	callq *%r9

	/*
	 * rewind stack
	 */
	movq %rbp, %rsp
	popq %rbp
	popq %rsp
	/*
	 * restore used registers.
	 */
	popq %r9
	popq %rdi
	popq %rsi
	popq %rax

	/*
	 * return to original control flow that
	 * before to trapped by ptrace.
	 */
	jmpq *0(%rip)
	.size	inject_so_loader, .-inject_so_loader
// inject_so_ret
	.globl	inject_so_loader_ret
	.section .data
inject_so_loader_ret:
	.zero	8
	.type	inject_so_loader_ret, @object
	.size	inject_so_loader_ret,8
// inject_dlopen_addr
	.globl	inject_dlopen_addr
	.section .data
inject_dlopen_addr:
	.zero	8
	.type	inject_dlopen_addr, @object
	.size	inject_dlopen_addr,8
// inject_so_path
	.globl	inject_so_path
	.section .data
	.type	inject_so_path, @object
	.size	inject_so_path, 128
inject_so_path:
	.string "absolute path of shared object"
	.zero	101

	.globl	inject_contents_end
	.section .data
	.type	inject_contents_end, @function
inject_contents_end:
	.size	inject_contents_end, .-inject_contents_end

