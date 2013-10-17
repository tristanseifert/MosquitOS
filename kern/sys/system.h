#include <types.h>

/*
 * This file (and the associated .c) contains most of the variables and code that sets
 * up the system into a defined state, and also activates and makes use of a lot of
 * the additional hardware in the PC.
 *
 * In addition, it can call other drivers (for example, PCI, VGA, etc) and initialise
 * those as well.
 */

#define SYS_IDT_MEMLOC 0x00100000
#define SYS_GDT_MEMLOC SYS_IDT_MEMLOC+(0x08*0x200)
#define SYS_TSS_MEMLOC SYS_GDT_MEMLOC+(0x18+(SYS_NUM_TSS * 0x08))

#define SYS_NUM_TSS 256
#define SYS_TSS_LEN	0x80

typedef struct __attribute__((__packed__)) sys_idt_descriptor {
   uint16_t offset_1;	// offset bits 0..15
   uint16_t selector;	// a code segment selector in GDT or LDT
   uint8_t zero;		// unused, set to 0
   uint8_t type_attr;	// type and attributes, see below
   uint16_t offset_2;	// offset bits 16..31
} sys_idt_descriptor;

typedef struct sys_gdt_descriptor {
	uint32_t limit;
	uint32_t base;
	uint8_t type;
} sys_gdt_descriptor;

void system_init();

void sys_build_idt();
void sys_install_idt(void* location);

void sys_build_gdt();
void sys_gdt_convert(uint8_t *target, sys_gdt_descriptor source);
void sys_install_gdt(void* location);

uint64_t sys_get_ticks();

bool sys_irq_enabled();

void sys_cpuid(uint32_t code, uint32_t* a, uint32_t* d);
void sys_rdtsc(uint32_t* upper, uint32_t* lower);
void sys_flush_tlb(void* m);
void sys_write_MSR(uint32_t msr, uint32_t lo, uint32_t hi);
void sys_read_MSR(uint32_t msr, uint32_t *lo, uint32_t *hi);
