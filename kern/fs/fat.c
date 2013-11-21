#include <types.h>
#include "fat.h"
#include "vfs.h"

static uint32_t fat_get_next_cluster(fs_superblock_t* superblock, uint32_t cluster_value);
static fs_superblock_t* fat_make_superblock(fs_superblock_t* superblock, ptable_entry_t* pt);
static char* fat_83_to_str(fat_dirent_t* dirent);

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

	// Calculate some variables
	fs_info->root_dir_sectors = ((fs_info->bpb->root_entry_count * 32) + (fs_info->bpb->bytes_per_sector - 1)) / fs_info->bpb->bytes_per_sector;
	fs_info->first_fat_sector = fs_info->bpb->reserved_sector_count;

	// Do FAT-specific calculations
	if(fs_info->fat_type == 32) {
		fat_fs_bpb32_t *bpb = (fat_fs_bpb32_t *) fs_info->bpb;

		fs_info->first_data_sector = bpb->reserved_sector_count + (bpb->table_count * bpb->table_size_32);
		fs_info->data_sectors = bpb->total_sectors_32 - (bpb->reserved_sector_count + (bpb->table_count * bpb->table_size_32) + fs_info->root_dir_sectors);

		fs_info->total_clusters = fs_info->data_sectors / bpb->sectors_per_cluster;

		fs_info->root_cluster = bpb->root_cluster;
	} else if(fs_info->fat_type == 16) {
		fat_fs_bpb16_t *bpb = (fat_fs_bpb16_t *) fs_info->bpb;

		fs_info->first_data_sector = bpb->reserved_sector_count + (bpb->table_count * bpb->table_size_16);
		fs_info->data_sectors = bpb->total_sectors_16 - (bpb->reserved_sector_count + (bpb->table_count * bpb->table_size_16) + fs_info->root_dir_sectors);

		fs_info->total_clusters = fs_info->data_sectors / bpb->sectors_per_cluster;

		fs_info->root_cluster = fs_info->first_data_sector;
	}

	kprintf("Root directory = 0x%X (size = 0x%X), first FAT sector = 0x%X\n0x%X data sectors (first = 0x%X), total clusters = 0x%X\n\n", fs_info->root_cluster, fs_info->root_dir_sectors,
		fs_info->first_fat_sector, fs_info->data_sectors, fs_info->first_data_sector, fs_info->total_clusters);

	// Compute the number of bytes the root directory takes up by following cluster chain
	fs_info->root_dir_length = 0x200 * 0x20;
	fs_info->root_directory = (void *) kmalloc(fs_info->root_dir_length);

	// Read the root directory of the filesystem
	fat_read_get_root_dir(superblock, fs_info->root_directory, fs_info->root_dir_length);

	uint8_t *ptr;
	uint32_t size;

	kprintf("\nContents of /:\n");
	for(int i = 0; i < (fs_info->root_dir_length / 0x20); i++) {
		ptr = fs_info->root_directory+(i * 0x20);
		fat_dirent_t *dirent = (fat_dirent_t *) ptr;

		if(ptr[0] != 0xE5 && ptr[0] != 0x00) {
			if(dirent->attributes != (FAT_ATTR_LFN) && !(dirent->attributes & FAT_ATTR_DIRECTORY)) {
				size = (ptr[0x1F] << 0x18) | (ptr[0x1E] << 0x10) | (ptr[0x1D] << 0x8) | ptr[0x1C];

				kprintf("%s (%i bytes)\n", fat_83_to_str(dirent), size);
			} else if(dirent->attributes & FAT_ATTR_DIRECTORY) {
				kprintf("%s (directory)\n", fat_83_to_str(dirent));
			}
		}
	}

	return superblock;
}

/*
 * Reads the filesystem's root directory into the buffer specified, until either
 * the read is complete, or buffer_size have been copied.
 */
