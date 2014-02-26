#include <types.h>
#include <sys/system.h>
#include <sys/paging.h>
#include <sys/binfmt_elf.h>

#include "error_handler.h"

// ELF sections useful for stack dumps
extern char *kern_elf_strtab;
extern elf_symbol_entry_t *kern_elf_symtab;
extern unsigned int kern_elf_symtab_entries;

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

	kprintf("\n\x01\x11%s\x01\x10\nError code: 0x%X\n\n", &err_names[regs.int_no], regs.err_code);

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

	kprintf("\n\x01\x11Stack trace:\x01\x10\n");
	error_dump_stack_trace(16);
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

		// Halt by going into an infinite loop.
		panic_halt_loop();
	//}
}

/*
 * Dumps a stack trace, up to maxFrames deep.
 */
void error_dump_stack_trace(unsigned int maxFrames) {
	// Stack contains:
	//  Second function argument
	//  First function argument (MaxFrames)
	//  Return address in calling function
	//  ebp of calling function (pointed to by current ebp)

	// Read current EBP
	unsigned int *ebp = &maxFrames - 2;
	uint32_t temp;
	__asm__ volatile("mov %%ebp, %0" : "=r" (temp));
	ebp = (unsigned int *) temp;

	unsigned int eip = 0;

	// Iterate through each stack frame
	for(unsigned int frame = 0; frame < maxFrames; ++frame) {
		// Get instruction pointer
		eip = ebp[1];
		// Find closest symbol name
		char *closest_symbol = error_get_closest_symbol(eip);
		// Print
		kprintf("%.3u: %s (0x%X)\n", frame, closest_symbol, eip);
		
		// Unwind to previous stack frame
		ebp = (unsigned int *) ebp[0];

		// Is there a stack frame before this one? (EBP != 0)
		if(!ebp) {
			eip = 0;
			kprintf("-- End of stack trace. --\n");
			break;
		}
	}

	// Print an end label if needed
	if(eip) {
		kprintf("-- End of stack trace. --\n");
	}
}

/*
 * Finds the name of the symbol closest to the specified address.
 */
char *error_get_closest_symbol(unsigned int address) {
	unsigned int index = 0; // index of closest
	unsigned int abs_offset = 0xFFFFFFFF; // distance from it (absolute)
	elf_symbol_entry_t *entry;

	for(unsigned int i = 0; i < kern_elf_symtab_entries; i++) {
		entry = &kern_elf_symtab[i];

		// Calculate offset
		int offset = address - entry->st_address;

		// Ignore negative offsets
		if(offset > 0) {
			// Is it closer than the last?
			if(offset < abs_offset) {
				// If so, save offset
				abs_offset = (unsigned int) offset;
				index = i;
			}
		}
	}

	// Get closest entry
	entry = &kern_elf_symtab[index];
	char *name = kern_elf_strtab + entry->st_name;
	int offset = address - entry->st_address;

	if(offset == 0) {
		// The address is the start of the symbol
		return name;
	} else { // Offset
		static char outBuffer[512];

		if(offset > 0) { // positive offset
			sprintf(outBuffer, "%s+%i", name, offset);
		} else if(offset < 0) { // negative offset
			sprintf(outBuffer, "%s-%i", name, -offset);
		}

		return outBuffer;
	}
}