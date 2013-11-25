#include <types.h>
#include "fat.h"
#include "vfs.h"
#include "vga/svga.h"

static void fat_read_cluster(fs_superblock_t* superblock, uint32_t cluster, void* buffer, uint32_t buffer_size);
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

		superblock->vol_label = (char *) &bpb->volume_label;
	} else if(fs_info->fat_type == 16) {
		fat_fs_bpb16_t *bpb = (fat_fs_bpb16_t *) fs_info->bpb;

		fs_info->first_data_sector = bpb->reserved_sector_count + (bpb->table_count * bpb->table_size_16);
		fs_info->data_sectors = bpb->total_sectors_16 - (bpb->reserved_sector_count + (bpb->table_count * bpb->table_size_16) + fs_info->root_dir_sectors);

		fs_info->total_clusters = fs_info->data_sectors / bpb->sectors_per_cluster;

		fs_info->root_cluster = fs_info->first_data_sector;

		superblock->vol_label = (char *) &bpb->volume_label;
	}

	// Compute the number of bytes the root directory takes up by following cluster chain
	fs_info->root_dir_length = 0x200 * 0x20;
	fs_info->root_directory = (void *) kmalloc(fs_info->root_dir_length);

	// Read the root directory of the filesystem
	fat_read_get_root_dir(superblock, fs_info->root_directory, fs_info->root_dir_length);

	// Set up the interface functions to access filesystems
	superblock->fp_read_file = fat_read_file;
	superblock->fp_read_directory = fat_read_directory;
	superblock->fp_unmount = fat_unmount;

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

		uint32_t cluster_to_read = fs_info->root_cluster-2;

		// Read root directory
		fat_read_cluster(superblock, cluster_to_read, buffer, buffer_size);
	} else if(fs_info->fat_type == 16) {
		fat_fs_bpb16_t *bpb = (fat_fs_bpb16_t *) fs_info->bpb;

	}
}

/*
 * Reads data starting at a specified sector until either all data has been
 * read into the buffer, or the buffer is filled up to buffer_size bytes.
 */
