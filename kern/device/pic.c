#include <types.h>
#include <io/io.h>
#include "pic.h"

#define PIC1			0x20	/* IO base address for master PIC */
#define PIC2			0xA0	/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA		(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA		(PIC2+1)

#define PIC_EOI			0x20	/* End-of-interrupt command code */

#define ICW1_ICW4		0x01	/* ICW4 (not) needed */
#define ICW1_SINGLE		0x02	/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04	/* Call address interval 4 (8) */
#define ICW1_LEVEL		0x08	/* Level triggered (edge) mode */
#define ICW1_INIT		0x10	/* Initialization - required! */
 
#define ICW4_8086		0x01	/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO		0x02	/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08	/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C	/* Buffered mode/master */
#define ICW4_SFNM		0x10	/* Special fully nested (not) */

#define PIC_READ_IRR	0x0A	/* OCW3 irq ready next CMD read */
#define PIC_READ_ISR	0x0B	/* OCW3 irq service next CMD read */

/*
 * Masks or unmasks an interrupt on the PIC.
 */
void sys_pic_irq_set_mask(uint8_t IRQline) {
	uint16_t port;
	uint8_t value;
 
	if(IRQline < 8) {
		port = PIC1_DATA;
	} else {
		port = PIC2_DATA;
		IRQline -= 8;
	}

	value = io_inb(port) | (1 << IRQline);
	io_outb(port, value);		
}
 
void sys_pic_irq_clear_mask(uint8_t IRQline) {
	uint16_t port;
	uint8_t value;
 
	if(IRQline < 8) {
		port = PIC1_DATA;
	} else {
		port = PIC2_DATA;
		IRQline -= 8;
	}

	value = io_inb(port) & ~(1 << IRQline);
	io_outb(port, value);		
}

/*
 * Remaps the PIC interrupt offsets
 */
void sys_pic_irq_remap(uint8_t offset1, uint8_t offset2) {
	uint8_t a1, a2;
 
	a1 = io_inb(PIC1_DATA); // save masks
	a2 = io_inb(PIC2_DATA);
 
	io_outb(PIC1_COMMAND, ICW1_INIT+ICW1_ICW4); // starts the initialization sequence (in cascade mode)
	io_wait();
	io_outb(PIC2_COMMAND, ICW1_INIT+ICW1_ICW4);
	io_wait();
	io_outb(PIC1_DATA, offset1); // ICW2: Master PIC vector offset
	io_wait();
	io_outb(PIC2_DATA, offset2); // ICW2: Slave PIC vector offset
	io_wait();
	io_outb(PIC1_DATA, 4); // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_wait();
	io_outb(PIC2_DATA, 2); // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();
 
	io_outb(PIC1_DATA, ICW4_8086);
	io_wait();
	io_outb(PIC2_DATA, ICW4_8086);
	io_wait();
 
	io_outb(PIC1_DATA, a1); // restore saved masks.
	io_outb(PIC2_DATA, a2);
}

/*
 * Helper method to read the IRQ registers
 */
static inline uint16_t pic_get_irq_reg(uint8_t ocw3) {
	/* OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
	 * represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain */
	io_outb(PIC1_COMMAND, ocw3);
	io_outb(PIC2_COMMAND, ocw3);
	io_wait();

	return (io_inb(PIC1_COMMAND) << 8) | io_inb(PIC2_COMMAND);
}
 
/*
 * Reads cascaded PIC interrupt request register,
 */
uint16_t sys_pic_irq_get_irr() {
	return pic_get_irq_reg(PIC_READ_IRR);
}

/*
 * Reads cascaded PIC in-service register,
 */
uint16_t sys_pic_irq_get_isr() {
	return pic_get_irq_reg(PIC_READ_ISR);
}

/*
 * Sends the End of Interrupt command to the PIC
 */
void sys_pic_irq_eoi(uint8_t irq) {
	if(irq >= 8) {
		io_outb(PIC2_COMMAND,PIC_EOI);
	}
 
	io_outb(PIC1_COMMAND,PIC_EOI);
}

/*
 * Disables the PICs.
 */
void sys_pic_irq_disable() {
	io_outb(PIC1_DATA, 0xFF);
	io_wait();
	io_outb(PIC2_DATA, 0xFF);
	io_wait();
}