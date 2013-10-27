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

	uint32_t eax, ebx, ecx, edx;
	cpuid(1, eax, ebx, ecx, edx);

	if(edx & CPUID_FEAT_EDX_SSE) {
		terminal_write_string("SSE Supported !!!\n");
	}

    // Set up TSS and their stack shits
    sys_init_tss();

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

		// Explanation of the 7: The MOV opcode is 5 bytes, SYSENTER is 2.
		__asm__("mov %esp, %ecx; mov $0xDEADBEEF, %ebx; mov $.+7, %edx; sysenter;");
	}
}

/*
 * Set up BSS and some other stuff needed to make the kernel not implode.
 * Called BEFORE kernel_main, so kernel structures may NOT be accessed!
 */
void kernel_init() {
	extern uint32_t __bss, __end;

	uint32_t *ptr = (uint32_t *) __bss;

	for(int i = 0; i < ((&__end - &__bss) >> 2); i++) {
		ptr[i] = 0;
	}
}