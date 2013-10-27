#ifndef SYSCALL_H
#define SYSCALL_H

#include <types.h>

#define SYSCALL_SYSENTER_OPCODE 0x0F34
#define SYSCALL_SYSEXIT_OPCODE 0x0F35

typedef struct registers {
   uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
} syscall_callstack_t;

void syscall_init();
int syscall_handler(syscall_callstack_t regs);

#endif