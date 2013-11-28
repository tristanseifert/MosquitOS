ENTRY(_start)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS {
	. = 0x08000000;

	.text : ALIGN(4K) {
		*(.text);
	}

	/* Data segment */
	.rodata : ALIGN(4K) {
		*(.rodata);
		*(.data);
	}

	/* Read-write data (uninitialized) and stack */
	__bss_start = .;

	.bss : ALIGN(4K) {
		*(.bss COMMON);
	}
	__bss_end = .;
}