#include <types.h>

#include "paging.h"
#include "kheap.h"
#include "runtime/error_handler.h"
#include "vga/svga.h"
 
extern uint32_t __kern_size, __kern_bss_start, __kern_bss_size;

void sys_build_gdt();

// The kernel's page directory
page_directory_t *kernel_directory = 0;
uint32_t kern_dir_phys;
// The current page directory;
page_directory_t *current_directory = 0;

// A bitset of frames - used or free.
uint32_t* frames;
uint32_t nframes;

static bool newGDTInstalled;

// Defined in kheap.c
extern uint32_t kheap_placement_address;
extern heap_t* kheap;

// Defined in system.c
void sys_init_tss();

// Macros used in the bitset algorithms.
#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

/*
 * Static function to set a bit in the frames bitset
 */
static void set_frame(uint32_t frame_addr) {
	uint32_t frame = frame_addr / 0x1000;
	uint32_t idx = INDEX_FROM_BIT(frame);
	uint32_t off = OFFSET_FROM_BIT(frame);
	frames[idx] |= (0x1 << off);
}

/*
 * Static function to clear a bit in the frames bitset
 */
static void clear_frame(uint32_t frame_addr) {
	uint32_t frame = frame_addr / 0x1000;
	uint32_t idx = INDEX_FROM_BIT(frame);
	uint32_t off = OFFSET_FROM_BIT(frame);
	frames[idx] &= ~(0x1 << off);
}

/*
 * Static function to test if a bit is set.
 */
static uint32_t test_frame(uint32_t frame_addr) {
	uint32_t frame = frame_addr / 0x1000;
	uint32_t idx = INDEX_FROM_BIT(frame);
	uint32_t off = OFFSET_FROM_BIT(frame);
	return (frames[idx] & (0x1 << off));
}

/*
 * Static function to find the first free frame.
 */
static uint32_t first_frame() {
	uint32_t i, j;
	for (i = 0; i < INDEX_FROM_BIT(nframes); i++) {
		if (frames[i] != 0xFFFFFFFF) { // nothing free, check next
			// At least one bit is free here
			for (j = 0; j < 32; j++) {
				uint32_t toTest = 0x1 << j;

				// If this frame is free, return
				if (!(frames[i] & toTest)) {
					return i*4*8+j;
				}
			}
		}
	}

	return -1;
}

/*
 * Function to allocate a frame.
 */
void alloc_frame(page_t* page, bool is_kernel, bool is_writeable) {
	if (page->frame != 0) {
		return;
	} else {
		uint32_t idx = first_frame();
		if (idx == (uint32_t) -1) {
			PANIC("No Free Frames");
		}

		set_frame(idx * 0x1000);

		page->present = 1;
		page->rw = (is_writeable) ? 1 : 0;
		page->user = (is_kernel) ? 0 : 1;
		page->frame = idx;
	}
}

/*
 * Function to deallocate a frame.
 */
void free_frame(page_t* page) {
	uint32_t frame;
	if (!(frame=page->frame)) {
		return;
	} else {
		clear_frame(frame);
		page->frame = 0x0;
	}
}

/*
 * Initialises paging and sets up page tables.
 */
