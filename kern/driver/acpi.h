#ifndef ACPI_H
#define ACPI_H

#include <types.h>

typedef struct acpi_rsdp_descriptor acpi_rsdp_descriptor_t;
typedef struct acpi_rsdp_descriptor_v2 acpi_rsdp_descriptor_v2_t;

struct acpi_rsdp_descriptor {
	char Signature[8];
	uint8_t Checksum;
	char OEMID[6];
	uint8_t Revision;
	uint32_t RsdtAddress;
};

struct acpi_rsdp_descriptor_v2 {
	char Signature[8];
	uint8_t Checksum;
	char OEMID[6];
	uint8_t Revision;
	uint32_t RsdtAddress;
	 
	uint32_t Length;
	uint64_t XsdtAddress;
	uint8_t ExtendedChecksum;
	uint8_t reserved[3];
};

#endif