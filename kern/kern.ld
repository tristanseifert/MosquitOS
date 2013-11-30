/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_osentry)
OUTPUT_FORMAT(elf32-i386)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS {
	. = 0xC0100000;

	__kern_code_start = .;
	.text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000) {
		*(.multiboot);
		*(.entry);
		*(.text);
		*(.rodata);
		*(.rodata*);
	}
}

/*
 * Data
 */
SECTIONS {
	/* Data segment */
	__kern_data_start = .;

	.data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000) {
		*(*.data);
	}

	__kern_data_size = SIZEOF(.data);

	/* Read-write data (uninitialized) and stack */
	__kern_bss_start = .;

	.bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000) {
		*(.bss);
		*(COMMON);
	}

	__kern_bss_size = SIZEOF(.bss);
	__kern_size = __kern_bss_start - __kern_code_start;

	__kern_end = .;

	/* Get rid of stuff we don't care for */
	/DISCARD/ : {
		/* Don't need .eh_frame, since the kernel does NOT use C++ exceptions */
		*(.eh_frame);
		*(.comment);
	}
}