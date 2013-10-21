#include <types.h>
#include <device/vga.h>
#include <io/terminal.h>
#include <sys/system.h>
#include <sys/paging.h>

#include "error_handler.h"

void error_dump_regs(err_registers_t regs);

uint32_t error_cr0, error_cr1, error_cr2, error_cr3;

/*
 * Initialises the VGA hardware for displaying an error screen.
 */
void error_init() {
	terminal_setColour(vga_make_colour(VGA_COLOUR_WHITE, VGA_COLOUR_BLUE));
	terminal_clear();

	terminal_setColour(vga_make_colour(VGA_COLOUR_WHITE, VGA_COLOUR_BLUE));
	terminal_setPos(32, 2);
	terminal_write_string("GURU MEDITATION");
	terminal_setPos(21, 4);
	terminal_write_string("(Or, \"piss off, you broke something!\")");
}

/*
 * Dumps registers and exception name.
 */
void error_dump_regs(err_registers_t regs) {
	static char err_names[19][28] = {
		"Division by Zero",
		"Debug Exception",
		"Non-Maskable Interrupt",
		"Breakpoint Exception",
		"Into Detected Overflow",
		"Out of Bounds Exception",
		"Invalid Opcode Exception",
		"No Coprocessor Exception",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Bad TSS",
		"Segment not Present",
		"Stack Fault",
		"General Protection Fault",
		"Page Fault",
		"Unknown Interrupt Exception",
		"Coprocessor Fault",
		"Alignment Check Exception",
		"Machine Check Exception"
		};

	terminal_setPos(4, 6);
	terminal_setColour(vga_make_colour(VGA_COLOUR_RED, VGA_COLOUR_WHITE));
	terminal_write_string((char *) &err_names[regs.int_no]);
	terminal_setColour(vga_make_colour(VGA_COLOUR_WHITE, VGA_COLOUR_BLUE));

	terminal_setPos(4, 8);
	terminal_write_string("Error Code: ");
	terminal_write_byte(regs.err_code);

	// Dump the registers now.
	static char reg_names[18][8] = {
		" DS: 0x",
		"EDI: 0x",
		"ESI: 0x",
		"EBP: 0x",
		"ESP: 0x",
		"EBX: 0x",
		"EDX: 0x",
		"ECX: 0x",
		"EAX: 0x",
		"EIP: 0x",
		" CS: 0x",
		"EFG: 0x",
		"USP: 0x",
		" SS: 0x",
		"CR0: 0x",
		"CR1: 0x",
		"CR2: 0x",
		"CR3: 0x",
	};

	uint32_t registers[18] = {
		regs.ds, regs.edi, regs.esi, regs.ebp, regs.esp, regs.ebx, regs.edx, regs.ecx,
		regs.eax, regs.eip, regs.cs, regs.eflags, regs.useresp, regs.ss, error_cr0, error_cr1,
		error_cr2, error_cr3
	};

	for(uint8_t i = 0; i < 18; i+=2) {
		terminal_setPos(4, (i/2)+10);

		terminal_write_string((char *) &reg_names[i]);
		terminal_write_dword(registers[i]);

		terminal_setPos(22, (i/2)+10);

		terminal_write_string((char *) &reg_names[i+1]);
		terminal_write_dword(registers[i+1]);
	}
}

/*
 * Called by ISRs when an error occurrs.
 */
void error_handler(err_registers_t regs) {
	//if(regs.int_no == 14) {
	//	paging_page_fault_handler();
	//} else {
		uint32_t obama;
		__asm__ volatile("mov %%cr2, %0" : "=r" (obama));
		error_cr2 = obama;
		__asm__ volatile("mov %%cr0, %0" : "=r" (obama));
		error_cr0 = obama;
		__asm__ volatile("mov %%cr3, %0" : "=r" (obama));
		error_cr3 = obama;

		error_init();
		error_dump_regs(regs);

		while(1);
	//}
}