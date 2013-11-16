#ifndef ATA_PIO_H
#define ATA_PIO_H

#include <types.h>
#include "io/disk.h"

#define ATA_BUS_0_IOADDR 0x1F0
#define ATA_BUS_1_IOADDR 0x170

#define ATA_NO_ERR kDiskErrorNone
#define ATA_ERR_NO_DRIVE 0xFF
#define ATA_ERR_INIT_ERR 0xFE
#define ATA_ERR_ATAPI_DRIVE 0xFD
#define ATA_ERR_READ_ERR 0xFD
#define ATA_ERR_READ_TIMEOUT 0xFC

void ata_driver_init(disk_t* disk);

DISK_ERROR ata_reset(disk_t* disk);
DISK_ERROR ata_init(disk_t* disk);

DISK_ERROR ata_read(disk_t* disk, uint64_t lba, uint32_t num_sectors, void* destination);

DISK_ERROR ata_write(disk_t* disk, uint64_t lba, uint32_t num_sectors, void* source);
DISK_ERROR ata_flush_cache(disk_t* disk);

DISK_ERROR ata_erase(disk_t* disk, uint64_t lba, uint32_t num_sectors);

DISK_ERROR ata_get_status(disk_t* disk, int* destination);

#endif