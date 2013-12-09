#include <types.h>
#include "apic.h"
#include "sys/cpuid.h"
#include "modules/module.h"

/*
 * Returns true if the CPU supports the APIC.
 */
bool apic_supported() {
	uint32_t eax, ebx, ecx, edx;
	cpuid(1, eax, ebx, ecx, edx);
	return edx & CPUID_FEAT_EDX_APIC;
}

/*
 * Module initialisation/exit functions.
 */
static int apic_init(void) {
	return 0;
}

module_init(apic_init);