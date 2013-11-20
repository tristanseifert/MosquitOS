#ifndef VFS_H
#define VFS_H

#include <types.h>
#include <io/disk.h>
#include "ptable.h"

#define VFS_NUM_ARRAY 8

typedef struct {
	const char *vol_label;
	const char *vol_mount_point;

	disk_t *disk;
	ptable_entry_t *pt;

	// pointer to fs-specific struct
	void* fs_info;
} fs_superblock_t;

typedef struct fs_type {
	const char *name;
	uint32_t flags;
	uint32_t handle; // set by kernel, should be 0
	uint32_t type; // parition table ID

	void *owner;

	fs_superblock_t* (*create_super) (fs_superblock_t*, ptable_entry_t*);

	// kernel stores the VFS table as doubly-linked list
	struct fs_type *next;
	struct fs_type *prev;
} fs_type_t;

void vfs_init();

int vfs_register(fs_type_t* fs);
void vfs_deregister(fs_type_t* fs);

void vfs_mount_all(ptable_t* pt);
int vfs_mount_filesystem(ptable_entry_t* fs);

#endif