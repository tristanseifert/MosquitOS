#include <types.h>
#include "device/ata_pio.h"

#include "disk.h"

/*
 * Allocates memory for a disk.
 */
disk_t* disk_allocate() {
	disk_t *disk = (disk_t *) kmalloc(sizeof(disk_t));
	disk->driver_calls = (void *) kmalloc(sizeof(driver_functions_t));

	kprintf("Allocated disk struct at 0x%X (driver calls at 0x%X)\n", disk, disk->driver_calls);

	return disk;
}

/*
 * Various wrappers that call the driver's functions for the disk.
 */
DISK_ERROR disk_reset(disk_t* disk) {
	driver_functions_t* functions = disk->driver_calls;
	return functions->disk_reset(disk);
}

DISK_ERROR disk_init(disk_t* disk) {
	driver_functions_t* functions = disk->driver_calls;
	return functions->disk_init(disk);
}

DISK_ERROR disk_read(disk_t* disk, uint64_t lba, uint32_t num_sectors, void* destination) {
	driver_functions_t* functions = disk->driver_calls;
	return functions->disk_read(disk, lba, num_sectors, destination);
}

DISK_ERROR disk_write(disk_t* disk, uint64_t lba, uint32_t num_sectors, void* source) {
	driver_functions_t* functions = disk->driver_calls;
	return functions->disk_write(disk, lba, num_sectors, source);
}

DISK_ERROR disk_flush_cache(disk_t* disk) {
	driver_functions_t* functions = disk->driver_calls;
	return functions->disk_flush_cache(disk);
}

DISK_ERROR disk_erase(disk_t* disk, uint64_t lba, uint32_t num_sectors) {
	driver_functions_t* functions = disk->driver_calls;
	return functions->disk_erase(disk, lba, num_sectors);
}