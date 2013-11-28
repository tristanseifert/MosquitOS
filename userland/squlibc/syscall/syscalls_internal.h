#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdint-gcc.h>
#include <limits.h>

#include <stdio.h>
#include "errno.h" 

// Syscall numbers
#include "syscall_num.h"

// Internal functions
__attribute__((visibility("internal"))) int do_syscall(int num, void* info);

// Syscall structures
typedef struct {
	void *data;
	size_t count;
	size_t size;
	FILE* file;
} syscall_write_struct;