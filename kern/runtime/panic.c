#include <types.h>
#include "panic.h"
#include "io/terminal.h"

void panic_assert(char *file, uint32_t line, char *desc) {
	// An assertion failed, and we have to panic.
	__asm__ volatile("cli"); // Disable interrupts.

	terminal_initialize(true);

	terminal_write_string("ASSERTION FAILED(");
	terminal_write_string(desc);
	terminal_write_string(") at ");
	terminal_write_string(file);
	terminal_write_string(":");
	terminal_write_int(line);
	terminal_write_string("\n");
	
	// Halt by going into an infinite loop.
	while(1);
}

void panic(char *message, char *file, uint32_t line) {
	// We encountered a massive problem and have to stop.
	__asm__ volatile("cli"); // Disable interrupts.

	terminal_initialize(true);

	terminal_write_string("PANIC(");
	terminal_write_string(message);
	terminal_write_string(") at ");
	terminal_write_string(file);
	terminal_write_string(":");
	terminal_write_int(line);
	terminal_write_string("\n");

	// Halt by going into an infinite loop.
	while(1);
}
