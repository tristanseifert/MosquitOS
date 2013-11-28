.section .text

/*
 * The stack should be set up as follows when calling _start:
 *
 * 0(%esp) 				argc
 * 4(%esp) 				argv[0]
 * ...
 * (4*argc)(%esp)		NULL
 * (4*(argc+1))(%esp)	envp[0]
 */
.globl _start
.type _start, @function
_start:
	// Clear frame pointer to mark outermost frame
	xor		%ebp, %ebp

	// Align stack to 16 bytes
	andl	$0xFFFFFFF0, %esp

	// Call the C main function
	call	main

	// In case exit does not kill this process
	hlt
