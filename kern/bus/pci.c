#include <types.h>
#include "io/io.h"
#include "pci.h"
#include "pci_tables.h"

// A bus number is valid if it's number corresponds to a non-NULL pointer
static pci_bus_t* pci_bus_info[256];

/*
 * Reads from PCI config space with the specified address
 */
static uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg) {
	uint32_t address;

	address = (uint32_t) ((bus << 16) | (device << 11) | (function << 8) | (reg & 0xFC) | 0x80000000);

	io_outl(0xCF8, address);
	return io_inl(0xCFC);
}

/*
 * Tries to find all devices on a given bus through brute-force.
 */
static void pci_probe_bus(pci_bus_t *bus) {
	uint32_t temp;

	uint8_t bus_number = bus->bridge_secondary_bus;

	for(int i = 0; i < 32; i++) {
		temp = pci_config_read(bus_number, i, 0, 0x00);
		uint16_t vendor = temp & 0xFFFF;
		uint16_t device = temp >> 0x10;

		if(vendor != 0xFFFF) { // Does the device exist?
			bus->devices[i].vendor_id = vendor;
			bus->devices[i].device_id = device;
		} else {
			bus->devices[i].vendor_id = 0xFFFF;
		}
	}
}

/*
 * Searches through every bus on the system and reads info if it's valid.
 */
static void pci_enumerate_busses() {
	uint32_t temp;

	for(int i = 0; i < 256; i++) {
		temp = pci_config_read(i, 0, 0, 0x00);
		uint16_t vendor = temp & 0xFFFF;
		uint16_t device = temp >> 0x10;

		if(vendor != 0xFFFF) { // is there something on this bus?
			// Read class/subclass/rev register
			temp = pci_config_read(i, 0, 0, 0x08);
			uint8_t class = (temp & 0xFF000000) >> 0x18;
			uint8_t subclass = (temp & 0x00FF0000) >> 0x10;

			// We found a bridge
			if(class == 0x06) {
				pci_bus_t *bus = (pci_bus_t *) kmalloc(sizeof(pci_bus_t));
				memclr(bus, sizeof(pci_bus_t));
				pci_bus_info[i] = bus;

				uint32_t busInfo = pci_config_read(i, 0, 0, 0x18);

				bus->bus_number = i;
				bus->isBridge = true;
				bus->bridge_secondary_bus = (busInfo & 0x0000FF00) >> 8;

				bus->vendor_id = vendor; bus->device_id = device;

				pci_probe_bus(bus);
			} else { // It isn't a bridge, but maybe this bus does have something on it
				uint32_t class = pci_config_read(i, 0, 0, 0x08) >> 0x18;
			}
		}
	}
}

/*
 * Tries to find info about the vendor.
 */
static pci_str_vendor_t* pci_info_get_vendor(uint16_t vendor) {
	for(int i = 0; i < PCI_VENDOR_MAP_LENGTH; i++) {
		if(unlikely(pci_map_vendor[i].vendor_id == vendor)) {
			return &pci_map_vendor[i];
		}
	}

	return NULL;
}

/*
 * Tries to find some info about the given device by the specified vendor.
 */
static pci_str_device_t* pci_info_get_device(uint16_t vendor, uint16_t device) {
	for(int i = 0; i < PCI_DEVTABLE_LEN; i++) {
		if(unlikely((pci_map_device[i].vendor_id == vendor) && pci_map_device[i].device_id == device)) {
			return &pci_map_device[i];
		}
	}

	return NULL;
}

/*
 * Prints a pretty representation of the system's PCI devices.
 */
static void pci_print_tree() {
	for(int i = 0; i < 256; i++) {
		pci_bus_t *bus = pci_bus_info[i];

		if(bus) {
			kprintf("Bus %i:\n", i);

			for(int d = 0; d < 32; d++) {
				if(bus->devices[d].vendor_id != 0xFFFF) {
					uint16_t vendor = bus->devices[d].vendor_id;
					uint16_t device = bus->devices[d].device_id;

					pci_str_vendor_t* vinfo = pci_info_get_vendor(vendor);
					pci_str_device_t* dinfo = pci_info_get_device(vendor, device);

					if(vinfo && dinfo) {
						kprintf("  Device %i[%4X:%4X]: %s %s\n", d, vendor, device, vinfo->vendor_full, dinfo->chip_desc);
					} else if(dinfo) {
						kprintf("  Device %i[%4X:%4X]: Unknown %s\n", d, vendor, device, dinfo->chip_desc);
					} else if(vinfo) {
						kprintf("  Device %i[%4X:%4X]: %s Unknown\n", d, vendor, device, vinfo->vendor_full);
					} else {
						kprintf("  Device %i[%4X:%4X]\n", d, vendor, device);
					}
				}
			}
		}
	}
}

/*
 * Initialise PCI subsystem
 */
static int pci_init(void) {
	memclr(pci_bus_info, 256*sizeof(pci_bus_t*));

	kprintf("PCI: Probing bus...\n");

	pci_enumerate_busses();
	pci_print_tree();

	kprintf("PCI: Bus enumerated.\n");

	return -1;
}

/*
 * Clean up resources and stop devices, unload any drivers that were loaded
 * as a response to PCI devices
 */
static void pci_exit(void) {

}

module_init(pci_init);
module_exit(pci_exit);