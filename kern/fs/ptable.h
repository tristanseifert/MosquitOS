/*
 * This header defines types to work with partition tables and constants for
 * use in identifying filesystems.
 */

#ifndef PTABLE_H
#define PTABLE_H

#include <types.h>
#include <io/disk.h>

// Entry in a partition table (i.e. a partition)
 typedef struct {
 	uint64_t lba_start;
 	uint64_t lba_length;
 } ptable_entry_t;

// Kinds of partition tables
typedef enum {
	kPartitionTableUnknown,
	kPartitionTableMBR
} ptable_type_t;

// Defines a partition table
typedef struct {
	disk_t *disk;
	ptable_type_t type;

	ptable_entry_t *first;
} ptable_t;

#endif