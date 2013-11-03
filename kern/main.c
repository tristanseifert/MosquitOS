#include "types.h"
#include "sys/system.h"
#include "sys/kheap.h"
#include "sys/paging.h"
#include "io/terminal.h"
#include "io/ps2.h"
#include "device/rs232.h"
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
		terminal_write_string("CPU supports SSE\n");
	}

	if(ps2_init() != 0) {
		terminal_write_string("ERROR: Could not initialise PS2 driver\n\n");
	}

	sys_kern_info_t* info = sys_get_kern_info();

/*
typedef struct sys_smap_entry {
	00 uint32_t BaseL; // base address QWORD
	04 uint32_t BaseH;
	08 uint32_t LengthL; // length QWORD
	12 uint32_t LengthH;
	16 uint32_t type; // entry Ttpe
	20 uint32_t ACPI; // exteded
} __attribute__((packed)) sys_smap_entry_t;
*/
	terminal_write_string("\n\nBIOS Memory Map:\n");

	sys_smap_entry_t *entry = info->memMap;
	uint32_t total_usable_RAM = 0;

	terminal_write_string("Base Address       | Length             | Type\n");

	for(int i = 0; i < info->numMemMapEnt; i++) {
		terminal_write_string("0x");
		terminal_write_dword(entry->baseH);
		terminal_write_dword(entry->baseL);
		terminal_write_string(" | 0x");

		terminal_write_dword(entry->lengthH);
		terminal_write_dword(entry->lengthL);
		terminal_write_string(" | ");

		switch(entry->type) {
			case 1:
				terminal_write_string("Free Memory (1)");
				total_usable_RAM += entry->lengthL;
				break;

			case 2:
				terminal_write_string("Reserved (2)");
				break;

			case 3:
				terminal_write_string("ACPI Reclaimable (3)");
				break;

			case 4:
				terminal_write_string("ACPI NVS (4)");
				break;

			case 5:
				terminal_write_string("Bad Memory (4)");
				break;

			default:
				terminal_write_string("Unknown (");
				terminal_write_word(entry->type);
				terminal_putchar(')');
				break;
		}

		terminal_putchar('\n');

		entry++;
	}

	terminal_write_string("\n\nTotal usable memory (bytes): 0x");
	terminal_write_dword(total_usable_RAM);

	uint32_t timer = 0;
	uint16_t ps2, temp;
	while(1) {
		// Explanation of the 7: The MOV opcode is 5 bytes, SYSENTER is 2.
		// __asm__("mov %esp, %ecx; mov $0xDEADBEEF, %ebx; mov $.+7, %edx; sysenter;");
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