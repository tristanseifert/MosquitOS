#ifndef PCI_H
#define PCI_H

#include <types.h>

typedef struct {
	uint16_t vendor_id, device_id;
	uint8_t class, subclass;
} pci_function_t;

typedef struct {
	uint16_t vendor_id, device_id;
	bool multifunction;

	pci_function_t function[8];
} pci_device_t;

typedef struct {
	uint8_t bus_number;

	bool isBridge;
	uint8_t bridge_secondary_bus;

	uint16_t vendor_id, device_id;

	pci_device_t devices[32];
} pci_bus_t;

#endif