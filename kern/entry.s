#########################################################################################
# This function is responsible for taking control from the bootloader, enabling protected
# mode, and starting the kernel.
#########################################################################################
.globl _loader

_loader:
	# Set up kernel stack at $400000
	mov		0x400000, %esp

	# Initialise kernel first
	call	_kernel_init

	# Now jump into the kernel's main function
	call	_kernel_main

	# In case the kernel returns, hang forever

	cli
	hlt

.Lhang:
	jmp .Lhang
