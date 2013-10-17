#include <types.h>
#include <io/io.h>
#include <device/pic.h>
#include <device/pit.h>
#include "system.h"

#define	IRQ_0			0x20	// IRQ0 = PIT timer tick

static inline void sys_set_lidt(void* base, uint16_t size);
static uint64_t sys_timer_ticks;

// Define the assembly IRQ handlers
void sys_dummy_irq(void);
void sys_timer_tick_irq(void);

/*
 * Initialises the system into a known state.
 */ 
void system_init() {
	sys_build_gdt();
	sys_build_idt();
}

/*
 * Builds the Interrupt Descriptor Table at a fixed location in memory.
 */
void sys_build_idt() {
	sys_idt_descriptor* idt = (sys_idt_descriptor *) SYS_IDT_MEMLOC;

	// Fill the IDT with dummy 32-bit interrupt handlers
	for(uint16_t i = 0; i < 256; i++) {
		idt[i].zero = 0x00;
		idt[i].selector = 0x08; // Kernel code selector
		idt[i].offset_1 = (((uint32_t) sys_dummy_irq) & 0xFFFF); // ISR logical location
		idt[i].offset_2 = (((uint32_t) sys_dummy_irq) & 0xFFFF0000) >> 0x10; // ISR logical location
		idt[i].type_attr = 0x8E; // Present, 32-bit interrupt

	}

	// 0x000830f0 0x00008e00
	// Set up IRQ0
	idt[IRQ_0].offset_1 = (((uint32_t) sys_timer_tick_irq) & 0xFFFF); // ISR logical location
	idt[IRQ_0].offset_2 = (((uint32_t) sys_timer_tick_irq) & 0xFFFF0000) >> 0x10; // ISR logical location

	sys_install_idt((void *) idt);

	// Remap PICs
	sys_pic_irq_remap(0x20, 0x28);

	// Unmask timer interrupt
	sys_pic_irq_clear_mask(0);

	// Initialise PIT ch0 to hardware int
	sys_pit_init(0, 0);
	//  time in ms = reload_value / (3579545 / 3) * 1000
	// Reload counter of 0x4AA, giving an int every 1.00068584136 ms
	sys_pit_set_reload(0, 0x4AA);

	// Re-enable interrupts now
	//__asm__("sti");
	__asm__("int $20");
}

/*
 * Installs the Interrupt Descriptor Table.
 */
void sys_install_idt(void* location) {
	sys_set_lidt(location, 0x800);
}

/*
 * Builds the Global Descriptor Table with the proper code/data segments, and
 * space for some task state segment descriptors.
 */
void sys_build_gdt() {
	uint8_t *gdt = (uint8_t *) SYS_GDT_MEMLOC;

	// Set up null entry
	memset(gdt, 0x00, 0x08);

	uint8_t *gdt_write_ptr = gdt+8;

	// Code segment
	sys_gdt_convert(gdt_write_ptr, (sys_gdt_descriptor) {0xFFFFFFFF, 0x00000000, 0x9A});
	gdt_write_ptr += 0x08;

	// Data segment
	sys_gdt_convert(gdt_write_ptr, (sys_gdt_descriptor) {0xFFFFFFFF, 0x00000000, 0x92});

	// Create the correct number of TSS descriptors
	for(int i = 0; i < SYS_NUM_TSS; i++) {
		sys_gdt_convert(gdt_write_ptr, (sys_gdt_descriptor) 
			{SYS_TSS_LEN, SYS_TSS_MEMLOC+(i*SYS_TSS_LEN), 0x89});		
	}

	sys_install_gdt(gdt);
}

/*
 * Turns a sys_gdt_descriptor struct into an actual GDT descriptor.
 */
void sys_gdt_convert(uint8_t *target, sys_gdt_descriptor source) {
    // Check the limit to make sure that it can be encoded
    if ((source.limit > 65536) && (source.limit & 0xFFF) != 0xFFF) {
        // error out here
    }

    if (source.limit > 65536) {
        // Adjust granularity if required
        source.limit = source.limit >> 12;
        target[6] = 0xC0;
    } else {
        target[6] = 0x40;
    }
 
    // Encode the limit
    target[0] = source.limit & 0xFF;
    target[1] = (source.limit >> 8) & 0xFF;
    target[6] |= (source.limit >> 16) & 0xF;
 
    // Encode the base 
    target[2] = source.base & 0xFF;
    target[3] = (source.base >> 8) & 0xFF;
    target[4] = (source.base >> 16) & 0xFF;
    target[7] = (source.base >> 24) & 0xFF;
 
    // And... Type
    target[5] = source.type;
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
	__asm__("lgdt (%0)" : : "p"(&IDTR));
}

/*
 * Loads the IDTR register.
 */
static inline void sys_set_lidt(void* base, uint16_t size) {
	struct {
		uint16_t length;
		uint32_t base;
	} __attribute__((__packed__)) IDTR;
 
	IDTR.length = size;
	IDTR.base = (uint32_t) base;
	__asm__("lidt (%0)" : : "p"(&IDTR));
}

/*
 * Returns the number of system timer ticks since the kernel was started.
 */
inline uint64_t sys_get_ticks() {
	return sys_timer_ticks;
}

/*
 * Checks if interrupts are enabled.
 */
inline bool sys_irq_enabled() {
	int f;
	__asm__ volatile ("pushf\n\t" "popl %0" : "=g"(f));
	return f & (1 << 9);
}

/*
 * Reads CPUID register
 */
inline void sys_cpuid(uint32_t code, uint32_t* a, uint32_t* d) {
	__asm__ volatile("cpuid" : "=a"(*a), "=d"(*d) : "0"(code) : "ebx", "ecx");
}

/*
 * Reads TSC (CPU timestamp counter)
 */
inline void sys_rdtsc(uint32_t* upper, uint32_t* lower) {
	__asm__ volatile("rdtsc" : "=a"(*lower), "=d"(*upper) );
}

/*
 * Flushes the MMU TLB for a given logical address.
 */
inline void sys_flush_tlb(void* m) {
	__asm__ volatile("invlpg %0" : : "m"(*m) : "memory");
}

/*
 * Reads an MSR register.
 */
inline void sys_read_MSR(uint32_t msr, uint32_t *lo, uint32_t *hi) {
	__asm__ volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

/*
 * Writes an MSR register.
 */ 
inline void sys_write_MSR(uint32_t msr, uint32_t lo, uint32_t hi) {
	__asm__ volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

/*
 * IRQ handler for the system tick.
 */
void sys_timer_tick_handler(void) {
	sys_pic_irq_eoi(0x00); // sent int ack

	sys_timer_ticks++;
}