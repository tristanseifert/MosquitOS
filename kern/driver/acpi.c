#include <types.h>
#include "acpi.h"

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
	if(checksum != 0) {
		goto error;
	}

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

}

module_init(acpi_init);