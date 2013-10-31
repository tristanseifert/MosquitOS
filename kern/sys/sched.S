/*
 * Implementation-specific and speed-requiring routines for the scheduler.
 */
.globl sched_trap
.align 16
sched_trap:
	pusha														# Pushes edi, esi, ebp, esp, ebx, edx, ecx, eax

	pushl	$0x00												# Event status
	pushl	$0x80000000											# Event being waited for (yield)
	call	sched_yield											# Run scheduler
	add 	$0x8, %esp											# Cleans up the pushed event information

	popa														# Pops edi, esi, ebp, esp, ebx, edx, ecx, eax
	iret														# Pops CS, EIP, EFLAGS, SS, and ESP
