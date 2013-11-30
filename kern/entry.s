#########################################################################################
# External variable declarations
#########################################################################################
.extern sys_multiboot_info

#########################################################################################
# Multiboot header
#########################################################################################
.set ALIGN,    1 << 0					# align loaded modules on page boundaries
.set MEMINFO,  1 << 1					# provide memory map
.set FLAGS,    ALIGN | MEMINFO			# this is the multiboot 'flag' field
.set MAGIC,    0x1BADB002				# 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS)			# checksum required

#########################################################################################
# This function is responsible for taking control from the bootloader, re-locating the
# kernel to a more sane address, and then starting the kernel.
#########################################################################################
.section .multiboot
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .entry

.globl _osentry
.type _osentry, @function
.set osentry, (_osentry - 0xC0000000)
.globl osentry

_osentry:
	# Enter protected mode
	mov		%cr0, %ecx
	or		$0x00000001, %ecx
	mov		%ecx, %cr0

	# Load page directory
	mov		$(boot_page_directory - 0xC0000000), %ecx
	mov		%ecx, %cr3

	# Enable paging
	mov		%cr0, %ecx
	or		$0x80000000, %ecx
	mov		%ecx, %cr0

	# jump to the higher half kernel
	lea		jmp_highhalf, %ecx
	jmp		*%ecx
 

# We have rudimentary paging down here.
jmp_highhalf:
	# Now, we're executing at 0xC0100000, so use the virtual address for stack.
	mov		$stack_top, %esp

	# Push multiboot info (EBX) and magic number (EAX)
	push 	%ebx
	push	%eax

	# Write multiboot info pointer to memory
	mov		%ebx, sys_multiboot_info

	# Check for SSE, and if it exists, enable it
	mov		$0x1, %eax
	cpuid
	test	$(1 << 25), %edx
	jz 		.noSSE
	
	call	sse_init

.noSSE:
	# Error handling and the like
	call	kernel_init

	# Initialise paging
	call	paging_init

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

.section .text
/*
 * Panic halt loop
 */
.globl panic_halt_loop
panic_halt_loop:
	hlt
	jmp		panic_halt_loop

# Reserve a stack of 16K
.section .bss
stack_bottom:
.skip 0x4000
stack_top:

# Bootup page tables
.section .entry.data

# Map 4MB of space.
.align 0x1000
boot_page_table:
	.set addr, 0

	.rept 1024
	.long addr + 0x03
	.set addr, addr+0x1000
	.endr

# Mirror the first 4 MB throughout the address space.
.align 0x1000
boot_page_directory:
	.rept 1024
	.long (boot_page_table - 0xC0000000) + 0x07
	.endr
