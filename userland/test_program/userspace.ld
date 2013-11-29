/*
 * C library entry point
 */
ENTRY(_start)

SECTIONS {
	/*
 	 * Start mapping to 0x02000000: This gives us 32M of memory that's "free"
 	 * below the .text section, 16M of which we can allocate as a stack for the
 	 * program (that's why stack starts at 0x02000000!), and the low 16M are
 	 * reserved to catch NULL pointers. Clever, eh?
	 */
	. = 0x02000000;

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