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
 
extern uint32_t __kern_size, __kern_bss_start, __kern_bss_size;
extern uint32_t KERN_BNUM;
extern page_directory_t *kernel_directory;
extern heap_t *kheap;

#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif
void kernel_main(uint32_t magic, void *multibootInfo) {
	uint32_t *bss = (uint32_t *) (&__kern_bss_start);
	uint32_t bss_size = (uint32_t) &__kern_bss_size;

	// kernel stuff
	system_init();
	vfs_init();

	kprintf("\x01\x11\x01\x0EMosquitOS\x01\x0F\x01\x10 Kernel v0.1 build %u compiled %s on %s with %s\n", (unsigned long) &KERN_BNUM, KERN_BDATE, KERN_BTIME, KERN_COMPILER);
	kprintf("Kernel size: %i bytes (BSS at 0x%X, %i bytes)\n", &__kern_size, bss, bss_size);

	if(ps2_init() != 0) {
		kprintf("ERROR: Could not initialise PS2 driver\n");
	}

	while(1);

	sys_kern_info_t* info = sys_get_kern_info();

	kprintf("\nBIOS Memory Map:\n");

	sys_smap_entry_t *entry = info->memMap;
	uint32_t total_usable_RAM = 0;

	char *typeStr;

	for(int i = 0; i < info->numMemMapEnt; i++) {
		switch(entry->type) {
			case 1:
				typeStr = "Available";
				total_usable_RAM += entry->lengthL;
				break;

			case 2:
				typeStr = "Reserved";
				break;

			case 3: case 4:
				typeStr = "ACPI";
				break;

			case 5:
				typeStr = "Bad";
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

	kprintf("\nTotal usable memory: %i kB\n", total_usable_RAM/1024);

	// Disk test
	disk_t *hda0 = disk_allocate();
	ata_driver_init(hda0);
	DISK_ERROR ret = disk_init(hda0);

	if(ret != kDiskErrorNone) {
		kprintf("hd0 initialisation error: 0x%X\n", ret);
	} else {
		fat_init();

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

	/*kprintf("Attempting to switch contexts...\n");
	__asm__("mov $0xDEADC0DE, %edx; int $0x88");
	kprintf("Context switch OK !!!\n");*/

	// Explanation of the 7: The MOV opcode is 5 bytes, SYSENTER is 2.
	// __asm__("mov %esp, %ecx; mov $0x0, %ebx; mov $.+7, %edx; sysenter;");

	while(1);
}

/*
 * Initialises some kernel stuff so we don't fail as catastrophically.
 */
void kernel_init(void) {
	sys_build_gdt();
	sys_build_idt();
	console_init();

	kprintf("Balls.\n");
}