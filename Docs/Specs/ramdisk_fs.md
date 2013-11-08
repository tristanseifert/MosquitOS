# What is QRFS?
*QRFS*, or *Q*uick *R*AMDisk *F*ile*s*ystem, is intended to be a simple filesystem for the MosquitOS kernel to read drivers and such from during the kernel boot process, before the root filesystem can be mounted. As such, this filesystem does not support much more than read-only mounting, fixed-length filenames, no folders, and no extended attributes for files. However, it's easy to parse!

# File Format
First of all, it should be noted that files inside a valid QRFS image should be aligned to 512-byte boundaries, as to allow a single sector read operation to read an entire file, without having to discard the unwanted data.

In addition, integers and pointers should be specified as little-endian, and character strings specified as UTF-8 and zero-terminated. Pointers within the file are absolute to the first byte of the header.

## Header
At offset `0x00000000` of any valid QRFS is a 512-byte header that is used to identify the filesystem as valid, as well as to specify the location of the file table.