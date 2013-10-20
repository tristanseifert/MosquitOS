#include "types.h"
#include "sys/system.h"
#include "sys/paging.h"
#include "io/terminal.h"
#include "runtime/error_handler.h"
 
#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif
void kernel_main() {
	system_init();

	paging_init();

	terminal_initialize(true);
	terminal_write_string("Hello, kernel world!\n");

	uint8_t *mosquiten = (uint8_t *) 0x02000;
	terminal_setPos(4, 2);
	terminal_write_dword(*(mosquiten));

	uint32_t *ptr = (uint32_t *) 0xDEADBEEF;
	uint32_t do_page_fault = *ptr;

	uint32_t timer = 0;
	while(1) {
		timer = sys_get_ticks() & 0xFFFFFFFF;
		terminal_setPos(4, 4);
		terminal_write_dword(timer);
	}
}

/*
 * Set up BSS and some other stuff needed to make the kernel not implode.
 * Called BEFORE kernel_main, so kernel structures may NOT be accessed!
 */
void kernel_init() {
	extern char __bss, __end;

	memset(&__bss, 0x00, (&__end - &__bss));
}