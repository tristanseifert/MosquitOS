#ifndef FS_FAT_H
#define FS_FAT_H

#include <types.h>
#include "io/disk.h"

#define FAT_BUF_SECTORS 8
#define FAT_SECTOR_BUFFER_SIZE 512*FAT_BUF_SECTORS

// We can cast the sector buffer to one of these
typedef struct fat_extBS_32 {
	uint8_t bootjmp[3];
	char oem_name[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sector_count;
	uint8_t table_count;
	uint16_t root_entry_count;
	uint16_t total_sectors_16;
	uint8_t media_type;
	uint16_t table_size_16;
	uint16_t sectors_per_track;
	uint16_t head_side_count;
	uint32_t hidden_sector_count;
	uint32_t total_sectors_32;

	uint32_t table_size_32;
	uint16_t extended_flags;
	uint16_t fat_version;
	uint32_t root_cluster;
	uint16_t fat_info;
	uint16_t backup_BS_sector;
	uint8_t reserved_0[12];
	uint8_t drive_number;
	uint8_t reserved_1;
	uint8_t boot_signature;
	uint32_t volume_id;
	uint8_t volume_label[11];
	uint8_t fat_type_label[8];
 
} __attribute__((packed)) fat_fs_bpb32_t;

typedef struct fat_extBS_16 {
	uint8_t bootjmp[3];
	char oem_name[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sector_count;
	uint8_t table_count;
	uint16_t root_entry_count;
	uint16_t total_sectors_16;
	uint8_t media_type;
	uint16_t table_size_16;
	uint16_t sectors_per_track;
	uint16_t head_side_count;
	uint32_t hidden_sector_count;
	uint32_t total_sectors_32;

	uint8_t bios_drive_num;
	uint8_t reserved1;
	uint8_t boot_signature;
	uint32_t volume_id;
	uint8_t volume_label[11];
	uint8_t fat_type_label[8];
 
} __attribute__((packed)) fat_fs_bpb16_t;

typedef struct {
	uint8_t bootjmp[3];
	char oem_name[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sector_count;
	uint8_t table_count;
	uint16_t root_entry_count;
	uint16_t total_sectors_16;
	uint8_t media_type;
	uint16_t table_size_16;
	uint16_t sectors_per_track;
	uint16_t head_side_count;
	uint32_t hidden_sector_count;
	uint32_t total_sectors_32;
} __attribute__((packed)) fat_fs_bpb_t;

typedef struct {
	fat_fs_bpb_t *bpb;

	// either 16 or 32
	uint8_t fat_type;
} fat_fs_info_t;

void fat_init();

#endif