#include <types.h>
#include "panic.h"

extern void panic_halt_loop(void);

void panic_assert(char *file, uint32_t line, char *desc) {
	// An assertion failed, and we have to panic.
	__asm__ volatile("cli"); // Disable interrupts.

	kprintf("ASSERTION FAILURE(%s) at %s:%i\n", desc, file, line);
	
	// Halt by going into an infinite loop.
	panic_halt_loop();
}

void panic(char *message, char *file, uint32_t line) {
	// We encountered a massive problem and have to stop.
	__asm__ volatile("cli"); // Disable interrupts.

	kprintf("PANIC(%s) at %s:%i\n", message, file, line);

	// Halt by going into an infinite loop.
	panic_halt_loop();
}
