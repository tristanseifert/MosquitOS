Without some form of filesystem interface, the kernel would not be able to read files from a disk in a user-friendly way, instead relyong on hard-coded sector locations and other such messes, in addition to creating a whole plethora of issues for users.

Therefore, the MosquitOS kernel exposes an interface that filesystem modules can use to register themselves with the kernel, allowing the kernel to select the correct filesystem, permitting file read and writes, as well as a standardised way of reading metadata. In addition, this interface allows the kernel to load more filesystem modules at runtime to support additional filesystems without having to link them into the kernel itself.

# Kernel Functions
In order to allow modules to register and interract with the kernel, it exposes the following functions:

