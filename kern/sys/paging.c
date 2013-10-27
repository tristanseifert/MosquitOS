#include <types.h>

#include "paging.h"
#include "kheap.h"
#include "runtime/error_handler.h"
#include "io/terminal.h"

// The kernel's page directory
page_directory_t *kernel_directory = 0;
// The current page directory;
page_directory_t *current_directory = 0;

// A bitset of frames - used or free.
uint32_t *frames;
uint32_t nframes;

// Defined in kheap.c
extern uint32_t kheap_placement_address;
extern heap_t *kheap;

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
		if (frames[i] != 0xFFFFFFFF) { // nothing free, exit early
			// at least one bit is free here
			for (j = 0; j < 32; j++) {
				uint32_t toTest = 0x1 << j;
				if (!(frames[i]&toTest)) {
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
void alloc_frame(page_t *page, bool is_kernel, bool is_writeable) {
	if (page->frame != 0) {
		return;
	} else {
		uint32_t idx = first_frame();
		if (idx == (uint32_t)-1) {
			PANIC("No Free Frames");
			// PANIC! no free frames!!
		}
		set_frame(idx*0x1000);
		page->present = 1;
		page->rw = (is_writeable)?1:0;
		page->user = (is_kernel)?0:1;
		page->frame = idx;
	}
}

/*
 * Function to deallocate a frame.
 */
void free_frame(page_t *page) {
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
    // The size of physical memory. For the moment we 
    // assume it is 32MB big.
    uint32_t mem_end_page = 0x02000000;
    
    nframes = mem_end_page / 0x1000;
    frames = (uint32_t *) kmalloc(INDEX_FROM_BIT(nframes));
    memset(frames, 0, INDEX_FROM_BIT(nframes));
    
    // Let's make a page directory.
    kernel_directory = (page_directory_t *)kmalloc_a(sizeof(page_directory_t));
    memset(kernel_directory, 0, sizeof(page_directory_t));
    current_directory = kernel_directory;

    // Map some pages in the kernel heap area.
    // Here we call get_page but not alloc_frame. This causes page_table_t's 
    // to be created where necessary. We can't allocate frames yet because they
    // they need to be identity mapped first below, and yet we can't increase
    // placement_address between identity mapping and enabling the heap!
    int i = 0;
    for (i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += 0x1000) {
        paging_get_page(i, 1, kernel_directory);
    }

    // We need to identity map (phys addr = virt addr) from
    // 0x0 to the end of used memory, so we can access this
    // transparently, as if paging wasn't enabled.
    // NOTE that we use a while loop here deliberately.
    // inside the loop body we actually change placement_address
    // by calling kmalloc(). A while loop causes this to be
    // computed on-the-fly rather than once at the start.
    // Allocate a lil' bit extra so the kernel heap can be
    // initialised properly.
    i = 0;
    while (i < kheap_placement_address + 0x1000) {
        // Kernel code is readable but not writeable from userspace.
        alloc_frame(paging_get_page(i, 1, kernel_directory), 0, 0);
        i += 0x1000;
    }

    // Now allocate those pages we mapped earlier.
    for (i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += 0x1000) {
        alloc_frame(paging_get_page(i, 1, kernel_directory), 0, 0);
	}

    // Now, enable paging!
    paging_switch_directory(kernel_directory);

    // Initialise the kernel heap.
    kheap = create_heap(KHEAP_START, KHEAP_START+KHEAP_INITIAL_SIZE, 0xCFFFF000, 0, 0);
}

/*
 * Switches the currently-used page directory.
 */
void paging_switch_directory(page_directory_t *new) {
	current_directory = new;
	__asm__ volatile("mov %0, %%cr3":: "r"(&new->tablesPhysical));
	uint32_t cr0;
	__asm__ volatile("mov %%cr0, %0": "=r"(cr0));
	cr0 |= 0x80000000; // Enable paging!
	__asm__ volatile("mov %0, %%cr0":: "r"(cr0));

}

/*
 * Returns pointer to the specified page, and if not present and make = true,
 * creates it.
 */
page_t *paging_get_page(uint32_t address, bool make, page_directory_t *dir) {
	// Turn the address into an index.
	address /= 0x1000;
	// Find the page table containing this address.
	uint32_t table_idx = address / 1024;
	if (dir->tables[table_idx]) { // If this table is already assigned
		return &dir->tables[table_idx]->pages[address % 0x400];
	} else if(make == true) {
		uint32_t tmp;
		dir->tables[table_idx] = (page_table_t *) kmalloc_ap(sizeof(page_table_t), &tmp);
		dir->tablesPhysical[table_idx] = tmp | 0x7; // PRESENT, RW, US.
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

	terminal_setPos(0, 0);

	terminal_write_string("Page fault! ( ");
	if (present) {terminal_write_string("present ");}
	if (rw) {terminal_write_string("read-only ");}
	if (us) {terminal_write_string("user-mode ");}
	if (reserved) {terminal_write_string("reserved ");}
	terminal_write_string(") at 0x");
	terminal_write_dword(faulting_address);
	terminal_write_string("\n");

	while(1);

	PANIC("Page Fault");
}