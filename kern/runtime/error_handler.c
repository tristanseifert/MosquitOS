#include <types.h>
#include <sys/system.h>
#include <sys/paging.h>

#include "error_handler.h"

void error_dump_regs(err_registers_t regs);
extern void panic_halt_loop(void);

uint32_t error_cr0, error_cr1, error_cr2, error_cr3;

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

	kprintf("\n\n\x01\x11%s\x01\x10\nError code: 0x%X\n\n", &err_names[regs.int_no], regs.err_code);

	// Dump the registers now.
	static char reg_names[18][4] = {
		" DS",
		"EDI",
		"ESI",
		"EBP",
		"ESP",
		"EBX",
		"EDX",
		"ECX",
		"EAX",
		"EIP",
		" CS",
		"EFG",
		"USP",
		" SS",
		"CR0",
		"CR1",
		"CR2",
		"CR3",
	};

	uint32_t registers[18] = {
		regs.ds, regs.edi, regs.esi, regs.ebp, regs.esp, regs.ebx, regs.edx, regs.ecx,
		regs.eax, regs.eip, regs.cs, regs.eflags, regs.useresp, regs.ss, error_cr0, error_cr1,
		error_cr2, error_cr3
	};

	for(uint8_t i = 0; i < 18; i+=2) {
		kprintf("%s: 0x%8X\t %s: 0x%8X\n", &reg_names[i], registers[i], &reg_names[i+1], registers[i+1]);
	}

	// Halt by going into an infinite loop.
	panic_halt_loop();
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

		error_dump_regs(regs);

		while(1);
	//}
}