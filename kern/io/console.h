#ifndef CONSOLE_H
#define CONSOLE_H

#include <types.h>

// Defined externally
extern void sprintf(char* s, char *fmt, ...);

// Initialises the console.
void console_init();
void kprintf(char *fmt, ...);

#endif