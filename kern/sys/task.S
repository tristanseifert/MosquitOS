/*
 * Expects an i386_task_state_t struct to be on the stack, and restores
 * the CPU state to that specified inside this structure.
 */
 /*
	+0x00 uint32_t gs, fs, es, ds; *

	+0x10 uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; *
	+0x30 uint32_t eip, cs, eflags, useresp, ss; *

	+0x44 uint32_t page_table; *

	+0x48 uint8_t *fpu_state; 
  */
.extern task_temp_fxsave_region
.globl task_restore_context
task_restore_context:
	# Get pointer to the structure in edi
	push	%ebp
	mov		%esp, %ebp
	mov		8(%ebp), %edi

	# Push location to IRET image onto stack
	mov		%edi, %eax
	add		$0x30, %eax
	mov		%eax, iret_image_loc

	# Read GS through DS from the image
	mov		(%edi), %gs
	mov		4(%edi), %fs
	mov		8(%edi), %es
	mov		12(%edi), %ds

	# Restore FPU state
	fxrstor	task_temp_fxsave_region

	# Get page table pointer and set it
	mov		0x44(%edi), %eax
#	mov		%eax, %cr3

	# Move stack pointer to top of PUSHAL image
	mov		%edi, %eax
	add		$0x10, %eax
	mov		%eax, %esp

	# Restore registers by using POPAL
	popal

	# The value we just popped off the stack is the IRET image that we want to
	# pop to restore the process' EIP and privilege level.
	mov		iret_image_loc, %esp

	# Restore EFLAGS, ESP, EIP, all that fun stuff from user mode
	iret

/*
 * Below here is a place we can declare some variables at, which will be put in
 * the BSS segment and initialised to zero.
 */
.bss
iret_image_loc:
	.word	0
