#include "types.h"
#include "sys/kinfo.h"
#include "sys/system.h"
#include "sys/kheap.h"
#include "sys/paging.h"
#include "io/ps2.h"
#include "vga/svga.h"
#include "device/rs232.h"
#include "runtime/error_handler.h"
 
extern uint32_t __kern_size, __kern_bss_start;

#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif
void kernel_main() {
	console_init();

	system_init();

	uint32_t eax, ebx, ecx, edx;
	cpuid(1, eax, ebx, ecx, edx);

	kprintf("\x01\x11MosquitOS\x01\x10 Kernel v0.1 compiled %s on %s with %s\n", KERN_BDATE, KERN_BTIME, KERN_COMPILER);
	kprintf("Kernel size: 0x%X\n", __kern_size);

	if(edx & CPUID_FEAT_EDX_SSE) {
		kprintf("CPU supports SSE\n");
	}

	if(ps2_init() != 0) {
		kprintf("ERROR: Could not initialise PS2 driver\n");
	}

	sys_kern_info_t* info = sys_get_kern_info();

	kprintf("\nBIOS Memory Map:\n");

	sys_smap_entry_t *entry = info->memMap;
	uint32_t total_usable_RAM = 0;

	char *typeStr;

	for(int i = 0; i < info->numMemMapEnt; i++) {
		switch(entry->type) {
			case 1:
				typeStr = "Free Memory (1)";
				total_usable_RAM += entry->lengthL;
				break;

			case 2:
				typeStr = "Reserved (2)";
				break;

			case 3:
				typeStr = "ACPI Reclaimable (3)";
				break;

			case 4:
				typeStr = "ACPI NVS (4)";
				break;

			case 5:
				typeStr = "Bad Memory (5)";
				break;

			default:
				typeStr = (char*) kmalloc(32);
				sprintf(typeStr, "Unknown (%X)", entry->type);
				break;
		}

		kprintf("Base: 0x%X%X Size: 0x%X%X %s\n", entry->baseH, entry->baseL, 
			entry->lengthH, entry->lengthL, typeStr);

		if(entry->type > 5) {
			kfree(typeStr);
		}

		entry++;
	}

	kprintf("\n\nTotal usable memory (bytes): 0x%X\n", total_usable_RAM);
	
	svga_mode_info_t *svga_mode_info = svga_mode_get_info(0x101);
	kprintf("Mode 0x101 framebuffer: 0x%X\n", svga_mode_info->physbase);

/*	uint32_t *doomen = 0x1800DEAD;
	uint32_t test = *doomen;
	kprintf("Pagefaulting read: 0x%X\n", test);*/

	/*kprintf("Attempting to switch contexts...\n");
	__asm__("mov $0xDEADC0DE, %edx; int $0x88");
	kprintf("Context switch OK !!!\n");*/

	// Explanation of the 7: The MOV opcode is 5 bytes, SYSENTER is 2.
	// __asm__("mov %esp, %ecx; mov $0x0, %ebx; mov $.+7, %edx; sysenter;");


	while(1);
}

/*
 * Set up BSS and some other stuff needed to make the kernel not implode.
 * Called BEFORE kernel_main, so kernel structures may NOT be accessed!
 */
void kernel_init() {
	extern heap_t *kheap;
	kheap = 0;
}