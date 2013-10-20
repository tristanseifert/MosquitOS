/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_loader)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS {
	. = 0x3000;

	/* First put the multiboot header, as it is required to be put very early
	   early in the image or the bootloader won't recognize the file format.
	   Next we'll put the .text section. */
	.text BLOCK(1K) : ALIGN(1K) {
		_code = .; __code = .;
		*(.text)
	}

	/* Data segment */
	.rodata BLOCK(1K) : ALIGN(1K) {
		_data = .; __data = .;
		*(.data)
		*(.rodata)
	}

	/* Read-write data (uninitialized) and stack */
	.bss BLOCK(1K) : ALIGN(1K) {
		_bss = .; __bss = .;
		*(.bss)
	}

    _end = .; __end = .;
}