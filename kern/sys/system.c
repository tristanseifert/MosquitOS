#include <types.h>
#include <io/io.h>
#include <device/pic.h>
#include <device/pit.h>

#include "system.h"

#define	IRQ_0			0x20	// IRQ0 = PIT timer tick

static void sys_set_lidt(void* base, uint16_t size);
uint64_t sys_timer_ticks;
cpu_info_t* sys_current_cpu_info;

// Builds IDT to system defaults
void sys_build_idt();
// Builds GDT to system defaults
void sys_build_gdt();
// Sets location of GDT
void sys_install_gdt(void* location);

// Define the assembly IRQ handlers
extern void sys_dummy_irq(void);
extern void sys_timer_tick_irq(void);
extern void sys_page_fault_irq(void);

// ISR/Exception handlers
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);

/*
 * Initialises the system into a known state.
 */ 
void system_init() {
	// TODO: Fix GDT!
	sys_build_gdt();
	sys_build_idt();
	sys_setup_ints();
}

/*
 * Builds the Interrupt Descriptor Table at a fixed location in memory.
 */
void sys_build_idt() {
	idt_entry_t* idt = (idt_entry_t *) SYS_IDT_MEMLOC;

	// Clear IDT
	memset(idt, 0, sizeof(idt_entry_t)*256);

	for(int i = 0; i < 256; i++) {
		sys_set_idt_gate(i, (uint32_t) sys_dummy_irq, 0x08, 0x8E);
	}

	// Set IDT gate for IRQ0
	sys_set_idt_gate(IRQ_0, (uint32_t) sys_timer_tick_irq, 0x08, 0x8E);

	// Install exception handlers
	sys_set_idt_gate(0, (uint32_t) isr0, 0x08, 0x8E);
	sys_set_idt_gate(1, (uint32_t) isr1, 0x08, 0x8E);
	sys_set_idt_gate(2, (uint32_t) isr2, 0x08, 0x8E);
	sys_set_idt_gate(3, (uint32_t) isr3, 0x08, 0x8E);
	sys_set_idt_gate(4, (uint32_t) isr4, 0x08, 0x8E);
	sys_set_idt_gate(5, (uint32_t) isr5, 0x08, 0x8E);
	sys_set_idt_gate(6, (uint32_t) isr6, 0x08, 0x8E);
	sys_set_idt_gate(7, (uint32_t) isr7, 0x08, 0x8E);
	sys_set_idt_gate(8, (uint32_t) isr8, 0x08, 0x8E);
	sys_set_idt_gate(9, (uint32_t) isr9, 0x08, 0x8E);
	sys_set_idt_gate(10, (uint32_t) isr10, 0x08, 0x8E);
	sys_set_idt_gate(11, (uint32_t) isr11, 0x08, 0x8E);
	sys_set_idt_gate(12, (uint32_t) isr12, 0x08, 0x8E);
	sys_set_idt_gate(13, (uint32_t) isr13, 0x08, 0x8E);
	sys_set_idt_gate(15, (uint32_t) isr15, 0x08, 0x8E);
	sys_set_idt_gate(16, (uint32_t) isr16, 0x08, 0x8E);
	sys_set_idt_gate(17, (uint32_t) isr17, 0x08, 0x8E);
	sys_set_idt_gate(18, (uint32_t) isr18, 0x08, 0x8E);

	// Page fault handler
	sys_set_idt_gate(14, (uint32_t) isr14, 0x08, 0x8E);

	// Install IDT (LIDT instruction)
	sys_set_lidt((void *) idt, sizeof(idt_entry_t)*256);
}

void sys_setup_ints() {
	// Remap PICs
	sys_pic_irq_remap(0x20, 0x28);

	// Unmask timer interrupt
	sys_pic_irq_clear_mask(0);

	// Initialise PIT ch0 to hardware int
	sys_pit_init(0, 0);
	//  time in ms = reload_value / (3579545 / 3) * 1000
	// Reload counter of 0x078, giving an int every 0.10057144134 ms
	sys_pit_set_reload(0, 0x078);

	sys_timer_ticks = 0;

	// Re-enable interrupts now
	__asm__("sti");
	//__asm__("int $0x20");
}

void sys_set_idt_gate(uint8_t entry, uint32_t function, uint8_t segment, uint8_t flags) {
	idt_entry_t *ptr = (idt_entry_t *) SYS_IDT_MEMLOC;

	ptr[entry].offset_1 = function & 0xFFFF;
	ptr[entry].offset_2 = (function >> 0x10) & 0xFFFF;
	ptr[entry].selector = segment;
	ptr[entry].flags = flags; // OR with 0x60 for user level
	ptr[entry].zero = 0x00;
}

