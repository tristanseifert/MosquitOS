#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "task.h"
#include "syscall.h"

/*
 * Syscalls exported to syscall.h: Each function should follow this prototype:
 * typedef int (*syscall_routine)(void* task, syscall_callstack_t regs, void* info);
 */

int syscall_stub(void* task, syscall_callstack_t regs, void* syscall_struct);

#endif