void fat_read_cluster(fs_superblock_t* superblock, uint32_t cluster, void* buffer, uint32_t buffer_size) {
	fat_fs_info_t *fs_info = (fat_fs_info_t *) superblock->fs_info;

	uint32_t sector;
	uint32_t cluster_to_read = cluster & FAT32_MASK;
	DISK_ERROR derr;

	uint8_t *write_ptr = buffer;
	int bytes_written = 0;

	// Read the entire directory
	while(true) {
		sector = PARTITION_LBA_REL2ABS(cluster_to_read+fs_info->first_data_sector, superblock->pt);
		derr = disk_read(superblock->disk, sector, 1, sector_buffer);

		// Handle disk read errors
		if(derr != kDiskErrorNone) {
			kprintf("Error reading file: 0x%X\n", derr);
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
		cluster_to_read = fat_get_next_cluster(superblock, cluster_to_read+2);

		// no additional cluster to read
		if(cluster_to_read == 0xFFFFFFFF) break;
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
	char* buf = (char *) kmalloc(16);
	memclr(buf, 16);

	int buf_offset = 0;

	// filename
	for(int i = 0; i < 8; i++) {
		// FAT pads filenames with spaces
		if(dirent->name[i] != ' ' && dirent->name[i] != 0x00) {
			buf[buf_offset++] = dirent->name[i];
		}
	}

	// Directories don't get extensions
	if(dirent->attributes & FAT_ATTR_DIRECTORY) return buf;

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

/*
 * Reads the directory table for the specified directory. Traverses all parent
 * directories, and returns NULL on error, or a pointer to the directory data if
 * successful.
 *
 * The VFS layer uses the forward slash ("/") to delimit different levels of
 * folders and files. These locations are relative to the root of the
 * filesystem, and any non-existent directory will cause an error to be raised.
 */
void* fat_read_directory(fs_superblock_t* superblock, char* path) {
	// Get first component of the path
	char* pch = strtok(path, "/");

	// Get pointer to FS info
	fat_fs_info_t *fs_info = (fat_fs_info_t *) superblock->fs_info;

	// Pointer to directory table (root dir to start with)
	void *directoryTable = fs_info->root_directory;
	void *dirTableRead;
	fat_dirent_t *dirent;
	uint32_t cluster;

	// Allocate some temporary memory for directory tables
	void *readMem = (void *) kmalloc(0x8000);

	// Iterate through all paths
	while(pch != NULL) {
		dirTableRead = directoryTable;

		// Iterate through directory's contents
		while(true) {
			dirent = (fat_dirent_t *) dirTableRead;

			// Is this the last entry?
			if(dirent->name[0] == 0x00) {
				kprintf("Could not find directory %s\n", pch);
				goto error;
			} else { // We have more entries to process
				if(dirent->attributes & FAT_ATTR_DIRECTORY) {
					// It's a directory entry, so compare filename
					if(strncasecmp(pch, (char *) &dirent->name, strlen(pch)) == 0) {
						cluster = ((dirent->cluster_high << 0x10) | (dirent->cluster_low))-2;
						fat_read_cluster(superblock, cluster, readMem, 0x8000);

						break;
					}
				}
			}

			// Go to next directory entry
			dirTableRead += 0x20;
		}

		pch = strtok(NULL, "/");

		// Make sure we don't release the root directory's memory
		if(directoryTable != fs_info->root_directory) {
			kfree(directoryTable);
		}

		// Parse the directory we just read on the next iteration
		directoryTable = readMem;
	}

	// We drop down here if the above loop exits
	return readMem;

	// If there's an error (or a directory doesn't exist) we get here
error: ;
	kfree(readMem);
	return NULL;
}

/*
 * Reads a file from the filesystem, allocating a memory buffer for it.
 */
void* fat_read_file(fs_superblock_t* superblock, char* file, void* buffer, uint32_t buffer_size) {
	// Get first component of the path
	char* pch = strtok(file, "/");
	char* pch2 = pch;

	// Variables
	char *filename = 0x00;
	char *path = file;
	char *pretty_filename;

	uint32_t cluster = 0;

	// Split filename and path
	while(pch != NULL) {
		pch = strtok(NULL, "/");

		if(pch == NULL) {
			filename = pch2;
			pch2--;
			*pch2 = 0x00;
			break;
		}

		pch2 = pch;
	}

	// Read the directory
	void *dirTable = fat_read_directory(superblock, path);

	// Search through the directory for the file
	uint8_t* ptr = dirTable;

	while(true) {
		fat_dirent_t *dirent = (fat_dirent_t *) ptr;

		// Have we reached the end of the directory?
		if(unlikely(dirent->name[0] == 0x00)) break;

		// Is this entry containing something?
		if(ptr[0] != 0xE5 && ptr[0] != 0x00) {
			// Is it a file?
			if(dirent->attributes != (FAT_ATTR_LFN) && !(dirent->attributes & FAT_ATTR_DIRECTORY)) {
				uint32_t size = (ptr[0x1F] << 0x18) | (ptr[0x1E] << 0x10) | (ptr[0x1D] << 0x8) | ptr[0x1C];
				
				pretty_filename = fat_83_to_str(dirent);

				// Is it the file we're looking for?
				if(unlikely(strncasecmp(filename, (char *) pretty_filename, strlen(pretty_filename)) == 0)) {
					if(!buffer) {
						buffer = (void *) kmalloc(size);
						buffer_size = size;
					}

					cluster = ((dirent->cluster_high << 0x10) | (dirent->cluster_low))-2;

					// Clean up memory
					kfree(pretty_filename);

					// Read the file
					goto readFile;
				}

				// Clean up memory
				kfree(pretty_filename);
			}
		}

		ptr += 0x20;
	}

	// File could not be found !!!
	return NULL;

readFile: ;
	kprintf("Reading %s from cluster 0x%X\n", filename, cluster);
	fat_read_cluster(superblock, cluster, buffer, buffer_size);

	// Free memory
	kfree(dirTable);

	return buffer;
}

/*
 * Unmounts the filesystem.
 */
int fat_unmount(fs_superblock_t* superblock) {
	return EBUSY;
}