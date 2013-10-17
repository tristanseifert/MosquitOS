#include "types.h"
#include "sys/system.h"
#include "io/terminal.h"
#include "runtime/error_handler.h"
 
#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif
void kernel_main() {
	system_init();

	terminal_initialize(true);
	terminal_write_string("Hello, kernel world!\n");

	error_init();
}

/*
 * Set up BSS and some other stuff needed to make the kernel not implode.
 */
void kernel_init() {
	extern char _DATA__bss__begin, _DATA__bss__end;
	extern char _DATA__common__begin, _DATA__common__end;

	memset(&_DATA__bss__begin, 0x00, (&_DATA__bss__end - &_DATA__bss__begin));
	memset(&_DATA__common__begin, 0x00, (&_DATA__common__end - &_DATA__common__begin));
}