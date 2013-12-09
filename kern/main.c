#include "types.h"
#include "sys/kinfo.h"
#include "sys/system.h"
#include "sys/kheap.h"
#include "sys/paging.h"
#include "io/ps2.h"
#include "vga/svga.h"
#include "vga/gui.h"
#include "device/rs232.h"
#include "io/disk.h"
#include "device/ata_pio.h"
#include "fs/mbr.h"
#include "fs/vfs.h"
#include "fs/fat.h"
#include "runtime/error_handler.h"
#include "sys/binfmt_elf.h"
#include "sys/task.h"
#include "sys/multiboot.h"
 
static void kernel_preload(void);

extern multiboot_info_t* sys_multiboot_info;
extern uint32_t __kern_size, __kern_bss_start, __kern_bss_size;
extern uint32_t KERN_BNUM;
extern page_directory_t *kernel_directory;
extern heap_t *kheap;

/*
 * Kernel's main entrypoint.
 */
void kernel_main(uint32_t magic, multiboot_info_t* multibootInfo) {
	// Make sure we're booted by a multiboot loader
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		PANIC("Not booted with multiboot-compliant bootloader!");
	}

	kernel_preload();

	paging_stats_t paging_info = paging_get_stats();

	kprintf("%iKB low memory, %iKB high memory\n", sys_multiboot_info->mem_lower, sys_multiboot_info->mem_upper);
	kprintf("%i/%i pages mapped (%i pages free, %i pages wired)\n", paging_info.pages_mapped, paging_info.total_pages, paging_info.pages_free, paging_info.pages_wired);
//	kprintf("%i/%i KB allocated (%i KB free, %i KB wired)\n", paging_info.pages_mapped*4, paging_info.total_pages*4, paging_info.pages_free*4, paging_info.pages_wired*4);

	// kernel stuff
	system_init();

	kprintf("\x01\x11\x01\x0EMosquitOS\x01\x0F\x01\x10 Kernel v0.1 build %u compiled %s on %s with %s\n", (unsigned long) &KERN_BNUM, KERN_BDATE, KERN_BTIME, KERN_COMPILER);

	// Initialise modules
	modules_load();

	// Disk test
	disk_t *hda0 = disk_allocate();
	ata_driver_init(hda0);
	DISK_ERROR ret = disk_init(hda0);

	if(ret != kDiskErrorNone) {
		kprintf("hd0 initialisation error: 0x%X\n", ret);
	} else {
		ptable_t* mbr = mbr_load(hda0);

		// Mount the root filesystem
		ptable_entry_t* partInfo = mbr->first;
		fs_superblock_t *superblock = (fs_superblock_t *) vfs_mount_filesystem(partInfo, "/");

		// Try to make a new task from the ELF we loaded
		void* elfFile = fat_read_file(superblock, "/TEST.ELF", NULL, 0);
		elf_file_t *elf = elf_load_binary(elfFile);
		kprintf("\nParsed ELF to 0x%X\n", elf);

		i386_task_t* task = task_allocate(elf);
	}

/*	
	svga_mode_info_t *svga_mode_info = svga_mode_get_info(0x101);
	gui_set_screen_mode(svga_mode_info);

	window_t *window = malloc(sizeof(window_t));

	window->position.pt.x = 32;
	window->position.pt.y = 32;
	window->position.size.width = 320;
	window->position.size.height = 240;
	window->isDialogue = false;
	window->isActive = true;

	framebuffer_t* fb = malloc(sizeof(framebuffer_t));
	fb->ptr = (uint16_t *) 0xD0000000;
	fb->size.width = 1024;
	fb->size.height = 768;

	gui_draw_window_frame(fb, window); */

/*	uint32_t *doomen = 0x1800DEAD;
	uint32_t test = *doomen;
	kprintf("Pagefaulting read: 0x%X\n", test);*/

	// Explanation of the 7: The MOV opcode is 5 bytes, SYSENTER is 2.
	// __asm__("mov %esp, %ecx; mov $0x0, %ebx; mov $.+7, %edx; sysenter;");

	while(1);
}

/*
 * Initialises some kernel stuff so we don't fail as catastrophically.
 */
void kernel_init(void) {
	// We cannot rely on the bootloader's GDT because it may raise a GPF when
	// an IRQ routine returns (if CS != 0x08)
	sys_build_gdt();
	sys_build_idt();

	// Initialize console so we can display stuff
	console_init();

	// Copy multiboot structure
	sys_copy_multiboot();
}

/*
 * Performs more system initialisation dependant on paging and more advanced
 * kernel facilities.
 */
static void kernel_preload(void) {
	// Check if we're in a VBE mode that's 32bpp or 16bpp and set up FB console
	svga_mode_info_t *vbe_info = (svga_mode_info_t *) sys_multiboot_info->vbe_mode_info;
	if(vbe_info->bpp == 32 || vbe_info->bpp == 16) {
		console_init_fb();
	}
}