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
	// From here on, the kernel works with VIRTUAL addresses, not physical.
	paging_init();

	terminal_initialize(true);

	terminal_write_string("Hello, kernel world!\n\n");

	cpu_info_t* cpuinfo = sys_get_cpu_info();

	if(cpuinfo->manufacturer == kCPUManufacturerIntel) {
		terminal_write_string("Detected Intel CPU\n");
	} else if(cpuinfo->manufacturer == kCPUManufacturerAMD) {
		terminal_write_string("Detected AMD CPU\n");
	} else {
		terminal_write_string("Detected unknown CPU\n");
	}

	terminal_write_string("Family: ");
	terminal_write_dword(cpuinfo->family);
	terminal_write_string(" Model: ");
	terminal_write_dword(cpuinfo->model);
	terminal_write_string(" Stepping: ");
	terminal_write_dword(cpuinfo->stepping);
	terminal_write_string(" Ext. Family: ");
	terminal_write_dword(cpuinfo->extended_family);

	terminal_write_string("\nRead CPU type: ");
	terminal_write_string(cpuinfo->detected_name);

	terminal_write_string("\n\n");
	if(cpuinfo->str_type) {
		terminal_write_string("  CPU Type: ");
		terminal_write_string(cpuinfo->str_type);
		terminal_write_string("\n");
	}

	if(cpuinfo->str_family) {
		terminal_write_string("CPU Family: ");
		terminal_write_string(cpuinfo->str_family);
		terminal_write_string("\n");
	}

	if(cpuinfo->str_model) {
		terminal_write_string(" CPU Model: ");
		terminal_write_string(cpuinfo->str_model);
		terminal_write_string("\n");
	}

	if(cpuinfo->str_brand) {
		terminal_write_string(" CPU Brand: ");
		terminal_write_string(cpuinfo->str_brand);
		terminal_write_string("\n");
	}

/*	uint8_t *mosquiten = (uint8_t *) 0x02000;
	terminal_setPos(4, 2);
	terminal_write_dword(*(mosquiten));*/

	//uint32_t *ptr = (uint32_t *) 0xDEADBEEF;
	//uint32_t do_page_fault = *ptr;

	uint32_t timer = 0;
	while(1) {
		timer = sys_get_ticks() & 0xFFFFFFFF;
		terminal_setPos(4, 18);
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