#include <types.h>
#include "io/io.h"
#include "bus.h"
#include "pci.h"
#include "pci_tables.h"

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

// Struct passed to bus driver
static bus_t pci_bus = {
	.match = pci_match
};

// We have initialised a faster way to do config reads rather than IO if this is set
static bool pci_fast_config_avail;

/*
 * Performs either a longword, word or byte write to PCI config space.
 */
void pci_config_write_l(uint32_t address, uint32_t value) {
	io_outl(PCI_CONFIG_ADDR, address);
	io_outl(PCI_CONFIG_DATA, value);
}

void pci_config_write_w(uint32_t address, uint16_t value) {
	io_outl(PCI_CONFIG_ADDR, address);
	io_outl(PCI_CONFIG_DATA + (address & 0x02), value);

}

void pci_config_write_b(uint32_t address, uint8_t value) {
	io_outl(PCI_CONFIG_ADDR, address);
	io_outl(PCI_CONFIG_DATA + (address & 0x3), value);
}

/*
 * Performs either a longword, word or byte read from PCI config space
 */
uint32_t pci_config_read_l(uint32_t address) {
	io_outl(PCI_CONFIG_ADDR, address);
	return io_inl(PCI_CONFIG_DATA);
}

uint16_t pci_config_read_w(uint32_t address) {
	io_outl(PCI_CONFIG_ADDR, address);
	return io_inl(PCI_CONFIG_DATA + (address & 0x2));

}

uint8_t pci_config_read_b(uint32_t address) {
	io_outl(PCI_CONFIG_ADDR, address);
	return io_inl(PCI_CONFIG_DATA+ (address & 0x3));
}

/*
 * Reads from PCI config space with the specified address
 */
static uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg) {
	if(!pci_fast_config_avail) {
		uint32_t address;

		address = pci_config_address(bus, device, function, reg);

		io_outl(PCI_CONFIG_ADDR, address);
		return io_inl(PCI_CONFIG_DATA);
	} else {
		return 0xFFFFFFFF;
	}
}

/*
 * Updates the BAR of a specific function on a device, and updates the PCI
 * config space.
 */
void pci_device_update_bar(pci_device_t *d, int function, int bar, uint32_t value) {
	ASSERT(bar < 6);

	// Calculate length and address to write to
	uint32_t length = d->function[function].bar[bar].end - d->function[function].bar[bar].start;
	uint32_t addr = pci_config_address(d->location.bus, d->location.device, function, (0x10 + (bar << 2)));

	kprintf("PCI BAR UPDATE: bus %u, device %u, function %u, BAR %X, value 0x%X, address 0x%X\n", d->location.bus, d->location.device, function, (0x10 + (bar << 2)), value, addr);

	// Do write to PCI config space
	pci_config_write_l(addr, value);

	// Reset flags
	d->function[function].bar[bar].flags = 0;

	// IO BAR
	if(bar & 0x01) {
		d->function[function].bar[bar].start = value & 0xFFFFFFFC;
		d->function[function].bar[bar].end = (value & 0xFFFFFFFC) + length;
		d->function[function].bar[bar].flags = kPCIBARFlagsIOAddress;
	} else { // Memory address BAR
		d->function[function].bar[bar].start = value & 0xFFFFFFF0;
		d->function[function].bar[bar].end = (value & 0xFFFFFFF0) + length;

		// Is this BAR prefetchable?
		if(value & 0x8) {
			d->function[function].bar[bar].flags = kPCIBARFlagsPrefetchable;
		}

		// Get BAR type
		uint8_t type = (value & 0x6) >> 1;

		// Is the BAR 64 bits?
		if(type == 0x02) {
			d->function[function].bar[bar].flags |= kPCIBARFlags64Bits;
		} else if(type == 0x01) { // 16 bits?
			d->function[function].bar[bar].flags |= kPCIBARFlags16Bits;
		} else if(type == 0x00) { // 32 bits?
			d->function[function].bar[bar].flags |= kPCIBARFlags32Bits;
		}
	}
}

/*
 * Gets info about the device's function.
 */
