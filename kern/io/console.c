#include <types.h>
#include <stdarg.h>

#include "vga/fb_console.h"
#include "vga/vga_console.h"
#include "console.h"
#include "device/rs232.h"

static bool fb_console_initialized;
char* printf_buffer;

/*
 * Outputs a character to the display.
 */
void console_putc(char c) {
	if(c == '\n') {
		(fb_console_initialized) ? fb_console_control(c) : vga_console_control(c);
		rs232_putchar(KERN_DEBUG_SERIAL_PORT, '\r');
		rs232_putchar(KERN_DEBUG_SERIAL_PORT, '\n');
	} else {
		rs232_putchar(KERN_DEBUG_SERIAL_PORT, c);
		(fb_console_initialized) ? fb_console_putchar(c) : vga_console_putchar(c);
	}
}

/*
 * Initialises kernel console
 */
void console_init() {
	vga_console_init();
}

/*
 * Switches to framebuffer console.
 */
void console_init_fb() {
	fb_console_initialized = true;
	fb_console_init();
}

/*
 * Implements printf for the kernel.
 */
int kprintf(const char* format, ...) {
	// Allocate buffer if needed
	if(unlikely(!printf_buffer)) {
		printf_buffer = (char *) kmalloc(0x1000);
	}

	memclr(printf_buffer, 0x1000);

	char* buffer = (char *) printf_buffer;
	va_list ap;
	va_start(ap, format);
	int n = vsprintf(buffer, format, ap);
	va_end(ap);

	while(*buffer != 0x00) console_putc(*buffer++);

	return n;
}
