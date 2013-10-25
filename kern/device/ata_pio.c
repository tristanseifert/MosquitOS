#include <types.h>
#include "ata_pio.h"

/*
 * Private data structures used internally by the driver for each disk.
 */
typedef struct ata_driver_command {
	struct ata_driver_command* next;
} ata_command_t;

typedef struct ata_driver_info_struct {
	// Pointer to an ata_command_t struct
	ata_command_t* commandQueue;
} ata_info_t;

/*
 * Initialises the ATA driver.
 */
void ata_driver_init() {

}

/*
 * Sends a SOFT RESET command to the drive.
 */
ATA_ERROR ata_reset(disk_t* disk) {
	return -1;
}

/*
 * Initialises the drive.
 */
ATA_ERROR ata_init(disk_t* disk) {
	if(!disk->driver_specific_data) {
		disk->driver_specific_data = malloc(sizeof(ata_info_t));

		ata_info_t* info = disk->driver_specific_data;
		memclr(info, sizeof(ata_info_t));

		info->commandQueue = 0x0;
	}
	
	return -1;
}

/*
 * Reads num_sectors starting at lba from the disk to destination.
 */
ATA_ERROR ata_read(disk_t* disk, uint64_t lba, uint32_t num_sectors, void* destination) {
	return -1;
}

/*
 * Writes num_sectors to lba on the disk, reading from the memory at source.
 */
ATA_ERROR ata_write(disk_t* disk, uint64_t lba, uint32_t num_sectors, void* source) {
	return -1;
}

/*
 * Flushes the disk's cache
 */
ATA_ERROR ata_flush_cache(disk_t* disk) {
	return -1;
}


/*
 * Erases num_sectors, starting at sector lba.
 */
ATA_ERROR ata_erase(disk_t* disk, uint64_t lba, uint32_t num_sectors) {
	return -1;
}

/*
 * 
 */
ATA_ERROR ata_get_status(disk_t* disk, int* destination) {
	return -1;
}