static void pci_set_function_info(uint8_t bus, uint8_t device, uint8_t function, pci_function_t* finfo) {
	uint32_t temp;

	finfo->class = pci_config_read(bus, device, function, 0x08);

	uint8_t header_type = ((pci_config_read(bus, device, function, 0x0C)) >> 0x10) & 0x7F;

	int num_bars = 0;

	// Device has six BARs if header type == 0x00
	if(header_type == 0x00) num_bars = 6;
	// Device has two BARs if header type == 0x01
	if(header_type == 0x01) num_bars = 2;

	for (unsigned int b = 0; b < num_bars; b++) {
		finfo->bar[b].flags = 0;

		// Get address of BAR: 0x10 is first, each BAR is 4 bytes long
		uint32_t addr = pci_config_address(bus, device, function, (0x10 + (b << 2)));

		// Read original BAR
		uint32_t bar = pci_config_read_l(addr);

		// Write all 1's to get size required
		pci_config_write_l(addr, 0xFFFFFFFF);

		// Read size
		uint32_t size = pci_config_read_l(addr);

		// IO BAR
		if(bar & 0x01) {
			size &= 0xFFFFFFFC;
			finfo->bar[b].start = bar & 0xFFFFFFFC;
			finfo->bar[b].end = finfo->bar[b].start + (~(size) + 1);
			finfo->bar[b].flags = kPCIBARFlagsIOAddress;
		} else { // Memory address BAR
			size &= 0xFFFFFFF0;
			finfo->bar[b].start = bar & 0xFFFFFFF0;
			finfo->bar[b].end = finfo->bar[b].start + (~(size) + 1);

			// Is this BAR prefetchable?
			if(bar & 0x8) {
				finfo->bar[b].flags = kPCIBARFlagsPrefetchable;
			}

			// Get BAR type
			uint8_t type = (bar & 0x6) >> 1;

			// Is the BAR 64 bits?
			if(type == 0x02) {
				finfo->bar[b].flags |= kPCIBARFlags64Bits;
			} else if(type == 0x01) { // 16 bits?
				finfo->bar[b].flags |= kPCIBARFlags16Bits;
			} else if(type == 0x00) { // 32 bits?
				finfo->bar[b].flags |= kPCIBARFlags32Bits;
			}
		}

		// Restore BAR value
		pci_config_write_l(addr, bar);
	}
}

/*
 * Tries to find all devices on a given bus through brute-force.
 */
static void pci_probe_bus(pci_bus_t *bus) {
	uint32_t temp, temp2;
	uint8_t bus_number;

	if(bus->bridge_secondary_bus != 0xFFFF) {
		bus_number = bus->bridge_secondary_bus & 0x00FF;
	} else {
		bus_number = bus->bus_number;
	}

	for(int i = 0; i < 32; i++) {
		temp = pci_config_read(bus_number, i, 0, 0x00);
		uint16_t vendor_id = temp & 0xFFFF;
		uint16_t device_id = temp >> 0x10;

		if(vendor_id != 0xFFFF) { // Does the device exist?
			pci_device_t *device = (pci_device_t *) kmalloc(sizeof(pci_device_t));

			device->ident.vendor = vendor_id;
			device->ident.device = device_id;
			device->ident.class = pci_config_read(bus_number, i, 0, 0x08);
			device->ident.class_mask = 0xFFFFFFFF;

			device->location.bus = bus_number;
			device->location.device = i;
			device->location.function = 0;

			// Does device have multiple functions?
			temp = pci_config_read(bus_number, i, 0, 0x0C);
			uint8_t header_type = temp >> 0x10;

			// If so, go through all of them
			if(header_type & 0x80) {
				device->multifunction = true;

				memclr(&device->function[0], sizeof(pci_function_t)*8);

				for(uint8_t f = 0; f < 7; f++) {
					temp = pci_config_read(bus_number, i, f, 0x00);
					vendor_id = temp & 0xFFFF;

					device->function[f].ident.vendor = vendor_id;

					// This function is defined
					if(vendor_id != 0xFFFF) {
						pci_set_function_info(bus_number, i, f, &device->function[f]);

						device->function[f].ident.device = temp >> 0x10;
					}
				}
			} else {
				pci_set_function_info(bus_number, i, 0, &device->function[0]);
				device->multifunction = false;
			}

			// Add "device" to the bus' node.children, and set the bus' node.parent.
			device->d.node.name = "PCI Device";
			device->d.node.parent = &bus->d.node;

			list_add(bus->d.node.children, device);	
		}
	}
}

/*
 * Searches through every bus on the system and reads info if it's valid.
 */
