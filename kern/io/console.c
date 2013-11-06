#include <types.h>
#include <stdarg.h>

#include "console.h"
#include "terminal.h"
#include "device/rs232.h"

#define printf tfp_printf 
#define sprintf tfp_sprintf 

typedef void (*putcf) (void*, char);
void kprintf_format(void* putp, putcf putf, char *fmt, va_list va);

/*
 * Outputs a character to the display.
 */
void console_putc(void* p, char c) {
	rs232_putchar(KERN_DEBUG_SERIAL_PORT, c);
	terminal_putchar(c);
}

/*
 * Initialises kernel console
 */
void console_init() {
	terminal_initialize(true);
	rs232_init();
}

/*
 * Implements printf for the kernel.
 */
void kprintf(char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	kprintf_format(NULL, console_putc, fmt, va);
	va_end(va);
}
