/*
 * Expects an i386_task_state_t struct to be on the stack, and restores
 * the CPU state to that specified inside this structure.
 */
 /*
	+0x00 uint32_t gs, fs, es, ds; *

	+0x10 uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; *
	+0x2C uint32_t eip, cs, eflags, useresp, ss; *

	+0x3C uint32_t page_table; *

	+0x40 uint8_t *fpu_state; 
  */
.globl task_restore_context
task_restore_context:
	# Get pointer to the structure in edi
	push	%ebp
	mov		%esp, %ebp
	mov		0x8(%ebp), %edi

	# Push location to IRET image onto stack
	mov		%edi, %eax
	add		$0x2C, %edi
	pushl	%eax

	# Get pointer to FPU state
	mov		0x40(%edi), %eax

	# Back up previous stack pointer
	mov		%esp, %eax

	# Set up stack pointer so we can pop DS-GS
	mov		%edi, %esp
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds

	# Restore stack pointer
	mov		%eax, %esp

	# Change the struct so that the esp we pop using popal is the current
	mov		%eax, 0x1C(%edi)

	# Get page table pointer and set it
	mov		0x3C(%edi), %eax
	mov		%eax, %cr3

	# Restore registers by using POPAL
	popal

	# The value we just popped off the stack is the IRET image that we want to
	# pop to restore the process' EIP and privilege level.
	popl	%esp

	# Restore EFLAGS, ESP, EIP, all that fun stuff from user mode
	iret

/*
 * Below here is a place we can declare some variables at, which will be put in
 * the BSS segment and initialised to zero.
 */
.bss
