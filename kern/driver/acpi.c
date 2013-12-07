#include <types.h>
#include "acpi.h"
#include "sys/paging.h"

extern page_directory_t *kernel_directory;

static acpi_rsdp_descriptor_t* acpi_rsdp;

/*
 * Verifies if there is a valid RSDP at this pointer.
 */
static acpi_rsdp_descriptor_t* acpi_validate_rsdp(void* ptr) {
	if(memcmp(ptr, "RSD PTR ", 8) != 0) {
		goto error;
	}

	// Verify checksum of the structure
	uint8_t checksum = 0;
	for(int i = 0; i < sizeof(acpi_rsdp_descriptor_t); i++) {
		checksum += ((uint8_t *) ptr)[i];
	}

	// It should be zero
	if((checksum && 0xFF) != 0) {
		goto error;
	}

	// We found a valid RSDP descriptor.
	return (acpi_rsdp_descriptor_t *) ptr;

	error: ;
	return NULL;
}

/*
 * Initialises the ACPI driver, locating the RSDP and other information that is
 * required for ACPI to function. It also puts the computer into ACPI mode so
 * the features it provides can be used.
 */
static int acpi_init(void) {
	void *searchStart = (void *) 0x90000;

	acpi_rsdp_descriptor_t *rsdp = NULL;

	// RSDP is never above 0x01000000
	while(((int) searchStart) < 0x100000) {
		rsdp = acpi_validate_rsdp(searchStart);

		if(rsdp) {
			acpi_rsdp = (acpi_rsdp_descriptor_t *) kmalloc(sizeof(acpi_rsdp_descriptor_v2_t));
			memcpy(acpi_rsdp, rsdp, sizeof(acpi_rsdp_descriptor_v2_t));

			uint32_t rsdtLogical = paging_map_section(acpi_rsdp->RSDTAddress, sizeof(acpi_rsdt_t), kernel_directory, kMemorySectionHardware);
			acpi_rsdt_t *rsdt = (acpi_rsdt_t *) rsdtLogical;
		}

		searchStart += 0x10;
	}

	return -1;
}

module_init(acpi_init);