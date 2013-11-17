#include <types.h>
#include "fat.h"
#include "vfs.h"

fs_type_t* fat_init_struct;

void* sector_buffer;

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

	// Register with kernel
	vfs_register(fat_init_struct);

	// Allocate other memory
	sector_buffer = (void *) kmalloc(FAT_SECTOR_BUFFER_SIZE);
}