#ifndef ATA_PIO_H
#define ATA_PIO_H

#include <types.h>
#include "io/disk.h"

#define ATA_BUS_0_IOADDR 0x1F0
#define ATA_BUS_0_CTRLPORT 0x3F6
#define ATA_BUS_1_IOADDR 0x170
#define ATA_BUS_1_CTRLPORT 0x376

#define ATA_BUS_MASTER_IO 0xD000

// Channels:
#define ATA_PRIMARY				0x00
#define ATA_SECONDARY			0x01
 
// Directions:
#define ATA_READ				0x00
#define ATA_WRITE				0x01

// Device types
#define ATA_DEVICE_TYPE_ATA		0x00
#define ATA_DEVICE_TYPE_ATAPI	0x01
 
#define ATA_MASTER				0x00
#define ATA_SLAVE				0x01

/*
 * This structure can be used to interact with the driver.
 */
typedef struct ata_info ata_info_t;
typedef struct ata_driver ata_driver_t;
typedef struct ata_device ata_device_t;
typedef struct ata_channel ata_channel_t;

// Struct defining an ATA device
struct ata_device {
	bool drive_exists; // Set to true if a device actually exists
	uint8_t channel; // 0 (Primary Channel) or 1 (Secondary Channel).
	uint8_t drive; // 0 (Master Drive) or 1 (Slave Drive).
	uint16_t type; // 0: ATA, 1:ATAPI.
	uint16_t signature;	// Drive Signature
	uint16_t capabilities; // Features.
	uint32_t commandSets; // Command Sets Supported.
	uint64_t size; // Size in Sectors.
	char model[42]; // Model name string

	ata_info_t *ata_info; // 4KB
};

// Struct defining an ATA channel
struct ata_channel {
	uint16_t base;  // I/O Base.
	uint16_t ctrl;  // Control Base
	uint16_t bmide; // Bus Master IDE
	bool nIEN;  // nIEN (No Interrupt);
};

// Struct to hold ATA info read from the device
struct ata_info {
	uint8_t ata_identify[512];

	uint8_t reserved[3584];
};

// Struct to interact with the driver
struct ata_driver {
	uint32_t BAR0, BAR1, BAR2, BAR3, BAR4, BAR5;

	// Channels
	ata_channel_t channels[2];

	// Devices
	ata_device_t devices[4];
};

ata_driver_t* ata_init_pci(uint32_t BAR0, uint32_t BAR1, uint32_t BAR2, uint32_t BAR3, uint32_t BAR4);

#endif