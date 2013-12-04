#ifndef ACPI_H
#define ACPI_H

#include <types.h>

typedef struct acpi_rsdp_descriptor acpi_rsdp_descriptor_t;
typedef struct acpi_rsdp_descriptor_v2 acpi_rsdp_descriptor_v2_t;
typedef struct acpi_table_header acpi_table_header_t;
typedef struct acpi_rsdt acpi_rsdt_t;

struct acpi_rsdp_descriptor {
	char signature[8];
	uint8_t checksum;
	char oem_id[6];
	uint8_t Rev;
	uint32_t RSDTAddress;
} __attribute__((packed));

struct acpi_rsdp_descriptor_v2 {
	char signature[8];
	uint8_t checksum;
	char oem_id[6];
	uint8_t Rev;
	uint32_t RSDTAddress;
	 
	uint32_t length;
	uint64_t XSDTAddress;
	uint8_t extendedChecksum;
	uint8_t reserved[3];
} __attribute__((packed));

struct acpi_table_header {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem_ID[6];
	char oem_table_ID[8];
	uint32_t oemRev;
	uint32_t creatorID;
	uint32_t creatorRev;
} __attribute__((packed));

struct acpi_rsdt {
	acpi_table_header_t h;
	uint32_t pointerToSDTs[256];
};


#endif