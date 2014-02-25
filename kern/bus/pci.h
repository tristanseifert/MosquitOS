#ifndef PCI_H
#define PCI_H

#include <types.h>

#define PCI_GET_CLASS(x) ((x >> 0x18) & 0xFF)
#define PCI_GET_SUBCLASS(x) ((x >> 0x18) & 0xFF)
#define PCI_GET_PROGIF(x) ((x >> 0x8) & 0xFF)
#define PCI_GET_REVISION(x) (x & 0xFF)

#define pci_config_address(bus, device, function, reg) ((uint32_t) ((bus & 0xFF) << 16) | ((device & 0x1F) << 11) | ((function & 0x07) << 8) | (reg & 0xFF) | 0x80000000)

#define kPCIBARFlagsIOAddress (1 << 0)
#define kPCIBARFlags16Bits (1 << 1)
#define kPCIBARFlags32Bits (1 << 2)
#define kPCIBARFlags64Bits (1 << 3)
#define kPCIBARFlagsPrefetchable (1 << 4)

typedef struct {
	uint32_t start;
	uint32_t end;
	uint32_t flags;
} pci_bar_t;

typedef struct {
	uint16_t vendor;
	uint16_t device;
	uint32_t class;
	uint32_t class_mask;
} pci_ident_t;

typedef struct pci_loc {
	uint8_t bus, device, function;
} pci_loc_t;

typedef struct {
	pci_ident_t ident;

	uint32_t class;
	pci_bar_t bar[6];
} pci_function_t;

typedef struct {
	device_t d;

	pci_ident_t ident;
	pci_loc_t location;

	bool multifunction;

	pci_function_t function[8];
} pci_device_t;

typedef struct {
	device_t d;

	pci_ident_t ident;

	uint8_t bus_number;

	// The secondary bus is ONLY set if this is a bridge, otherwise it's 0xFFFF
	uint16_t bridge_secondary_bus;
} pci_bus_t;

void pci_config_write_l(uint32_t address, uint32_t value);
void pci_config_write_w(uint32_t address, uint16_t value);
void pci_config_write_b(uint32_t address, uint8_t value);

uint32_t pci_config_read_l(uint32_t address);
uint16_t pci_config_read_w(uint32_t address);
uint8_t pci_config_read_b(uint32_t address);

void pci_device_update_bar(pci_device_t *d, int function, int bar, uint32_t value);

#endif