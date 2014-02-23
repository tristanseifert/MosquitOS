#include <types.h>

#include "piix3_ide.h"

#include "device/ata.h"
#include "sys/irq.h"
#include "bus/bus.h"
#include "bus/pci.h"

#define PIIX3_BUS_MASTER_IO ATA_BUS_MASTER_IO

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
	PIIX3_BMB = 4, // 20 Bus Master IDE base
	PIIX3_EBM = 5 // 24 Expansion Base Memory Address
};

// Private functions
static bool drv_piix3_ide_claim(pci_device_t *device);
static bool drv_piix3_ide_configure(uint16_t vendor, uint16_t device);
static bool drv_piix3_ide_using_80conductor(uint8_t channel, uint8_t device);
static void drv_piix3_ide_irq(void *context);

// Driver info
static driver_t drv = {
	.name = "PIIX 3 IDE Driver",
	.supportQuery = (bool (*)(device_t *)) drv_piix3_ide_claim
};

// Internal state
static bool piix3_ide_initialised = false;

// Location of PIIX3
static uint16_t pci_bus, pci_device;

// PCI device associated with PIIX3
static pci_device_t *piix3_device;
// IDE function of PIIX3 PCI device
static pci_function_t *functionPtr;

// Expansion memory
static void* piix3_expansion_memory;

// ATA driver associated with this hardware
static ata_driver_t *ata;

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
 * Cleans up resources associated with the IDE controller, and removes it from
 * the PCI bus.
 */
static void drv_piix3_ide_exit(void) {
	// Remove from PCI bus
	pci_config_write_w(pci_config_address(pci_bus, pci_device, 1, 0x06), 0);

	piix3_ide_initialised = false;
}
module_exit(drv_piix3_ide_exit);

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

				// Save location of device
				piix3_device = device;
				functionPtr = &device->function[1];
				pci_bus = device->location.bus;
				pci_device = device->location.device;

				kprintf("piix3_ide: found on PCI bus at bus %u, device %u, function 1\n", device->location.bus, device->location.device);

				// Set up the chipset
				drv_piix3_ide_configure(pci_bus, pci_device);

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
	// Enable native PCI mode
	pci_config_write_b(pci_config_address(bus, device, 1, 0x09), 0x85);

	// Program the command task file locations to legacy values
	pci_device_update_bar(piix3_device, 1, PIIX3_PCMDB, 0);
	pci_device_update_bar(piix3_device, 1, PIIX3_SCMDB, 0);

	// Program the control task file locations to legacy values
	pci_device_update_bar(piix3_device, 1, PIIX3_PCTRLB, 0);
	pci_device_update_bar(piix3_device, 1, PIIX3_SCTRLB, 0);

	// Allocate 4K for extended memory
	uint32_t expansion_memory_phys;
	piix3_expansion_memory = (void *) kmalloc_ap(4096, &expansion_memory_phys);

//	kprintf("piix3_ide: Expansion memory at 0x%X (0x%X phys)\n", piix3_expansion_memory, expansion_memory_phys);

	// Set expansion memory address
	pci_device_update_bar(piix3_device, 1, PIIX3_EBM, expansion_memory_phys);

	// Set bus master IDE Base IO Address
	pci_device_update_bar(piix3_device, 1, PIIX3_BMB, 0xD000 | 0x00000001);

	// Enable I/O Space and Memory Space Enable, enable bus master
	pci_config_write_w(pci_config_address(bus, device, 1, 0x04), 0x0007);

	/*
	 * The PIIX3 IDE controller will use IRQ 14 and 15, regardless of how it's
	 * configured, as it is a parallel controller.
	 */
	irq_register(14, drv_piix3_ide_irq, NULL);

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
	uint16_t cfg = 0b1110000100110011;
	pci_config_write_w(pci_config_address(bus, device, 1, 0x40), cfg); // primary
	pci_config_write_w(pci_config_address(bus, device, 1, 0x42), cfg); // secondary

	// Configure slave IORDY sample point 3 clocks, recovery time 3 clocks
	pci_config_write_b(pci_config_address(bus, device, 1, 0x44), 0b10011001);

	// Set WR_PingPong_EN bit in IDE I/O Config reg (speed increase for PIO)
	pci_config_write_w(pci_config_address(bus, device, 1, 0x54), (1 << 10));	

	// Set up ATA driver (this will probe for devices)
	ata = ata_init_pci(functionPtr->bar[0].start, functionPtr->bar[1].start, functionPtr->bar[2].start, functionPtr->bar[3].start, functionPtr->bar[4].start);

	/*
	 * The ATA spec states that for UDMA 3 and higher, we require a 80-conductor
	 * cable for reliable operation. If the device is not connected using one,
	 * but supports UDMA 3 or better operation, it will use UDMA 2.
	 */
	for (int i = 0; i < 4; i++) {
		if (ata->devices[i].drive_exists == true) {
			if(ata->devices[i].udma_supported >= kATA_UDMA3) {
				if(drv_piix3_ide_using_80conductor(ata->devices[i].channel, ata->devices[i].drive)) {
					ata->devices[i].udma_preferred = ata->devices[i].udma_supported;
					kprintf("piix3_ide: device %u using UDMA %u\n", i, ata->devices[i].udma_preferred);
				} else {
					ata->devices[i].udma_preferred = kATA_UDMA2;	
					kprintf("piix3_ide: device %u capped at UDMA 2\n", i);				
				}
			}

			// Set up DMA modes

		}
	}

	return true;
}

/*
 * Checks if a device is connected using an 80-conductor cable
 */
static bool drv_piix3_ide_using_80conductor(uint8_t channel, uint8_t device) {
	uint16_t reg = pci_config_read_w(pci_config_address(pci_bus, pci_device, 1, 0x54));
	uint8_t offset = 4 + (device & 0x01);
	offset += (channel << 1);

	// kprintf("piix3_ide: Cable register 0x%X 0x%X\n", reg, pci_config_read_l(pci_config_address(pci_bus, pci_device, 1, 0x54)));

	// If the bit is set, there's an 80 conductor cable
	if(reg & (1 << offset)) {
		return true;
	} else {
		return false;
	}
}

/*
 * IRQ handler
 */
static void drv_piix3_ide_irq(void *context) {
	kprintf("ATA IRQ!\n");
}