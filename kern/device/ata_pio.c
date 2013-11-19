#include <types.h>
#include "io/io.h"
#include "ata_pio.h"

static void ata_do_pio_read(void* dest, uint32_t num_words, uint8_t bus);
static char* ata_convert_string(void* in, size_t length);

// Maps a bus to IO addresses
static uint16_t ata_bus_to_ioaddr[4] = {ATA_BUS_0_IOADDR, ATA_BUS_1_IOADDR, 0x0000, 0x0000};

/*
 * Private data structures used internally by the driver for each disk.
 */
typedef struct ata_driver_command {
	struct ata_driver_command* next;
} ata_command_t;

typedef struct ata_driver_info_struct {
	// Pointer to an ata_command_t struct
	ata_command_t* commandQueue;

	// memory ptr to drive's IDENTIFY response
	void *ata_identify_info;
} ata_info_t;

/*
 * Initialises the ATA driver and places the functions in the struct
 */
void ata_driver_init(disk_t* disk) {
	driver_functions_t* functionStruct = (driver_functions_t*) disk->driver_calls;

	functionStruct->disk_reset = ata_reset;
	functionStruct->disk_init = ata_init;
	functionStruct->disk_read = ata_read;
	functionStruct->disk_write = ata_write;
	functionStruct->disk_flush_cache = ata_flush_cache;
	functionStruct->disk_erase = ata_erase;

	disk->connection = kDiskInterfaceATA;
}

/*
 * Sends a SOFT RESET command to the drive.
 */
DISK_ERROR ata_reset(disk_t* disk) {
	return -1;
}

/*
 * Initialises the drive.
 */
DISK_ERROR ata_init(disk_t* disk) {
	// If an ATA disk structure hasn't been allocated, create one
	if(!disk->driver_specific_data) {
		disk->driver_specific_data = (void *) kmalloc(sizeof(ata_info_t));

		ata_info_t* info = disk->driver_specific_data;
		memclr(info, sizeof(ata_info_t));

		info->commandQueue = 0x0;

		// Figure out what bus the drive is on
		uint8_t bus = disk->bus;
		ASSERT(bus < 3);

		// Drive ID on ATA is 1 bit
		disk->drive_id &= 0x01;
	}

	ata_info_t* info = disk->driver_specific_data;
	uint8_t bus = disk->bus;

	// Check if there is any drives on this bus (0xFF == floating)
	if(io_inb(ata_bus_to_ioaddr[bus] + 7) == 0xFF) {
		return ATA_ERR_NO_DRIVE;
	}

	// Select correct drive
	io_outb(ata_bus_to_ioaddr[bus] + 6, (disk->drive_id == 0x01) ? 0xB0 : 0xA0);

	// Clear LBA address ports
	for(uint8_t i = 2; i < 5; i++) {
		io_outb(ata_bus_to_ioaddr[bus] + i, 0x00);
	}

	// Send IDENTIFY command
	io_outb(ata_bus_to_ioaddr[bus] + 7, 0xEC);

	// If status register is 0x00, the drive does not exist
	if(!io_inb(ata_bus_to_ioaddr[bus] + 7)) {
		return ATA_ERR_NO_DRIVE;
	}

	// Wait for BSY bit to become clear
	while((io_inb(ata_bus_to_ioaddr[bus] + 7) & 0x80));

	// If it's an ATAPI drive, LBAhi and LBAmid are zero
	if(io_inb(ata_bus_to_ioaddr[bus] + 4) || io_inb(ata_bus_to_ioaddr[bus] + 5)) {
		kprintf("Found ATAPI drive at (%i, %i), cannot initialise\n", bus, disk->drive_id);
		return ATA_ERR_ATAPI_DRIVE;
	}

	// Wait for DRQ or ERR to set
	uint8_t ata_status = io_inb(ata_bus_to_ioaddr[bus] + 7);

	while(true) {
		ata_status = io_inb(ata_bus_to_ioaddr[bus] + 7);

		// Is DRQ set?
		if(ata_status & 0x08) {
			break;
		} else if(ata_status & 0x01) { // ERR is set
			return ATA_ERR_INIT_ERR;
		}
	}

	// Allocate memory for IDENTIFY response and read
	info->ata_identify_info = (void *) kmalloc(0x200);
	ata_do_pio_read(info->ata_identify_info, 0x100, bus);

	kprintf("HDD Model: %s\n", info->ata_identify_info+(27*2));

	return ATA_NO_ERR;
}

/*
 * Reads num_sectors starting at lba from the disk to destination.
 */
