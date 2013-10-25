#ifndef ATA_PIO_H
#define ATA_PIO_H

#include <types.h>
#include "io/disk.h"

typedef uint8_t ATA_ERROR;

void ata_driver_init();

ATA_ERROR ata_reset(disk_t* disk);
ATA_ERROR ata_init(disk_t* disk);

ATA_ERROR ata_read(disk_t* disk, uint64_t lba, uint32_t num_sectors, void* destination);

ATA_ERROR ata_write(disk_t* disk, uint64_t lba, uint32_t num_sectors, void* source);
ATA_ERROR ata_flush_cache(disk_t* disk);

ATA_ERROR ata_erase(disk_t* disk, uint64_t lba, uint32_t num_sectors);

ATA_ERROR ata_get_status(disk_t* disk, int* destination);

#endif