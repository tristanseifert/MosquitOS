Without some form of filesystem interface, the kernel would not be able to read files from a disk in a user-friendly way, instead relyong on hard-coded sector locations and other such messes, in addition to creating a whole plethora of issues for users.

Therefore, the MosquitOS kernel exposes an interface that filesystem modules can use to register themselves with the kernel, allowing the kernel to select the correct filesystem, permitting file read and writes, as well as a standardised way of reading metadata. In addition, this interface allows the kernel to load more filesystem modules at runtime to support additional filesystems without having to link them into the kernel itself.

# Kernel Functions
In order to allow modules to register and interract with the kernel, it exposes the following functions:

## `vfs_register`
Registers a filesystem with the kernel. The only argument is a pointer to a struct of type `fs_type_t`, which identifies the fileystem type to the kernel. This struct is defined as follows:

	typedef struct fs_type {
		const char *name;
		uint32_t flags;
		uint32_t handle; // set by kernel, should be 0
		uint32_t type; // parition table ID

		void *owner;

		fs_superblock_t* (*create_super) (fs_superblock_t*, ptable_entry_t*);

		struct fs_type *next; // held internally as doubly linked list
		struct fs_type *prev;
	} fs_type_t;


This kernel function will return an error code or 0 if success.

Note that the pointer to the `fs_type_t` struct must remain valid for the entire time the filesystem is loaded into the kernel. Deallocate the memory only when it has been successfully unloaded.

## `vfs_deregister`
Takes a previously-registered filesystem (pointer to the `fs_type_t` struct used to register the filesystem) and removes it from the kernel's list of supported filesystems.

Note that any partitions that utilise this filesystem module and are currently mounted *must* be unmounted before this module can be unloaded, or an error will be returned.

On success, this function will return 0, otherwise an error code will be returned.

Note that any memory held by the filesystem module has to be released by it when unloading, as the kernel does not keep track of the memory allocated by the module. If not released, it will linger around forever as 'zombie' memory blocks that cannot be re-allocated.

### `create_super` function pointer
`create_super` is a function pointer to a function in the filesystem module that will read the required filesystem information from the partition specified in the second argument, then create a superblock from this information and place it into the memory pointed by the first argument. 

If there is an error while creating the superblock, the function shall return `NULL`, otherwise the original superblock pointer.