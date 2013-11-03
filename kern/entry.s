#########################################################################################
# This function is responsible for taking control from the bootloader, re-locating the
# kernel to a more sane address, and then starting the kernel.
#########################################################################################
.section .text

.globl _entry
.type _entry, @function

_entry:
	# Set up kernel stack at 0x400000 to 0x380000
	movl	$0x400000, %esp

	# Initialise kernel first (clear memory, etc)
	call	kernel_init

	# Check for SSE, and if it exists, enable it
	mov		$0x1, %eax
	cpuid
	test	$(1 << 25), %edx
	jz 		.noSSE
	
	call	sse_init

.noSSE:
	# Now jump into the kernel's main function
	call	kernel_main

	# In case the kernel returns, hang forever
	cli

.Lhang:
	hlt
	jmp .Lhang

/*
 * Enables the SSE features of the processor.
 */
sse_init:
	mov		%cr0, %eax

	# clear coprocessor emulation CR0.EM
	and		$0xFFFB, %ax

	# set coprocessor monitoring CR0.MP
	or		$0x2, %ax
	mov		%eax, %cr0
	mov		%cr4, %eax

	# set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
	or		$(3 << 9), %ax
	mov		%eax, %cr4
	ret

/*
 * Panic halt loop
 */
.globl panic_halt_loop
panic_halt_loop:
	hlt
	jmp		panic_halt_loop