/*
 * Builds the Global Descriptor Table with the proper code/data segments, and
 * space for some task state segment descriptors.
 */
void sys_build_gdt() {
	gdt_entry_t *gdt = (gdt_entry_t *) SYS_GDT_MEMLOC;

	// Set up null entry
	memset(gdt, 0x00, sizeof(gdt_entry_t));

	// Kernel code segment and data segments
	sys_set_gdt_gate(1, 0x00000000, 0xFFFFFFFF, 0x9A, 0xCF);
	sys_set_gdt_gate(2, 0x00000000, 0xFFFFFFFF, 0x92, 0xCF);
	sys_set_gdt_gate(3, 0x00000000, 0xFFFFFFFF, 0xFA, 0xCF);
	sys_set_gdt_gate(4, 0x00000000, 0xFFFFFFFF, 0xF2, 0xCF);

	// Create the correct number of TSS descriptors
	for(int i = 0; i < SYS_NUM_TSS; i++) {
		sys_set_gdt_gate(i+5, SYS_TSS_MEMLOC+(i*SYS_TSS_LEN), SYS_TSS_LEN, 0x89, 0x4F);
	}

	sys_install_gdt(gdt);
}

void sys_set_gdt_gate(uint16_t num, uint32_t base, uint32_t limit, uint8_t flags, uint8_t gran) {
	gdt_entry_t *gdt = (gdt_entry_t *) SYS_GDT_MEMLOC;	
	
	gdt[num].base_low = (base & 0xFFFF);
	gdt[num].base_middle = (base >> 16) & 0xFF;
	gdt[num].base_high = (base >> 24) & 0xFF;

	gdt[num].limit_low = (limit & 0xFFFF);
	gdt[num].granularity = (limit >> 16) & 0x0F;

	gdt[num].granularity |= gran & 0xF0;
	gdt[num].access = flags;
}

/*
 * Installs the GDT.
 */
void sys_install_gdt(void* location) {
	struct {
		uint16_t length;
		uint32_t base;
	} __attribute__((__packed__)) IDTR;
 
	IDTR.length = (0x18 + (SYS_NUM_TSS * 0x08))-1;
	IDTR.base = (uint32_t) location;
	__asm__ volatile("lgdt (%0)" : : "p"(&IDTR));
}

/*
 * Loads the IDTR register.
 */
static void sys_set_lidt(void* base, uint16_t size) {
	struct {
		uint16_t length;
		uint32_t base;
	} __attribute__((__packed__)) IDTR;
 
	IDTR.length = size;
	IDTR.base = (uint32_t) base;
	__asm__ volatile("lidt (%0)" : : "p"(&IDTR));
}

/*
 * Returns information about the current CPU that has been previously read,
 * or performs a read of said info if not available in memory.
 */
cpu_info_t* sys_get_cpu_info() {
	// Detect CPU if not cached
	if(!sys_current_cpu_info) {
		sys_current_cpu_info = cpuid_detect();
	}

	cpuid_set_strings(sys_current_cpu_info);

	return sys_current_cpu_info;
}

/*
 * Returns the number of system timer ticks since the kernel was started.
 */
uint64_t sys_get_ticks() {
	return sys_timer_ticks;
}

/*
 * Checks if interrupts are enabled.
 */
bool sys_irq_enabled() {
	int f;
	__asm__ volatile("pushf\n\t" "popl %0" : "=g"(f));
	return f & (1 << 9);
}

/*
 * Reads TSC (CPU timestamp counter)
 */
void sys_rdtsc(uint32_t* upper, uint32_t* lower) {
	__asm__ volatile("rdtsc" : "=a"(*lower), "=d"(*upper) );
}

/*
 * Flushes the MMU TLB for a given logical address.
 */
void sys_flush_tlb(uint32_t m) {
	__asm__ volatile("invlpg %0" : : "m"(m) : "memory");
}

/*
 * Reads an MSR register.
 */
void sys_read_MSR(uint32_t msr, uint32_t *lo, uint32_t *hi) {
	__asm__ volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

/*
 * Writes an MSR register.
 */ 
void sys_write_MSR(uint32_t msr, uint32_t lo, uint32_t hi) {
	__asm__ volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

/*
 * IRQ handler for the system tick.
 */
void sys_timer_tick_handler(void) {
	sys_pic_irq_eoi(0x00); // send int ack
	sys_timer_ticks++;
}

/*
 * Page fault handler
 */
void sys_page_fault_handler(uint32_t address) {

}