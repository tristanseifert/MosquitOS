/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_entry)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS {
	/*
	 * The bootloader contains code that relocates the entire kernel to address
	 * 0xC0000000 in memory, since we now are running in protected mode and
	 * it makes sense to keep the low 1M of memory available for use with
	 * stuff like V86 mode, and allow us to expand beyond about 600K.
	 */
	. = 0xC0000000;

	__kern_code_start = .;

	.text : {
		*(.text);
	}

	/* Data segment */
	__kern_data_start = .;

	.rodata : {
		*(.rodata);
		*(.data);
	}

	/* Read-write data (uninitialized) and stack */
	__kern_bss_start = .;

	.bss : {
		*(.bss COMMON);
	}

	__kern_end = .;

	__kern_bss_size = __kern_end - __kern_bss_start;
	__kern_size = __kern_bss_start - __kern_code_start;
}