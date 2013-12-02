#include <types.h>
#include "io/io.h"
#include "pci.h"
#include "pci_tables.h"

// We have initialised a faster way to do config reads rather than IO if this is set
static bool pci_fast_config_avail;
// A bus number is valid if it's number corresponds to a non-NULL pointer
static pci_bus_t* pci_bus_info[256];

/*
 * Reads from PCI config space with the specified address
 */
static uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg) {
	if(!pci_fast_config_avail) {
		uint32_t address;

		address = (uint32_t) ((bus << 16) | (device << 11) | (function<< 8) | (reg & 0xFC) | 0x80000000);

		io_outl(0xCF8, address);
		return io_inl(0xCFC);
	} else {
		return 0xFFFFFFFF;
	}
}

/*
 * Gets info about the device's function.
 */
static void pci_set_function_info(uint8_t bus, uint8_t device, uint8_t function) {
	uint32_t temp;
	pci_function_t *info = &pci_bus_info[bus]->devices[device].function[function];

	info->class = pci_config_read(bus, device, function, 0x08);
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
			bus->devices[i].ident.vendor = vendor;
			bus->devices[i].ident.device = device;
			bus->devices[i].ident.class = pci_config_read(bus_number, device, 0, 0x08);
			bus->devices[i].ident.class_mask = 0xFFFFFFFF;

			bus->devices[i].location.bus = bus_number;
			bus->devices[i].location.device = i;
			bus->devices[i].location.function = 0;

			// Does device have multiple functions?
			temp = pci_config_read(bus_number, i, 0, 0x0C);
			uint8_t header_type = temp >> 0x10;

			// Yes, go through all of them
			if(header_type & 0x80) {
				bus->devices[i].multifunction = true;

				for(uint8_t f = 0; f < 7; f++) {
					temp = pci_config_read(bus_number, i, f, 0x00);
					vendor = temp & 0xFFFF;

					bus->devices[i].function[f].ident.vendor = vendor;
					bus->devices[i].function[f].ident.device = temp >> 0x10;

					// This function is defined
					if(vendor != 0xFFFF) {
						pci_set_function_info(bus_number, i, f);
					}
				}
			} else {
				pci_set_function_info(bus_number, i, 0);
				bus->devices[i].multifunction = false;
			}
		} else {
			bus->devices[i].ident.vendor = 0xFFFF;
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
	// Check all possible busses
	for(int i = 0; i < 256; i++) {
		pci_bus_t *bus = pci_bus_info[i];

		// Is bus defined?
		if(bus) {
			kprintf("Bus %i:\n", i);

			// Search through all devices
			for(int d = 0; d < 32; d++) {
				// Process multifunction device
				if(bus->devices[d].ident.vendor != 0xFFFF) {
					uint16_t vendor = bus->devices[d].ident.vendor;
					uint16_t device = bus->devices[d].ident.device;

					if(bus->devices[d].multifunction) {
						kprintf("  Device %i: Multifunction\n", d);

						for(int f = 0; f < 8; f++) {
							pci_function_t function = bus->devices[d].function[f];
							vendor = function.ident.vendor;
							device = function.ident.device;

							// This function is defined
							if(vendor != 0xFFFF) {
								pci_str_vendor_t* vinfo = pci_info_get_vendor(vendor);
								pci_str_device_t* dinfo = pci_info_get_device(vendor, device);

								if(vinfo && dinfo) {
									kprintf("    Function %i[%4X:%4X %2X:%2X]: %s %s\n", f, vendor, device, PCI_GET_CLASS(function.class), PCI_GET_SUBCLASS(function.class), vinfo->vendor_full, dinfo->chip_desc);
								} else {
									kprintf("    Function %i[%4X:%4X %2X:%2X]: Unknown\n", f, vendor, device, PCI_GET_CLASS(function.class), PCI_GET_SUBCLASS(function.class));
								}
							}
						}
					} else {
						pci_str_vendor_t* vinfo = pci_info_get_vendor(vendor);
						pci_str_device_t* dinfo = pci_info_get_device(vendor, device);
						kprintf("  Device %i[%4X:%4X]: %s %s\n", d, vendor, device, vinfo->vendor_full, dinfo->chip_desc);
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
	pci_fast_config_avail = false;

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

module_bus_init(pci_init);
module_exit(pci_exit);