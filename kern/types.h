#ifndef TYPES_H
#define TYPES_H


#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdint.h>

// This gives us all the runtime functions
#include <runtime/std.h>

#define PANIC(x) terminal_initialize(true);\
				terminal_setPos(4, 0);terminal_write_string("Kernel Panic");\
				terminal_setPos(4, 2);terminal_write_string(x);\
				terminal_setPos(4, 4);terminal_write_string(__FILE__);\
				terminal_setPos(4, 5);terminal_write_dword(__LINE__);\
				__asm__("cli");\
				while(1);

#endif