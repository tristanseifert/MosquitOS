/*
 * Function epilogues for _init and _fini
 */

.section .init
.global _init
.type   _init,%function
	popl	%ebx
	popl	%ebp
	ret



.section .fini
.global _fini
.type   _fini,%function
	popl	%ebx
	popl	%ebp
	ret
