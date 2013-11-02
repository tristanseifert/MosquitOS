#include <types.h>

#include "sys/system.h"
#include "io/io.h"
#include "rs232.h"

static void rs232_wait_write_avail(rs232_port_t port);
static void rs232_wait_read_avail(rs232_port_t port);

extern void sys_rs232_irq_handler(void);

static uint16_t rs232_to_io_map[4] = {0x3F8, 0x2F8, 0x3E8, 0x2E8};

/*
 * Initialises the ports.
 *
 * By default, the driver initialises ports at 115.2kBaud, with 8 bits per
 * symbol, no parity and one stop bit.
 */
void rs232_init() {
	static uint16_t port;

	for(uint8_t i = 0; i < 4; i++) {
		port = rs232_to_io_map[i];

		io_outb(port + 1, 0x00);    // Disable all interrupts
		io_outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
		io_outb(port + 0, 0x01);    // Set divisor to 1 (lo byte) 115.2kbaud
		io_outb(port + 1, 0x00);    //                  (hi byte)
		io_outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
		io_outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
		io_outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
	}

	// Set up IRQs
	sys_set_idt_gate(IRQ_3, (uint32_t) sys_rs232_irq_handler, 0x08, 0x8E);
	sys_set_idt_gate(IRQ_4, (uint32_t) sys_rs232_irq_handler, 0x08, 0x8E);
}

/*
 * Sets the baud rate on the specified RS232 port.
 */
void rs232_set_baud(rs232_port_t port, rs232_baud_t baudrate) {

}

/*
 * Writes num_bytes from data to the specified RS232 port
 *
 * To accelerate the write process, we only check for FIFO overflow after
 * writing 8 bytes instead of every byte. While this *may* cause data
 * loss in some cases, it should work reasonably well for both high and low
 * baud rates.
 */
void rs232_write(rs232_port_t port, size_t num_bytes, void* data) {
	volatile uint16_t portnum = rs232_to_io_map[port-1];

	uint8_t* data_read = data;
	static uint8_t counter;

	for(int i = 0; i < num_bytes; i++) {
		rs232_wait_write_avail(num_bytes);
		io_outb(portnum, *data_read++);

		// Check for FIFO overflow
		if((counter++) == 8) {
			rs232_wait_write_avail(port);
			counter = 0;
		}
	}
}

/*
 * Reads num_bytes from the RS232 port to out. Blocks until bytes are available,
 * or until the timeout expired if specified.
 */
void rs232_read(rs232_port_t port, size_t num_bytes, void* out, bool timeout) {

}

/*
 * Waits for a byte to become available on the RS232 port specified.
 */
static void rs232_wait_read_avail(rs232_port_t port) {
	volatile uint16_t portnum = rs232_to_io_map[port-1];
	while(!(io_inb(portnum + 5) & 1));
}

/*
 * Waits for the write FIFO to have at least one byte free.
 */
static void rs232_wait_write_avail(rs232_port_t port) {
	volatile uint16_t portnum = rs232_to_io_map[port-1];
	while(!(io_inb(portnum + 5) & 0x20));
}

/*
 * Gets the IO address of the specified RS232 port.
 */
uint16_t rs232_get_io_addr(rs232_port_t port) {
	return rs232_to_io_map[((int) port) - 1];
}

/*
 * RS232 IRQ handler
 */
void rs232_irq_handler(uint32_t portSet) {

}