void pci_enumerate_busses() {
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

				uint32_t busInfo = pci_config_read(i, 0, 0, 0x18);

				bus->bus_number = i;

				if((busInfo & 0x0000FF00) >> 8 != i) {
					bus->bridge_secondary_bus = (busInfo & 0x0000FF00) >> 8;
				} else {
					bus->bridge_secondary_bus = 0xFFFF;
				}

				bus->ident.vendor = vendor; bus->ident.device = device;

				bus->d.node.name = "PCI Bus";
				bus->d.node.parent = &pci_bus.node;
				bus->d.node.children = list_allocate();

				pci_probe_bus(bus);

				// Insert bus as a child of the root PCI bus driver
				list_add(pci_bus.node.children, bus);
			} else if(class == 0x03) { // AGP busses have only a single device on them, the card !!!
				pci_bus_t *bus = (pci_bus_t *) kmalloc(sizeof(pci_bus_t));
				memclr(bus, sizeof(pci_bus_t));

				bus->bridge_secondary_bus = 0xFFFF;
				bus->bus_number = i;
				bus->ident.vendor = 0xFFFF;
				bus->ident.device = 0xDEAD;

				bus->d.node.name = "AGP Bus Bridge";
				bus->d.node.parent = &pci_bus.node;
				bus->d.node.children = list_allocate();

				pci_probe_bus(bus);

				list_add(pci_bus.node.children, bus);
			} else { // It isn't a bridge, but maybe this bus does have something on it
				uint32_t class = pci_config_read(i, 0, 0, 0x08) >> 0x18;
				kprintf("Found non-bridge device at bus %i (class = 0x%X, vendor = 0x%X, device = 0x%X)\n", i, temp, vendor, device);
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
	kprintf("==================== PCI Bus Device Listing ====================\n");

	// Check all possible busses
	for(int i = 0; i < pci_bus.node.children->num_entries; i++) {
		pci_bus_t *bus = (pci_bus_t *) list_get(pci_bus.node.children, i);

		// Is bus defined?
		if(bus) {
			kprintf("Bus %u:\n", bus->bus_number);

			// Search through all devices
			for(int d = 0; d < bus->d.node.children->num_entries; d++) {
				// Process multifunction device
				pci_device_t *device = (pci_device_t *) list_get(bus->d.node.children, d);

				if(device) {
					if(device->ident.vendor != 0xFFFF) {
						uint16_t vendor_id = device->ident.vendor;
						uint16_t device_id = device->ident.device;

						if(device->multifunction) {
							kprintf("  Device %u: Multifunction\n", d);

							for(int f = 0; f < 7; f++) {
								pci_function_t function = device->function[f];
								vendor_id = function.ident.vendor;
								device_id = function.ident.device;

								// This function is defined
								if(vendor_id != 0xFFFF) {
									pci_str_vendor_t* vinfo = pci_info_get_vendor(vendor_id);
									pci_str_device_t* dinfo = pci_info_get_device(vendor_id, device_id);

									if(vinfo && dinfo) {
										kprintf("    Function %u[%.4X:%.4X %.2X:%.2X]: %s %s\n", f, vendor_id, device_id, PCI_GET_CLASS(function.class), PCI_GET_SUBCLASS(function.class), vinfo->vendor_full, dinfo->chip_desc);
									} else {
										kprintf("    Function %u[%.4X:%.4X %.2X:%.2X]: Unknown\n", f, vendor_id, device_id, PCI_GET_CLASS(function.class), PCI_GET_SUBCLASS(function.class));
									}
								}
							}
						} else {
							pci_str_vendor_t* vinfo = pci_info_get_vendor(vendor_id);
							pci_str_device_t* dinfo = pci_info_get_device(vendor_id, device_id);
							kprintf("  Device %u[%.4X:%.4X]: %s %s\n", d, vendor_id, device_id, vinfo->vendor_full, dinfo->chip_desc);
						}
					}
				}
			}
		}
	}

	kprintf("\n");
}

/*
 * Checks if a driver will work for a given device. Returns true if the device
 * is in the list of supported device types or classes of the device. In this
 * instance, the device will actually be of type pci_device_t, which is an
 * extended version of device_t.
 */
bool pci_match(device_t* dev, driver_t* driver) {
	driver->supportQuery(dev);

	return false;
}

/*
 * Go through all PCI devices that have been enumerated and try to load drivers
 * for them.
 */
static int pci_load_drivers(void) {
	// Check all possible busses
	for(int i = 0; i < pci_bus.node.children->num_entries; i++) {
		pci_bus_t *bus = (pci_bus_t *) list_get(pci_bus.node.children, i);

		// Is bus defined?
		if(bus) {
			// Search through all devices
			for(int d = 0; d < bus->d.node.children->num_entries; d++) {
				// Process multifunction device
				pci_device_t *device = (pci_device_t *) list_get(bus->d.node.children, d);

				if(device) {
					if(device->ident.vendor != 0xFFFF) {
						uint16_t vendor_id = device->ident.vendor;
						uint16_t device_id = device->ident.device;
						
						driver_t *driver = bus_find_driver((device_t *) device, &pci_bus);
					}
				}
			}
		}
	}

	return 0;
}

module_post_driver_init(pci_load_drivers);

/*
 * Initialise PCI subsystem
 */
static int pci_init(void) {
	pci_fast_config_avail = false;

	bus_register(&pci_bus, BUS_NAME_PCI);

	pci_enumerate_busses();
	pci_print_tree();

	return 0;
}

/*
 * Clean up resources and stop devices, unload any drivers that were loaded
 * as a response to PCI devices
 */
static void pci_exit(void) {

}

module_bus_init(pci_init);
module_exit(pci_exit);