.section .text

.extern sys_restore_kern_pagetable
.extern syscall_return_value

/*
 * Syscall handler
 */
.globl	syscall_handler_stub

syscall_handler_stub:
	pushal
	
	call	syscall_handler
	
	popal

	# Restore the syscall's return value
	mov		%eax, syscall_return_value

	sysexit