void paging_init() {
	kheap = NULL;

	// The size of physical memory. For the moment we 
	// assume it is 64MB big.
	uint32_t mem_end_page = 0x04000000;
	
	nframes = mem_end_page / 0x1000;

	frames = (uint32_t *) kmalloc(INDEX_FROM_BIT(nframes));
	memclr(frames, INDEX_FROM_BIT(nframes));

	// Allocate mem for a page directory.
	kernel_directory = (page_directory_t *) kmalloc_a(sizeof(page_directory_t));

	ASSERT(kernel_directory != NULL);

	memclr(kernel_directory, sizeof(page_directory_t));
	current_directory = kernel_directory;

	// Allocate memory for pagetables.
	// XXX: This is really bad and should be fixed because it really is just an ugly
	// hack around the fact that we for some reason cannot allocate pagetables later.
	unsigned int i = 0;
	for(i = 0x00000000; i < 0xFFFFF000; i += 0x1000) {
		page_t* page = paging_get_page(i, true, kernel_directory);
	}

	// Map some pages in the kernel heap area.
	// Here we call get_page but not alloc_frame. This causes page_table_t's 
	// to be created where necessary. We can't allocate frames yet because they
	// they need to be identity mapped first below, and yet we can't increase
	// placement_address between identity mapping and enabling the heap
	for(i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += 0x1000) {
		paging_get_page(i, true, kernel_directory);
	}

	// Map 0xC0000000 to 0xC7FFFFFF to 0x00000000 to 0x07FFF000
	for(i = 0xC0000000; i < 0xC7FFF000; i += 0x1000) {
		page_t* page = paging_get_page(i, true, kernel_directory);

		page->present = 1;
		page->rw = 1;
		page->user = 0;
		page->frame = ((i & 0x0FFFF000) >> 12);
	}

	// We need to identity map (phys addr = virt addr) from
	// 0x0 to the end of used memory, so we can have access to the
	// low memory.
	//
	// NOTE that we use a while loop here deliberately.
	// inside the loop body we actually change placement_address
	// by calling kmalloc(). A while loop causes this to be
	// computed on-the-fly rather than once at the start.
	// Also allocate a little bit extra so the kernel heap can be
	// initialised properly.
	i = 0x00000000;
	while(i < (kheap_placement_address & 0x0FFFFFFF) + 0x1000) {
		// Kernel code is readable but not writeable from userspace.
		alloc_frame(paging_get_page(i, true, kernel_directory), false, false);
		i += 0x1000;
	}

	// Allocate pages mapped earlier.
	for(i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += 0x1000) {
		alloc_frame(paging_get_page(i, false, kernel_directory), false, false);
	}

	// Set page fault handler
	// sys_set_idt_gate(14, (uint32_t) isr14, 0x08, 0x8E);

	// Convert kernel directory address to physical and save it
	kern_dir_phys = (uint32_t) &kernel_directory->tablesPhysical;
	kern_dir_phys &= 0x0FFFFFFF;
	kern_dir_phys += 0x00100000;

	// Load the new GDT
	sys_build_gdt();

	// Enable paging
	paging_switch_directory(kernel_directory);

	// Initialise a kernel heap
	kheap = create_heap(KHEAP_START, KHEAP_START+KHEAP_INITIAL_SIZE, 0xCFFFF000, false, false);
}

/*
 * Switches the currently-used page directory.
 */
void paging_switch_directory(page_directory_t* new) {
	// If the GDT is still the old one, load the new one...
	if(!newGDTInstalled) {
		newGDTInstalled = true;
	}

	uint32_t tables_phys_ptr = (uint32_t) &new->tablesPhysical;
	tables_phys_ptr &= 0x0FFFFFFF;
	tables_phys_ptr += 0x00100000;

	current_directory = new;
	__asm__ volatile("mov %0, %%cr3" : : "r"(tables_phys_ptr));
	uint32_t cr0;
	__asm__ volatile("mov %%cr0, %0": "=r"(cr0));
	cr0 |= 0x80000000; // Enable paging!
	__asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

/*
 * Returns pointer to the specified page, and if not present and make = true,
 * creates it.
 *
 * Note that address = the logical address we wish to map.
 */
page_t* paging_get_page(uint32_t address, bool make, page_directory_t* dir) {
	// Turn the address into an index.
	address /= 0x1000;
	// Find the page table containing this address.
	uint32_t table_idx = address / 1024;

	if (dir->tables[table_idx]) { // If this table is already assigned
		return &dir->tables[table_idx]->pages[address % 0x400];
	} else if(make == true) {
		uint32_t tmp;
		dir->tables[table_idx] = (page_table_t *) kmalloc_ap(sizeof(page_table_t), &tmp);

		// update physical address
		uint32_t phys_ptr = tmp | 0x7;
		phys_ptr &= 0x0FFFFFFF; // get rid of high nybble
		phys_ptr += 0x00100000; // Add 1M offset
		dir->tablesPhysical[table_idx] = phys_ptr;

		return &dir->tables[table_idx]->pages[address % 0x400];
	} else {
		return 0;
	}
}

/*
 * Page fault handler
 */
void paging_page_fault_handler(err_registers_t regs) {
	// A page fault has occurred.
	// The faulting address is stored in the CR2 register.
	uint32_t faulting_address;
	__asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));

	// The error code gives us details of what happened.
	int present	= !(regs.err_code & 0x1); // Page not present
	int rw = regs.err_code & 0x2;			  // Write operation?
	int us = regs.err_code & 0x4;			  // Processor was in user-mode?
	int reserved = regs.err_code & 0x8;	  // Overwritten CPU-reserved bits of page entry?
	int id = regs.err_code & 0x10;			 // Caused by an instruction fetch?

	kprintf("Page fault! ( ");
	if (present) {kprintf("present ");}
	if (rw) {kprintf("read-only ");}
	if (us) {kprintf("user-mode ");}
	if (reserved) {kprintf("reserved ");}
	kprintf(") at 0x%X\n", faulting_address);

	while(1);

	PANIC("Page Fault");
}

/*
 * Flushes an address out of the MMU's cache.
 */
void paging_flush_tlb(uint32_t addr) {
	__asm__ volatile("invlpg (%0)" : : "r" (addr) : "memory");
}