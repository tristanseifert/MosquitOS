.section .text

/*
 * Syscall handler
 */
.globl	syscall_handler_stub
.align	16
syscall_handler_stub:
	pushal
	call	syscall_handler
	popal
	sysexit
