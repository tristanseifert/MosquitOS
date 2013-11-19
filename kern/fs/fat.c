#include <types.h>
#include "fat.h"
#include "vfs.h"

static fs_superblock_t* fat_make_superblock(fs_superblock_t* superblock, ptable_entry_t* pt);

static fs_type_t* fat_init_struct;
static void* sector_buffer;

/*
 * Initialises the FAT filesystem driver.
 */
void fat_init() {
	// Allocate memory for info struct
	fat_init_struct = (fs_type_t *) kmalloc(sizeof(fs_type_t));
	memclr(fat_init_struct, sizeof(fs_type_t));

	// Set type and name
	fat_init_struct->name = "FAT32 with LBA Addressing";
	fat_init_struct->type = 0x000C;
	fat_init_struct->create_super = fat_make_superblock;

	// Register with kernel
	vfs_register(fat_init_struct);

	// Allocate other memory
	sector_buffer = (void *) kmalloc(FAT_SECTOR_BUFFER_SIZE);
}

/*
 * Reads from the filesystem to create a superblock.
 */
static fs_superblock_t* fat_make_superblock(fs_superblock_t* superblock, ptable_entry_t* pt) {
	ASSERT(superblock != NULL);
	ptable_t *ptable = (ptable_t *) pt->ptable;

	kprintf("\nInitialising FAT superblock...\n");

	// Clear memory
	memclr(sector_buffer, FAT_SECTOR_BUFFER_SIZE);

	// Read first sector of FAT volume
	DISK_ERROR derr;
	derr = disk_read(ptable->disk, PARTITION_LBA_REL2ABS(0, pt), 1, sector_buffer);

	// Handle disk read errors
	if(derr != kDiskErrorNone) {
		return NULL;
	}

	// Allocate memory for FS info struct
	fat_fs_info_t *fs_info = (fat_fs_info_t *) kmalloc(sizeof(fat_fs_info_t));
	memclr(fs_info, sizeof(fat_fs_info_t));
	superblock->fs_info = fs_info;

	// Copy BPB (512 bytes)
	fs_info->bpb = (fat_fs_bpb_t *) kmalloc(512);
	memcpy(fs_info->bpb, sector_buffer, 512);

	// Determine FAT type
	if(fs_info->bpb->total_sectors_16 < 4085 && fs_info->bpb->total_sectors_16)  {
		fs_info->fat_type = 12;
	} else if(fs_info->bpb->total_sectors_16 < 65525 && fs_info->bpb->total_sectors_16) {
		fs_info->fat_type = 16;
	} else {
		fs_info->fat_type = 32;
	}

	// Do FAT-specific stuff
	if(fs_info->fat_type == 32) {
		fat_fs_bpb32_t *bpb = (fat_fs_bpb32_t *) fs_info->bpb;

		kprintf("Volume label: %s\n", &bpb->volume_label);
	} else if(fs_info->fat_type == 16) {
		fat_fs_bpb16_t *bpb = (fat_fs_bpb16_t *) fs_info->bpb;
	}

	return superblock;
}