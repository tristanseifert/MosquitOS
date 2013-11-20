#ifndef FS_FAT_H
#define FS_FAT_H

#include <types.h>
#include "io/disk.h"
#include "vfs.h"

#define FAT_BUF_SECTORS 8
#define FAT_SECTOR_BUFFER_SIZE 512*FAT_BUF_SECTORS

#define FAT32_MASK 0x0FFFFFFF
#define FAT32_BAD_CLUSTER 0x0FFFFFF7
#define FAT32_END_CHAIN 0x0FFFFFF0
#define FAT16_BAD_CLUSTER 0xFFF7
#define FAT16_END_CHAIN 0xFFF8

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
	uint8_t  q[11];
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
	fat_fs_bpb_t* bpb;

	// either 16 or 32
	uint8_t fat_type;

	uint32_t root_dir_sectors;
	uint32_t root_cluster;
	uint32_t first_data_sector;
	uint32_t first_fat_sector;
	uint32_t data_sectors;
	uint32_t total_clusters;

	// buffers
	void* root_directory;
	uint32_t root_dir_length;
} fat_fs_info_t;

void fat_init();

void fat_read_get_root_dir(fs_superblock_t* superblock, void* buffer, uint32_t buffer_size);

#endif