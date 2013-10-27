#include <types.h>
#include "io/terminal.h"
#include "syscall.h"
#include "system.h"

extern void syscall_handler_stub(void);

/*
 * Initialises the syscall environment by configuring MSRS.
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
 * information struct in the EAX register, if any.
 *
 * Before calling this function, it's imperative that the caller places the return
 * address directly after the SYSENTER opcode into EDX, and the stack pointer to
 * restore into ECX.
 */
int syscall_handler(syscall_callstack_t regs) {
	terminal_setColour(vga_make_colour(VGA_COLOUR_WHITE, VGA_COLOUR_GREEN));
	terminal_clear();
	terminal_setColour(vga_make_colour(VGA_COLOUR_WHITE, VGA_COLOUR_GREEN));

	terminal_setPos(4, 2);
	terminal_write_string("Syscall Triggered!\n\n");

	uint32_t syscall_id = regs.ebx;
	void* syscall_struct = (void *) regs.eax;

	terminal_setPos(4, 4);
	terminal_write_string("Syscall: 0x");
	terminal_write_dword(syscall_id);
	terminal_write_string(" Param Ptr: 0x");
	terminal_write_dword((uint32_t) syscall_struct);

	static char reg_names[8][8] = {
		"EDI: 0x",
		"ESI: 0x",
		"EBP: 0x",
		"ESP: 0x",
		"EBX: 0x",
		"EDX: 0x",
		"ECX: 0x",
		"EAX: 0x",
	};

	uint32_t registers[8] = {
		regs.edi, regs.esi, regs.ebp, regs.esp, regs.ebx, regs.edx, regs.ecx,
		regs.eax,
	};

	for(uint8_t i = 0; i < 8; i+=2) {
		terminal_setPos(4, (i/2)+6);

		terminal_write_string((char *) &reg_names[i]);
		terminal_write_dword(registers[i]);

		terminal_setPos(22, (i/2)+6);

		terminal_write_string((char *) &reg_names[i+1]);
		terminal_write_dword(registers[i+1]);
	}

	// Before we run the syscall, ensure the return address in EDX is valid
	uint16_t *returnAddress = ((uint16_t *) regs.edx)-1; // 1 word = size of SYSENTER opcode
	uint16_t opcode = *returnAddress;

	if(opcode != SYSCALL_SYSENTER_OPCODE) {
		terminal_setPos(4, 11);
		terminal_write_string("Invalid return address: process will be killed!");
	}


	while(1);

	return 0;
}