#ifndef DISK_H
#define DISK_H

#include <types.h>

typedef enum {
	kDiskInterfaceNone = 0,
	kDiskInterfaceATA // ATA PIO
} disk_interface;

typedef struct disk_info_struct {
	uint8_t disk_id;
	uint8_t bus;
	bool masterAndServant;

	// Tells the disk driver what physical device driver to use
	disk_interface connection;

	// Pointer to driver's data structure for the drive
	void* driver_specific_data;
} disk_t;

typedef enum {
	kDiskErrorNone = 0,
	kDiskErrorIO,
	kDiskErrorController,
	kDiskErrorATA,
	kDiskErrorUnknown
} DISK_ERROR;

DISK_ERROR disk_reset(disk_t* disk);
DISK_ERROR disk_init(disk_t* disk);

DISK_ERROR disk_read(disk_t* disk, uint64_t lba, uint32_t num_sectors, void* destination);

DISK_ERROR disk_write(disk_t* disk, uint64_t lba, uint32_t num_sectors, void* source);
DISK_ERROR disk_flush_cache(disk_t* disk);

DISK_ERROR disk_erase(disk_t* disk, uint64_t lba, uint32_t num_sectors);

DISK_ERROR disk_eject(disk_t* disk, bool eject);
DISK_ERROR disk_idle(disk_t* disk, uint8_t timeout);
DISK_ERROR disk_standby(disk_t* disk);
DISK_ERROR disk_wake(disk_t* disk);

DISK_ERROR disk_get_status(disk_t* disk, int* destination);
DISK_ERROR disk_get_health(disk_t* disk, void* healthBuffer);

#endif