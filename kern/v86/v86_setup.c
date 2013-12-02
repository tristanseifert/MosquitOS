#include "v86_setup.h"
#include <sys/cpuid.h>
#include <sys/paging.h>
#include <types.h>

static void* v86_BIOS_IDT;
static void* v86_BIOS_BDA;

static uint32_t v86_memory_phys;
static void* v86_memory;
static page_directory_t *v86_pagedir;

extern uint32_t v86_trampoline, v86_trampoline_end;

/*
 * Initialises Virtual 8086 mode.
 */
static int v86_init(void) {
	// Make sure CPU supports Enhanced V86
	uint32_t eax, ebx, ecx, edx;
	cpuid(0x00000000, eax, ebx, ecx, edx);

	if(!(edx | CPUID_R_D_ENHANCEDV86)) {
		kprintf("CPU does not support Enhanced Virtual 8086 Mode\n");
		return -1;
	}

	// Copy the BIOS' IDT to somewhere in our memory space
	v86_BIOS_IDT = (void *) kmalloc(0x400);
	memcpy(v86_BIOS_IDT, (void*) 0x00000000, 0x400);

	// Also, copy BIOS data area
	v86_BIOS_BDA = (void *) kmalloc(0x100);
	memcpy(v86_BIOS_BDA, (void*) 0x00000400, 0x100);

	// Allocate 640KB (160 pages) of memory for V86
	v86_memory = (void *) kmalloc_int(0xA0000, true, &v86_memory_phys);
	memclr(v86_memory, 0xA0000);

	// Get a pagetable.
	v86_pagedir = paging_new_directory();

	// Map the V8086 copy of memory
	uint32_t i;
	for(i = 0x00000000; i < 0x0009F000; i += 0x1000) {
		page_t* page = paging_get_page(i, true, v86_pagedir);

		memclr(page, sizeof(page_t));

		page->present = 1;
		page->rw = 1;
		page->user = 1;
		page->frame = ((i + v86_memory_phys) >> 12);
	}

	// Identity map from 0xA0000 to 0xFFFFF
	for(i = 0x000A0000; i < 0x000FF000; i += 0x1000) {
		page_t* page = paging_get_page(i, true, v86_pagedir);
		memclr(page, sizeof(page_t));
		page->present = 1;
		page->rw = 1;
		page->user = 1;
		page->frame = (i >> 12);
	}

	// Copy the trampoline to the start of our V86 memory
	uint32_t v86_trampoline_length = ((uint32_t) &v86_trampoline_end) - ((uint32_t) &v86_trampoline);

	memcpy(v86_memory, (void *)&v86_trampoline, v86_trampoline_length);

	return 0;
}

/*
 * Cleans up the resources related to V86 mode.
 */
static void v86_exit(void) {
	// Free memory
	kfree(v86_BIOS_IDT);
	kfree(v86_BIOS_BDA);
	kfree(v86_memory);

	// Release paging directory
	for(int i = 0; i < 1024; i++) {
		kfree(v86_pagedir->tables[i]);
	}

	kfree(v86_pagedir);
}

module_init(v86_init);
module_exit(v86_exit);

/*
 * Loads num_bytes from in_data* to the specified V86 segment and offset.
 */
void v86_copyin(void* in_data, size_t num_bytes, v86_address_t address) {
	// Ensure we don't write more than is safe
	if(v86_segment_to_linear(address) + num_bytes > 0xA0000) {
		num_bytes -= (v86_segment_to_linear(address) + num_bytes) - 0xA0000;
	}

	if(v86_segment_to_linear(address) < 0xA0000 && num_bytes < 0xA0000) {
		memcpy(v86_memory + v86_segment_to_linear(address), in_data, num_bytes);
	}
}

/*
 * Reads num_bytes starting at V86 address to out_data. Note that buffer
 * overflows can easily occurr if the buffer isn't big enough to hold all the
 * requested data.
 */
void* v86_copyout(v86_address_t address, void* out_data, size_t num_bytes) {
	// Ensure we don't read past the end of the V86 memory
	if(v86_segment_to_linear(address) + num_bytes > 0xA0000) {
		num_bytes -= (v86_segment_to_linear(address) + num_bytes) - 0xA0000;
	}

	if(v86_segment_to_linear(address) < 0xA0000 && num_bytes < 0xA0000) {
		memcpy(out_data, v86_memory + v86_segment_to_linear(address), num_bytes);
		return out_data;
	}

	return NULL;
}

/*
 * Read/write a word from the VM address space.
 */
uint16_t v86_peekw(v86_address_t address) {
	if(v86_segment_to_linear(address) < 0xA0000) {
		uint16_t *tmp = v86_memory;
		return tmp[v86_segment_to_linear(address)>>1];
	}

	return 0xFFFF;
}

void v86_pokew(v86_address_t address, uint16_t value) {
	if(v86_segment_to_linear(address) < 0xA0000) {
		uint16_t *tmp = v86_memory;
		tmp[v86_segment_to_linear(address)>>1] = value;
	}
}

/*
 * Read/write a byte from the VM address space.
 */
uint8_t v86_peekb(v86_address_t address) {
	if(v86_segment_to_linear(address) < 0xA0000) {
		uint8_t *tmp = v86_memory;
		return tmp[v86_segment_to_linear(address)];
	}

	return 0xFF;
}

void v86_pokeb(v86_address_t address, uint8_t value) {
	if(v86_segment_to_linear(address) < 0xA0000) {
		uint8_t *tmp = v86_memory;
		tmp[v86_segment_to_linear(address)] = value;
	}
}