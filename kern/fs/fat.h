#ifndef FS_FAT_H
#define FS_FAT_H

#include <types.h>
#include "io/disk.h"

#define FAT_BUF_SECTORS 8
#define FAT_SECTOR_BUFFER_SIZE 512*FAT_BUF_SECTORS

void fat_init();

#endif