#########################################################################################
# This function is responsible for taking control from the bootloader, enabling protected
# mode, and starting the kernel.
#########################################################################################
.section .text
.globl _loader
.type _loader, @function

_loader:
	# Set up kernel stack at 0x340000 to 0x400000
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
