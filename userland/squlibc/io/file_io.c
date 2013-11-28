#include "io_internal.h"
#include <syscall/syscalls_internal.h>
#include <unistd.h>

size_t fwrite(const void *ptr, size_t size, size_t nobj, FILE *stream) {
	syscall_write_struct infoStruct;

	infoStruct.data = ptr;
	infoStruct.count = nobj;
	infoStruct.size = size;
	infoStruct.file = stream;

	errno = do_syscall(SYSCALL_WRITE, &infoStruct);
	return errno;
}