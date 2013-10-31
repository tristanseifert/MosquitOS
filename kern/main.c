#include "types.h"
#include "sys/system.h"
#include "sys/kheap.h"
#include "sys/paging.h"
#include "io/terminal.h"
#include "io/ps2.h"
#include "runtime/error_handler.h"
 
extern uint32_t kern_bss_start, kern_end, kern_size, kern_data_start, kern_code_start;

#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif
void kernel_main() {
	terminal_initialize(true);

	system_init();

	// From here on, the kernel works with VIRTUAL addresses, not physical.
	paging_init();

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

	terminal_write_string("\nCode Loc: 0x");
	terminal_write_dword(kern_code_start);
	terminal_write_string(" Data Loc: 0x");
	terminal_write_dword(kern_data_start);
	terminal_write_string(" BSS Loc: 0x");
	terminal_write_dword(kern_bss_start);
	terminal_write_string("\nEnd Loc: 0x");
	terminal_write_dword(kern_end);
	terminal_write_string(" Size: 0x");
	terminal_write_dword(kern_size);

	//		while(1);

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
/*	for(int i = 0; i < ((__end - __bss) / 4); i++) {
		__bss[i] = 0;
	}*/

	extern heap_t *kheap;
	kheap = 0;
}