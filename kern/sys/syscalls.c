#include <types.h>
#include "syscalls.h"
#include "syscall.h"

/*
 * Stub for unimplemented syscalls.
 */
int syscall_stub(void* task, syscall_callstack_t regs, void* syscall_struct) {
	return -1;
}