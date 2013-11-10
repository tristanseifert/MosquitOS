#include <types.h>
#include "svga.h"

#include "sys/paging.h"

extern page_directory_t *kernel_directory;

/*
 * Switches the SVGA mode to the specified mode number.
 */
void svga_change_mode(uint16_t mode) {
	kprintf("Changing SVGA mode not supported (mode = 0x%X)\n", mode);
}

/*
 * Returns a pointer to the info struct about a certain SVGA mode.
 */
svga_mode_info_t* svga_mode_get_info(uint16_t mode) {
	return (svga_mode_info_t *) 0x001C00;
}

/*
 * Requests the physical frame buffer address be mapped at the logical frame
 * buffer address, 0xD0000000.
 *
 * This function will always map 16MB of space.
 *
 * On success, it returns the virtual address where the framebuffer was mapped,
 * or 0 on failure.
 */
uint32_t svga_map_fb(uint32_t real_addr) {
/*	int i = 0;
	uint32_t fb_addr;

	// Map framebuffer
	for(i = 0xD0000000; i < 0xD00FF000; i += 0x1000) {
		page_t* page = paging_get_page(i, true, kernel_directory);

		fb_addr = (i & 0x0FFFF000) + real_addr;

		page->present = 1;
		page->rw = 1;
		page->user = 0;
		page->frame = fb_addr >> 12;
		
		//kprintf("Allocated 0x%X -> 0x%X\n", i, fb_addr);
	}

	// Convert the kernel directory addresses to physical if needed
	for(i = 0; i < 1024; i++) {
		uint32_t physAddr = kernel_directory->tablesPhysical[i];

		if((physAddr & 0xC0000000) == 0xC0000000) {
			physAddr &= 0x0FFFFFFF; // get rid of high nybble
			physAddr += 0x00100000; // Add 1M offset

			kernel_directory->tablesPhysical[i] = physAddr;
		}
	}

	kprintf("0xD0000000 table at 0x%X\n", kernel_directory->tablesPhysical[0x340]);

	page_t *page = (page_t *) (kernel_directory->tablesPhysical[0x340] & 0xFFFFFFF0);
	//kprintf("0xD0000000 mapped to 0x%X\n", page->frame << 12);

	// Update MMU
	paging_switch_directory(kernel_directory);*/

	return 0xD0000000;
}