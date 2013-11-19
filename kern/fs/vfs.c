#include <types.h>
#include "vfs.h"

static fs_type_t* vfs_find_fs(uint16_t type);

static fs_type_t* fs_map;
static fs_type_t* fs_last;

/*
 * Initialises the VFS subsystem
 */
void vfs_init() {

}

/*
 * Registers a filesystem with the kernel.
 */
int vfs_register(fs_type_t* fs) {
	fs->next = NULL;
	fs->prev = NULL;

	// Check if there isn't any filesystems loaded
	if(!fs_map) {
		fs_map = fs;
		fs_last = fs;
	} else {
		fs->prev = fs_last;

		fs_last->next = fs;
		fs_last = fs;
	}

	return 0;
}

/*
 * Unregisters a filesystem with the kernel. Note that this does not relinquish
 * memory allocated by the filesystem, nor the "type" structure.
 */
void vfs_deregister(fs_type_t* fs) {
	fs_type_t *fs_list_ptr = fs_map;
	ASSERT(fs_list_ptr != NULL);

	// Loop through all filesystems
	while(true) {
		if(fs_list_ptr == fs) {
			// Remove from linked list
			if(fs->prev) {
				fs_type_t* prev = fs->prev;
				prev->next = fs->next;
			}

			// Update pointer to last element in list
			if(fs == fs_last) {
				fs_last = fs_last->prev;
			}
		}

		fs_list_ptr = fs_list_ptr->next;
		if(!fs_list_ptr) break;
	}
}

/*
 * Mounts all filesystems in a certain partition map.
 */
void vfs_mount_all(ptable_t* pt) {
	ptable_entry_t* partInfo = pt->first;

	// Process each filesystem
	while(true) {
		vfs_mount_filesystem(partInfo);

		partInfo = partInfo->next;
		if(!partInfo) break;
	}
}

/*
 * Mounts a specific filesystem.
 */
int vfs_mount_filesystem(ptable_entry_t* fs) {
	fs_type_t* fdrv = vfs_find_fs(fs->type);

	if(fdrv) {
		kprintf("Mounting partition %i: Type 0x%X (%s), start 0x%X, length 0x%X sectors\n", fs->part_num, fs->type, fdrv->name, fs->lba_start, fs->lba_length);

		// allocate memory for the superblock
		fs_superblock_t *superblock = (fs_superblock_t*) kmalloc(sizeof(fs_superblock_t));
		memclr(superblock, sizeof(fs_superblock_t));

		ASSERT(superblock != NULL);

		fs_superblock_t *ret = fdrv->create_super(superblock, fs);
	} else {
		kprintf("Unknown filesystem type 0x%X\n", fs->type);
		return -1;
	}

	return 0;
}

/*
 * Locates the pointer to a filesystem to handle a certain type.
 */
static fs_type_t* vfs_find_fs(uint16_t type) {
	fs_type_t *fs_list_ptr = fs_map;
	ASSERT(fs_list_ptr != NULL);

	// Loop through all filesystems
	while(true) {
		if(fs_list_ptr->type == type) return fs_list_ptr;

		fs_list_ptr = fs_list_ptr->next;
		if(!fs_list_ptr) break;
	}

	return NULL;
}