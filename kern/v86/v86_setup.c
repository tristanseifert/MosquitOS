#include "v86_setup.h"
#include <sys/cpuid.h>
#include <types.h>

/*
 * Initialises Virtual 8086 mode.
 */
int v86_init() {
	// Make sure CPU supports Enhanced V86
	uint32_t eax, ebx, ecx, edx;
	cpuid(0x00000000, eax, ebx, ecx, edx);

	if(!(edx | CPUID_R_D_ENHANCEDV86)) {
		kprintf("CPU is missing Enhanced V86 support, cannot use V86 handler\n");
		return -1;
	}

	return 0;
}

/*
 * Loads num_bytes from in_data* to the specified V86 segment and offset.
 */
void v86_copyin(void* in_data, size_t num_bytes, v86_address_t address) {

}

/*
 * Reads num_bytes starting at V86 address to out_data. Note that buffer
 * overflows can easily occurr if the buffer isn't big enough to hold all the
 * requested data.
 */
void* v86_copyout(v86_address_t address, void* out_data, size_t num_bytes) {
	return NULL;
}