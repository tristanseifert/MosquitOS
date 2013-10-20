#include <types.h>

/*
 * This file (and the associated .c) contains most of the variables and code that sets
 * up the system into a defined state, and also activates and makes use of a lot of
 * the additional hardware in the PC.
 *
 * In addition, it can call other drivers (for example, PCI, VGA, etc) and initialise
 * those as well.
 */

#define SYS_IDT_MEMLOC 0x00400000
#define SYS_GDT_MEMLOC 0x00410000
#define SYS_TSS_MEMLOC 0x00420000

#define SYS_NUM_TSS 8
#define SYS_TSS_LEN	0x80

typedef struct sys_idt_descriptor {
   uint16_t offset_1;	// offset bits 0..15
   uint16_t selector;	// a code segment selector in GDT or LDT
   uint8_t zero;		// unused, set to 0
   uint8_t flags;		// type and attributes
   uint16_t offset_2;	// offset bits 16..31
} __attribute__((packed)) idt_entry_t;

typedef struct sys_gdt_descriptor {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_middle;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

void system_init();

void sys_set_idt_gate(uint8_t entry, uint32_t function, uint8_t segment, uint8_t flags);

void sys_setup_ints();

void sys_set_gdt_gate(uint16_t num, uint32_t base, uint32_t limit, uint8_t flags, uint8_t gran);

uint64_t sys_get_ticks();

bool sys_irq_enabled();

void sys_cpuid(uint32_t code, uint32_t* a, uint32_t* d);
void sys_rdtsc(uint32_t* upper, uint32_t* lower);
void sys_flush_tlb(uint32_t m);
void sys_write_MSR(uint32_t msr, uint32_t lo, uint32_t hi);
void sys_read_MSR(uint32_t msr, uint32_t *lo, uint32_t *hi);
