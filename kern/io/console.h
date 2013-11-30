#ifndef CONSOLE_H
#define CONSOLE_H

#include <types.h>

#define CONSOLE_BOLD "\x01\x11"
#define CONSOLE_REG "\x01\x10"

// Initialises the console.
void console_init();
void console_init_fb();
void kprintf(char *fmt, ...);

#endif