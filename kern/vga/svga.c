#include <types.h>
#include "svga.h"

#include "sys/paging.h"
#include "sys/multiboot.h"

extern page_directory_t *kernel_directory;
extern multiboot_info_t* sys_multiboot_info;

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
	return (svga_mode_info_t *) sys_multiboot_info->vbe_mode_info;
}

/*
 * Requests the physical frame buffer address be mapped at the logical frame
 * buffer address, 0xD0000000.
 *
 * This function will map fb_length bytes.
 *
 * On success, it returns the virtual address where the framebuffer was mapped,
 * or 0 on failure.
 */
uint32_t svga_map_fb(uint32_t real_addr, uint32_t fb_length) {
	int i = 0;
	uint32_t fb_addr;

	// Align framebuffer length to page boundaries
	fb_length += 0x1000;
	fb_length &= 0x0FFFF000;

	// Map enough framebuffer
	for(i = 0xD0000000; i < 0xD0000000 + fb_length; i += 0x1000) {
		page_t* page = paging_get_page(i, true, kernel_directory);

		fb_addr = (i & 0x0FFFF000) + real_addr;

		page->present = 1;
		page->rw = 1;
		page->user = 1;
		page->frame = fb_addr >> 12;
	}

	// Convert the kernel directory addresses to physical if needed
	for(i = 0x340; i < 0x340 + (fb_length / 0x400000); i++) {
		uint32_t physAddr = kernel_directory->tablesPhysical[i];

		if((physAddr & 0xC0000000) == 0xC0000000) {
			physAddr &= 0x0FFFFFFF; // get rid of high nybble

			kernel_directory->tablesPhysical[i] = physAddr;
		}
	}

	return 0xD0000000;
}