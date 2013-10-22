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

	# Initialise kernel first
	call	kernel_init

	# Now jump into the kernel's main function
	call	kernel_main

	# In case the kernel returns, hang forever
	cli

.Lhang:
	hlt
	jmp .Lhang
