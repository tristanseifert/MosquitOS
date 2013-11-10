/*
 * Implementation-specific and speed-requiring routines for the scheduler.
 */
.globl sched_trap
.align 16
sched_trap:
	pusha														# Pushes edi, esi, ebp, esp, ebx, edx, ecx, eax

	# Push the segment registers not backed up automagically
	pushl	%ds
	pushl	%es
	pushl	%fs
	pushl	%gs

	# Set up some stuff and call scheduler
	pushl	$0x00												# Event status
	pushl	$0x80000000											# Event being waited for (yield)

	call	sched_yield											# Run scheduler
