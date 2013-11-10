#include <types.h>
#include "syscall.h"
#include "system.h"
#include "sched.h"
#include "syscalls.h"

extern void syscall_handler_stub(void);

/* Stores the return value of the function to be placed in %eax on return by the assembly
 * syscall handler so we don't need to modify the stack image
 */
int syscall_return_value;

/*
 * This table holds an array for each available syscall in the system. The compiler will
 * fetch the address of the function, place it in the array, and then we can jump to it
 * from our syscall handler.
 */
syscall_routine syscall_table[SYSCALL_TABLE_SIZE] = {
		syscall_stub
	};

/*
 * Initialises the syscall environment by configuring MSRs.
 */
void syscall_init() {
	// Set GDT entry for system code segment
	sys_write_MSR(SYS_MSR_IA32_SYSENTER_CS, SYS_KERN_CODE_SEG, 0);

	// Set kernel stack and syscall entry point
	sys_write_MSR(SYS_MSR_IA32_SYSENTER_ESP, SYS_KERN_SYSCALL_STACK, 0);
	sys_write_MSR(SYS_MSR_IA32_SYSENTER_EIP, (uint32_t) syscall_handler_stub, 0);
}

/*
 * Handler that processes all syscalls given to the OS by the SYSENTER instruction.
 * The struct passed in contains the info that was pushed on the stack, including
 * contents of registers before the call to this function.
 *
 * The C library (or whatever other library is in use) will have placed the syscall
 * that it wants executed into the EBX register, and a pointer to a syscall-specific
 * information struct in the EAX register, if any. Alternatively, if the syscall takes
 * only a single argument that can fit in a register, it is placed into EAX.
 *
 * Before calling this function, it's imperative that the caller places the return
 * address directly after the SYSENTER opcode into EDX, and the stack pointer to
 * restore into ECX. This is done by the _kern_syscall stub in process memory.
 */
int syscall_handler(syscall_callstack_t regs) {
	// Syscall number and info
	uint32_t syscall_id = regs.ebx;
	void* syscall_struct = (void *) regs.eax;

	// The task that called this syscall
	i386_task_t *task = sched_curr_task();

	if(syscall_id > SYSCALL_TABLE_SIZE) {
		kprintf("Got invalid syscall 0x%X\n", syscall_id);
	} else {
		syscall_return_value = (syscall_table[syscall_id](task, regs, syscall_struct));
	}

	// Before we run the syscall, ensure the return address in EDX is valid
	uint16_t *returnAddress = ((uint16_t *) regs.edx)-1; // 1 word = size of SYSENTER opcode
	uint16_t opcode = *returnAddress;

	if(opcode != SYSCALL_SYSENTER_OPCODE) {
		kprintf("Invalid return address: process will be killed!");
	}

	return 0;
}