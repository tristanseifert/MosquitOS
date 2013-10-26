#include "types.h"
#include "sys/system.h"
#include "sys/paging.h"
#include "io/terminal.h"
#include "io/ps2.h"
#include "runtime/error_handler.h"
 
#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif
void kernel_main() {
	system_init();
	// From here on, the kernel works with VIRTUAL addresses, not physical.
	paging_init();

	terminal_initialize(true);

	if(ps2_init() != 0) {
		terminal_write_string("ERROR: Could not initialise PS2 driver\n\n");
	}

	uint32_t timer = 0;
	uint16_t ps2, temp;
	while(1) {
		timer = sys_get_ticks() & 0xFFFFFFFF;
		terminal_setPos(4, 18);
		terminal_write_dword(timer);

		temp = ps2_read(false);

		if(temp != 0x8000) {
			ps2 = temp;

			terminal_setPos(14, 18);
			terminal_write_word(ps2);
		}
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