/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_entry)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS {
	/*
	 * The bootloader contains code that relocates the entire kernel to address
	 * 0x00100000 in memory, since we now are running in protected mode and
	 * it makes sense to keep the low 1M of memory available for use with
	 * stuff like V86 mode, and allow us to expand beyond about 600K.
	 */
	. = 1M;
	.text BLOCK(1K) : ALIGN(1K) {
		__code = .;
		*(.text)
	}

	/* Data segment */
	.rodata BLOCK(1K) : ALIGN(1K) {
		__data = .;
		*(.data)
		*(.rodata)
	}

	/* Read-write data (uninitialized) and stack */
	.bss BLOCK(4K) : ALIGN(1K) {
		__bss = .;
		*(.bss)
	}

    __end = .;

    __size = __bss - __code;
}