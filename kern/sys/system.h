#include <types.h>

#include "cpuid.h"

/*
 * This file (and the associated .c) contains most of the variables and code that sets
 * up the system into a defined state, and also activates and makes use of a lot of
 * the additional hardware in the PC.
 *
 * In addition, it can call other drivers (for example, PCI, VGA, etc) and initialise
 * those as well.
 */

#define SYS_KERN_CODE_SEG 0x08
#define SYS_KERN_DATA_SEG 0x10
#define SYS_USER_CODE_SEG 0x18
#define SYS_USER_DATA_SEG 0x20

#define SYS_KERN_STACK_SIZE 1024*32

#define SYS_IDT_MEMLOC 0x00550000
#define SYS_GDT_MEMLOC 0x00560000
#define SYS_TSS_MEMLOC 0x00570000

#define SYS_MSR_IA32_SYSENTER_CS 0x174
#define SYS_MSR_IA32_SYSENTER_ESP 0x175
#define SYS_MSR_IA32_SYSENTER_EIP 0x176

#define SYS_KERN_SYSCALL_STACK 0x400000

// We need 2 TSS per privilege level
#define SYS_NUM_TSS_PER_PRIV 2
#define SYS_NUM_TSS SYS_NUM_TSS_PER_PRIV*4
#define SYS_TSS_LEN	0x100

#define	IRQ_0			0x20	// IRQ0 = PIT timer tick
#define	IRQ_1			0x21	// IRQ1 = PS2 channel 1
#define	IRQ_2			0x22
#define	IRQ_3			0x23	// COM 2 or 4
#define	IRQ_4			0x24	// COM 1 or 3
#define	IRQ_5			0x25
#define	IRQ_6			0x26
#define	IRQ_7			0x27
#define	IRQ_8			0x28
#define	IRQ_9			0x29
#define	IRQ_10			0x2A
#define	IRQ_11			0x2B
#define	IRQ_12			0x2C	// IRQ12 = PS2 channel 2
#define	IRQ_13			0x2D
#define	IRQ_14			0x2E
#define	IRQ_15			0x2F

#define SYS_KERN_INFO_LOC 0x001600

// Struct encapsulating the list the BIOS 0xE820 mode call gave us
typedef struct sys_smap_entry {
	uint32_t baseL; // base address QWORD
	uint32_t baseH;
	uint32_t lengthL; // length QWORD
	uint32_t lengthH;
	uint32_t type; // entry Ttpe
	uint32_t ACPI; // exteded
} __attribute__((packed)) sys_smap_entry_t;

// Located at $1600 in physical memory by our bootloader
typedef struct sys_kern_info {
	uint32_t munchieValue; // Should be "KERN"
	uint16_t supportBits;
	uint16_t high16Mem; // 64K blocks above 16M
	uint16_t low16Mem; // 1k blocks below 16M
	sys_smap_entry_t* memMap; // 32-bit ptr to list
	uint16_t numMemMapEnt; // Number of entries in above map
	uint8_t vesaSupport;
	uint8_t bootDrive;
	uint32_t vesaMap;
} __attribute__((packed)) sys_kern_info_t;

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

typedef struct sys_tss {
	// High word ignored
	uint32_t backlink;

	// All 32 bits significant for ESP, high word ignored for SS
	uint32_t esp0;
	uint32_t ss0;
	uint32_t esp1;
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;

	// All 32 bits are significant
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;

	// High word is ignored in all these
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;

	uint16_t trap;
	uint16_t iomap; // low word ignored
} __attribute__((packed)) i386_thread_state_t;

void system_init();

sys_kern_info_t* sys_get_kern_info();

void sys_set_idt_gate(uint8_t entry, uint32_t function, uint8_t segment, uint8_t flags);
void sys_setup_ints();
bool sys_irq_enabled();

void sys_set_gdt_gate(uint16_t num, uint32_t base, uint32_t limit, uint8_t flags, uint8_t gran);
void sys_init_tss();

cpu_info_t* sys_get_cpu_info();

uint64_t sys_get_ticks();

void sys_rdtsc(uint32_t* upper, uint32_t* lower);
void sys_flush_tlb(uint32_t m);
void sys_write_MSR(uint32_t msr, uint32_t lo, uint32_t hi);
void sys_read_MSR(uint32_t msr, uint32_t *lo, uint32_t *hi);