DISK_ERROR ata_read(disk_t* disk, uint64_t lba, uint32_t num_sectors, void* destination) {
	kprintf("Reading %i sectors starting at LBA %i", num_sectors, lba);
	ata_info_t* info = disk->driver_specific_data;
	uint8_t bus = disk->bus;

	// Wait for device to stop being busy
	while((io_inb(ata_bus_to_ioaddr[bus] + 7) & 0x80));

	// Select correct drive
	io_outb(ata_bus_to_ioaddr[bus] + 6, (disk->drive_id == 0x01) ? 0x50 : 0x40);

	// Set magic bits
	io_outb(ata_bus_to_ioaddr[bus] + 6, 0x40);

	// Write sector and LBA high
	io_outb(ata_bus_to_ioaddr[bus] + 2, (num_sectors >> 8));
	io_outb(ata_bus_to_ioaddr[bus] + 3, (lba >> 0x18)); // LBA4
	io_outb(ata_bus_to_ioaddr[bus] + 4, (lba >> 0x20)); // LBA5
	io_outb(ata_bus_to_ioaddr[bus] + 5, (lba >> 0x28)); // LBA6

	// Write sector and LBA low
	io_outb(ata_bus_to_ioaddr[bus] + 2, (num_sectors & 0xFF));
	io_outb(ata_bus_to_ioaddr[bus] + 3, (lba & 0xFF)); // LBA1
	io_outb(ata_bus_to_ioaddr[bus] + 4, (lba >> 0x08)); // LBA2
	io_outb(ata_bus_to_ioaddr[bus] + 5, (lba >> 0x10)); // LBA3

	// Send READ EXTENDED command
	io_outb(ata_bus_to_ioaddr[bus] + 7, 0x24);

	// Wait for BSY bit to become clear
	while((io_inb(ata_bus_to_ioaddr[bus] + 7) & 0x80));

	// Wait for DRQ or ERR to set
	uint8_t ata_status = io_inb(ata_bus_to_ioaddr[bus] + 7);

	while(true) {
		ata_status = io_inb(ata_bus_to_ioaddr[bus] + 7);

		// Is DRQ set?
		if(ata_status & 0x08) {
			break;
		} else if(ata_status & 0x01) { // ERR is set
			return ATA_ERR_READ_ERR;
		}
	}

	uint16_t *outPtr = (uint16_t *) destination;

	// Read 256-word blocks from the drive
	for(int i = 0; i < num_sectors; i++) {
		// Wait for BSY
		while((io_inb(ata_bus_to_ioaddr[bus] + 7) & 0x80));

		// Wait for DRQ or ERR to set
		uint8_t ata_status = io_inb(ata_bus_to_ioaddr[bus] + 7);

		while(true) {
			ata_status = io_inb(ata_bus_to_ioaddr[bus] + 7);

			// Is DRQ set?
			if(ata_status & 0x08) {
				break; // drive is ready
			} else if(ata_status & 0x21) { // ERR or DF is set
				return ATA_ERR_READ_ERR;
			}
		}

		ata_do_pio_read(outPtr, 0x100, bus);
		outPtr += 0x100;

		kprintf(".");
	}

	kprintf(" Done.\n");

	return ATA_NO_ERR;
}

/*
 * Writes num_sectors to lba on the disk, reading from the memory at source.
 */
DISK_ERROR ata_write(disk_t* disk, uint64_t lba, uint32_t num_sectors, void* source) {
	ata_flush_cache(disk);

	return ATA_NO_ERR;
}

/*
 * Flushes the disk's cache
 */
DISK_ERROR ata_flush_cache(disk_t* disk) {
	ata_info_t* info = disk->driver_specific_data;
	uint8_t bus = disk->bus;

	// Wait for device to stop being busy
	while((io_inb(ata_bus_to_ioaddr[bus] + 7) & 0x80));

	// Send FLUSH CACHES command
	io_outb(ata_bus_to_ioaddr[bus] + 7, 0xE7);

	return ATA_NO_ERR;
}


/*
 * Erases num_sectors, starting at sector lba.
 */
DISK_ERROR ata_erase(disk_t* disk, uint64_t lba, uint32_t num_sectors) {
	return ATA_NO_ERR;
}

/*
 * Writes a status word to the destination.
 */
DISK_ERROR ata_get_status(disk_t* disk, int* destination) {
	return ATA_NO_ERR;
}

/*
 * Performs a PIO read.
 */
static void ata_do_pio_read(void* dest, uint32_t num_words, uint8_t bus) {
	uint16_t *dest_ptr = (uint16_t *) dest;

	// Read from PIO data port
	static uint16_t temp;
	for(int i = 0; i < num_words; i++) {
		temp = io_inw(ata_bus_to_ioaddr[bus]);
		//*dest_ptr++ = ((temp & 0xFF00) >> 0x08) | ((temp & 0x00FF) << 0x08);
		*dest_ptr++ = temp;
	}
}

/*
 * Performs the required byteswapping and endianness-changes on an ASCII string returned
 * by a drive, as per the ATA specs, then converts it to a null-terminated C string.
 */
static char* ata_convert_string(void* in, size_t length) {
	size_t str_len = length+2;
	char *out = (char *) kmalloc(str_len);

	uint16_t* str_ptr = (uint16_t *) in;
	uint16_t* str_out_ptr = (uint16_t *) out;

	uint16_t temp;

	// Swap words
	for(int i = 0; i < length/2; i++) {
		temp = str_ptr[i];
		str_out_ptr[i] = ((temp & 0xFF00) >> 0x08) | ((temp & 0x00FF) << 0x08);
	}

	out[str_len-1] = 0x00;

	return out;
}