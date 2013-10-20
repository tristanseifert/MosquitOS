#include "kheap.h"

// end is defined in the linker script.
uint32_t kheap_placement_address = (uint32_t) 0x00480000; // After kernel

uint32_t kmalloc_int(uint32_t sz, int align, uint32_t *phys) {
	// This will eventually call malloc() on the kernel heap.
	// For now, though, we just assign memory at kheap_placement_address
	// and increment it by sz. Even when we've coded our kernel
	// heap, this will be useful for use before the heap is initialised.
	if (align == 1 && (kheap_placement_address & 0xFFFFF000)) {
		// Align the placement address;
		kheap_placement_address &= 0xFFFFF000;
		kheap_placement_address += 0x1000;
	} 

	if (phys) {
		*phys = kheap_placement_address;
	}

	uint32_t tmp = kheap_placement_address;
	kheap_placement_address += sz;
	return tmp;
}

uint32_t kmalloc_a(uint32_t sz) {
	return kmalloc_int(sz, 1, 0);
}

uint32_t kmalloc_p(uint32_t sz, uint32_t *phys) {
	return kmalloc_int(sz, 0, phys);
}

uint32_t kmalloc_ap(uint32_t sz, uint32_t *phys) {
	return kmalloc_int(sz, 1, phys);
}

uint32_t kmalloc(uint32_t sz) {
	return kmalloc_int(sz, 0, 0);
}
