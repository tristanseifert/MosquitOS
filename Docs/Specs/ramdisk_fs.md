# What is QRFS?
**QRFS**, or **Q**uick **R**AMDisk **F**ile**s**ystem, is intended to be a simple filesystem for the MosquitOS kernel to read drivers and such from during the kernel boot process, before the root filesystem can be mounted. As such, this filesystem does not support much more than read-only mounting, no folders, and no extended attributes for files. However, it's easy to parse and works quickly!

# File Format
First of all, it should be noted that files inside a valid QRFS image should be aligned to 512-byte boundaries, as to allow a single sector read operation to read an entire file, without having to discard the unwanted data.

In addition, integers and pointers should be specified as little-endian, and character strings specified as UTF-8 and zero-terminated. Pointers within the file are absolute to the first byte of the header.

## Header
At offset `0x00000000` of any valid QRFS is a 512-byte header that is used to identify the filesystem as valid, as well as to specify the location of the file table. This header is formatted as follows:

	typedef struct qrfs_header {
		uint32_t magic;
		uint32_t version;

		qrfs_file_entry_t *file_table;
	} qrfs_header_t;

**`magic`:** Differentiates any other volume from a valid QRFS volume. Should be "QRFS" (`0x51524653`) on valid volumes for them to be recognised.
**`version`:** Version of QRFS this image contains. The current version is `0x00000001`.
**`file_table`:** Pointer to the first entry in the file table.

## File Table
The File Table is a doubly-linked list of structures that holds information about files. The format of a file table entry is as follows:

	typedef struct qrfs_file_entry {
		char[] name;

		uint32_t file_start;
		uint32_t length;
		uint32_t attributes;

		struct qrfs_file_entry *next;
		struct qrfs_file_entry *prev;
	} qrfs_file_entry_t;

**`name`:** A `char` array that holds the name of this file.
**`file_start`:** Offset from the first byte of the header to the start of the file.
**`length`:** Length of the file in bytes.
**`attributes`:** A bitfield of attributes. See the attributes section below.
**`next`** and **`prev`:** Pointer to the next and previous entries in the file table, or NULL if they entry is the last or first in the list.

### Attributes
Files can have several special attributes, each of which are controlled by a bit in the `attributes` field:
*	**Compressed:** Indicates the file is compressed with gzip compression. The `length` field represents the final uncompressed size of the file. (`0x00000001`)

All undefined bit flags should be interpreted as being "reserved" and should be set to 0 when initialising the filesystem, and when altering the flags of the filesystem, these unused bits should be preserved in order to allow for forwards-compatibility with later features.