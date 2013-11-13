#include <types.h>
#include "pci.h"

/*
 * Translates the device and bus addresses to an address that can be written to the PCI
 * configuration IO address register at 0xCF8.
 */
uint32_t pci_make_config_address(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg) {
	// Set the high bit (ENABLE) to translate address into configuration cycles
	uint32_t address = 0x80000000;

	// Low 2 bits are discarded
	address |= (reg & 0xFC);

	// Apply function
	address |= (function & 0x07) << 8;

	// Device and bus address
	address |= (device & 0x1F) << 11;
	address |= bus << 24;

	return address;
}