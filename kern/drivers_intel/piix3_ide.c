#include <types.h>

#include "piix3_ide.h"

#include "device/ata_pio.h"
#include "bus/bus.h"
#include "bus/pci.h"

#define PIIX3_BUS_MASTER_IO 0xD000

/*
 * Primary Command Task File Base: BAR0
 * Primary Control Task File Base: BAR1
 * Secondary Command Task File Base: BAR2
 * Secondary Control Task File Base: BAR3
 * Bus Master IDE Base: BAR4
 * Reserved: BAR5
 */
enum {
	PIIX3_PCMDB = 0, // 10 primary command task file base
	PIIX3_PCTRLB = 1, // 14 primary control task file base
	PIIX3_SCMDB = 2, // 18 secondary command task file base
	PIIX3_SCTRLB = 3, // 1c secondary control task file base
	PIIX3_BMB = 4 // 20 Bus Master IDE base
	PIIX3_EBM = 5 // 24 Expansion Base Memory Address
};

// Private functions
static bool drv_piix3_ide_claim(pci_device_t *device);
static bool drv_piix3_ide_configure(uint16_t vendor, uint16_t device);

// Driver
static driver_t drv = {
	.name = "PIIX 3 IDE Driver",
	.supportQuery = (bool (*)(device_t *)) drv_piix3_ide_claim
};

// Internal state
static bool piix3_ide_initialised = false;

static uint16_t pci_bus, pci_device;

static pci_function_t *functionPtr;
static pci_device_t *piix3_device;

static void* piix3_expansion_memory;

/*
 * Initialises the PIIX 3 IDE controller driver.
 */
static int drv_piix3_ide_init(void) {
	// Register driver
	int err = bus_register_driver(&drv, BUS_NAME_PCI);
	
	if(err != 0) {
		kprintf("piix3_ide: Error registering driver (%i)", err);
	}

	return 0;
}

module_driver_init(drv_piix3_ide_init);

/*
 * Called by the PCI driver to query the driver if it will claim a certain PCI
 * device.
 */
static bool drv_piix3_ide_claim(pci_device_t *device) {
	uint16_t vendor_id, device_id;

	// The PIIX3 is a multifunction device
	if(device->multifunction) {
		// IDE is always function 1 of the chipset
		pci_function_t function = device->function[1];
		vendor_id = function.ident.vendor;
		device_id = function.ident.device;

		// This function is defined
		if(vendor_id != 0xFFFF) {
			if(vendor_id == PIIX3_IDE_VEN && device_id == PIIX3_IDE_DEV) {
				// Prevent doubly initialising driver
				ASSERT(!piix3_ide_initialised);
				piix3_ide_initialised = true;

				// Configure PIIX3
				piix3_device = device;
				functionPtr = &device->function[1];
				drv_piix3_ide_configure(device->location.bus, device->location.device);

				kprintf("piix3_ide: found on PCI bus at bus %u, device %u, function 1\n", device->location.bus, device->location.device);
				return true;
			}
		}
	}

	return false;
}

/*
 * Attempts to configure the PIIX3's IDE controller with the default settings.
 * Calling this function will also set in progress an initial drive scan, which
 * allows later drive functions to use cached data.
 */
static bool drv_piix3_ide_configure(uint16_t bus, uint16_t device) {
	pci_bus = bus;
	pci_device = device;

/*	// Command task file for primary is at 0x1F0
	pci_device_update_bar(piix3_device, 1, PIIX3_PCMDB, (ATA_BUS_0_IOADDR | 0x0001));
	// Control task file for primary is at 0x1F8
	pci_device_update_bar(piix3_device, 1, PIIX3_PCTRLB, (ATA_BUS_0_CTRLADDR | 0x0001));

	// Command task file for secondary is at 0x170
	pci_device_update_bar(piix3_device, 1, PIIX3_SCMDB, (ATA_BUS_1_IOADDR | 0x0001));
	// Control task file for secondary is at 0x178
	pci_device_update_bar(piix3_device, 1, PIIX3_SCTRLB, (ATA_BUS_1_CTRLADDR | 0x0001));*/

	// Allocate 4K for extended memory
	uint32_t expansion_memory_phys;
	piix3_expansion_memory = (void *) kmalloc_ap(4096, &expansion_memory_phys);
	kprintf("piix3_ide: Expansion memory at 0x%X (0x%X phys)", piix3_expansion_memory, expansion_memory_phys);

	// Set expansion memory address
	pci_device_update_bar(piix3_device, 1, PIIX3_EBM, expansion_memory_phys);

	// Set bus master IDE Base IO Address
	pci_device_update_bar(piix3_device, 1, PIIX3_BMB, PIIX3_BUS_MASTER_IO | 0x00000001);

	// Enable IO space addressing
	pci_config_write_w(pci_config_address(bus, device, 1, 0x04), 0x0005);

	// Determine size required by BAR0
	for(int i = 0; i < 6; i++) {
		pci_bar_t bar = functionPtr->bar[i];
		kprintf("piix3_ide: BAR%u size 0x%X, address 0x%X, flags 0x%X\n", i, bar.end - bar.start, bar.start, bar.flags);
	}

	// Get busmaster IO address
	busmaster_io_addr = functionPtr->bar[4].start;

	/*
	 * Configure Master and Slave IDE:
	 *
	 * Decode IDE PIO accesses (b15)
	 * Program individual slave data (b14)
	 * IORDY sample point after 3 clocks (b13:12)
	 * Recovery Time of 3 clocks (IORDY -> DIOx# strobe) (b9:8)
	 * DMA Timing Enable Only Drive 1 off (b7)
	 * Prefetch and Posting Drive 1 Enable off (b6)
	 * Enable IORDY sampling for drive 1 (b5)
	 * Fast Data Bank Drive Select 1 Enable (b4)
	 * DMA Timing Enable Only Drive 0 off (b3)
	 * Prefetch and Posting Drive 0 Enable off (b2)
	 * Enable IORDY sampling for drive 0 (b1)
	 * Fast Data Bank Drive Select 0 Enable (b0)
	 */
	uint32_t cfg = 0b11100001001100111110000100110011;
	pci_config_write_l(pci_config_address(bus, device, 1, 0x40), cfg);

	// Configure slave IORDY sample point 3 clocks, recovery time 3 clocks
	pci_config_write_l(pci_config_address(bus, device, 1, 0x44), 0b10011001);

	return true;
}