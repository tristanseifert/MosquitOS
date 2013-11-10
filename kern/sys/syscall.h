#ifndef SYSCALL_H
#define SYSCALL_H

#include <types.h>

#define SYSCALL_STRUCT_MAGIC "SYCL"

// these are endian swapped because gcc sucks
#define SYSCALL_SYSENTER_OPCODE 0x340F
#define SYSCALL_SYSEXIT_OPCODE 0x350F

#define SYSCALL_TABLE_SIZE 64

typedef struct registers {
   uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
} syscall_callstack_t;

typedef int (*syscall_routine)(void* task, syscall_callstack_t regs, void* info);

void syscall_init();
int syscall_handler(syscall_callstack_t regs);

#endif