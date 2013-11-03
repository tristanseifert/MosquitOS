#ifndef PAGING_H
#define PAGING_H

#include <types.h>

#define SYS_PAGE_DIR_ADDR 0x00400000

/*
 * Contains functions to set up and deal with paging.
 */
typedef struct page {
	uint32_t present:1;	// Page present in memory
	uint32_t rw:1;		// Read-only if clear, readwrite if set
	uint32_t user:1;		// Supervisor level only if clear
	uint32_t accessed:1;	// Has the page been accessed since last refresh?
	uint32_t dirty:1;	// Has the page been written to since last refresh?
	uint32_t unused:7;	// Amalgamation of unused and reserved bits
	uint32_t frame:20;	// Frame address (shifted right 12 bits)
} page_t;

typedef struct page_table {
	page_t pages[1024];
} page_table_t;

typedef struct page_directory {
	// Array of ptrs to page_table structs.
	page_table_t* tables[1024];
	// Array of pointers to page tables above, but giving their physical location
	uint32_t tablesPhysical[1024];
	// Physical address of tablesPhysical.
	uint32_t physicalAddr;
} page_directory_t;

void alloc_frame(page_t* page, bool is_kernel, bool is_writeable);
void free_frame(page_t* page);

void paging_init();
void paging_switch_directory(page_directory_t* new);
page_t* paging_get_page(uint32_t address, bool make, page_directory_t* dir);

void paging_page_fault_handler();

#endif