void fat_read_get_root_dir(fs_superblock_t* superblock, void* buffer, uint32_t buffer_size) {
	fat_fs_info_t *fs_info = (fat_fs_info_t *) superblock->fs_info;

	if(fs_info->fat_type == 32) {
		fat_fs_bpb32_t *bpb = (fat_fs_bpb32_t *) fs_info->bpb;

		uint32_t sector;
		uint32_t cluster_to_read = fs_info->root_cluster-2;
		DISK_ERROR derr;

		uint8_t *write_ptr = buffer;
		int bytes_written = 0;

		// Read the entire directory
		while(true) {
			sector = PARTITION_LBA_REL2ABS(cluster_to_read+fs_info->first_data_sector, superblock->pt);
			derr = disk_read(superblock->disk, sector, 1, sector_buffer);

			// Handle disk read errors
			if(derr != kDiskErrorNone) {
				kprintf("Error reading root directory: 0x%X\n", derr);
				break;
			}

			// Copy into buffer
			if(buffer_size > 512) {
				memcpy(write_ptr, sector_buffer, 512);

				buffer_size -= 512;
				write_ptr += 512;
				bytes_written += 512;
			} else { // do only partial copy
				memcpy(write_ptr, sector_buffer, (512 - buffer_size));
				bytes_written += (512 - buffer_size);
			}

			// See if there's another cluster in the chain
			cluster_to_read = fat_get_next_cluster(superblock, cluster_to_read);

			// no additional cluster to read
			if(cluster_to_read == 0xFFFFFFFF) break;
		}
	} else if(fs_info->fat_type == 16) {
		fat_fs_bpb16_t *bpb = (fat_fs_bpb16_t *) fs_info->bpb;

	}
}

/*
 * Reads the filesystem's FAT and determines if there the cluster specified is
 * followed by another cluster, or if it is the last cluster in the cluster
 * chain. Returns 0xFFFFFFFF if last cluster, or 0xFFFFFFFE if an error 
 * occurred.
 */
static uint32_t fat_get_next_cluster(fs_superblock_t* superblock, uint32_t cluster_value) {
	fat_fs_info_t *fs_info = (fat_fs_info_t *) superblock->fs_info;

	if(fs_info->fat_type == 32) {
		fat_fs_bpb32_t *bpb = (fat_fs_bpb32_t *) fs_info->bpb;

		cluster_value &= FAT32_MASK;

		// Allocate sector read buffer
		uint32_t *buffer = (uint32_t *) kmalloc(0x200);

		uint32_t fat_sector = bpb->reserved_sector_count + ((cluster_value * 4) / bpb->bytes_per_sector);
		uint32_t fat_offset = (cluster_value * 4) % bpb->bytes_per_sector;

		// Read FAT
		DISK_ERROR derr;
		derr = disk_read(superblock->disk, PARTITION_LBA_REL2ABS(fat_sector, superblock->pt), 1, buffer);

		// Handle disk read errors
		if(derr != kDiskErrorNone) {
			kprintf("Error reading FAT: 0x%X\n", derr);
			return 0xFFFFFFFE;
		}

		uint32_t next_cluster = buffer[cluster_value & 0x7F];

		// Release memory
		kfree(buffer);

		return (next_cluster >= FAT32_END_CHAIN) ? 0xFFFFFFFF : (next_cluster & FAT32_MASK);
	} else if(fs_info->fat_type == 16) {
		fat_fs_bpb16_t *bpb = (fat_fs_bpb16_t *) fs_info->bpb;

	}

	return 0xFFFFFFFF;
}

/*
 * Reads the 8.3 filename from a directory entry struct and converts it into a standard
 * 0-terminated string.
 */
static char* fat_83_to_str(fat_dirent_t* dirent) {
	char* buf = (char *) kmalloc(14);
	memclr(buf, 14);

	int buf_offset = 0;

	// filename
	for(int i = 0; i < 8; i++) {
		if(dirent->name[i] != ' ') {
			buf[buf_offset++] = dirent->name[i];
		}
	}

	// dot
	buf[buf_offset++] = '.';

	// extension
	for(int i = 0; i < 3; i++) {
		if(dirent->ext[i] != ' ') {
			buf[buf_offset++] = dirent->ext[i];
		}
	}

	return buf;
}