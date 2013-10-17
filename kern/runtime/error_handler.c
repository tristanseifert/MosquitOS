#include <types.h>
#include <device/vga.h>
#include <io/terminal.h>

#include "error_handler.h"

void error_dump_regs();

/*
 * Initialises the VGA hardware for displaying an error screen.
 */
void error_init() {
	terminal_setColour(vga_make_colour(VGA_COLOUR_WHITE, VGA_COLOUR_BLUE));
	terminal_clear();

	terminal_setColour(vga_make_colour(VGA_COLOUR_WHITE, VGA_COLOUR_BLUE));
	terminal_setPos(16, 2);
	terminal_write_string("GURU MEDITATION"-0x80);
	terminal_setPos(4, 4);
	terminal_write_string("(Or, \"piss off, you broke something!\")"-0x80);

	error_dump_regs();
}

/*
 * Dumps registers and exception name.
 */
void error_dump_regs() {
	terminal_setPos(4, 6);
	terminal_setColour(vga_make_colour(VGA_COLOUR_RED, VGA_COLOUR_WHITE));
	terminal_write_string("Exception name goes here"-0x80);
	terminal_setColour(vga_make_colour(VGA_COLOUR_WHITE, VGA_COLOUR_BLUE));
}
