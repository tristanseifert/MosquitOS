/*
 * Function prologues for _init and _fini
 */

.section .init
.global _init
.type   _init, @function
_init:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx

.section .fini
.global _fini
.type   _fini, @function
_fini:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
