#include <types.h>
#include "apic.h"
#include "sys/cpuid.h"
#include "modules/module.h"

/*
 * Returns true if the CPU supports the APIC.
 */
static bool apic_supported() {
	uint32_t eax, ebx, ecx, edx;
	cpuid(1, eax, ebx, ecx, edx);
	return edx & CPUID_FEAT_EDX_APIC;
}

/*
 * Module initialisation/exit functions.
 */
static int apic_init(void) {
	if(!apic_supported()) {
		kprintf("No APIC found.\n");
		return -1;
	}

	// Initialise APIC timer and IRQ routing based on ACPI

	// Override IRQ handler hooks, disable PIT

	return 0;
}

module_init(apic_init);