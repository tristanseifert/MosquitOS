#include <types.h>

#include <sys/system.h>
#include <device/pic.h>
#include <io/io.h>
#include "rs232.h"

typedef struct {
	int tx_buf_off;
	uint8_t *tx_buf;

	int rx_buf_off;
	uint8_t *rx_buf;

	uint8_t delta_flags;
	uint8_t line_status;
} rs232_buffer_t;

static void rs232_wait_write_avail(rs232_port_t port);
static void rs232_wait_read_avail(rs232_port_t port);

extern void sys_rs232_irq_handler1(void);
extern void sys_rs232_irq_handler2(void);

static uint16_t rs232_to_io_map[4] = {0x3F8, 0x2F8, 0x3E8, 0x2E8};

// Structs describing each port's buffers
static rs232_buffer_t rs232_buffer_ptrs[4];

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

		io_outb(port + 1, 0x00);	// Disable all interrupts
		io_outb(port + 3, 0x80);	// Enable DLAB (set baud rate divisor)
		io_outb(port + 0, 0x01);	// Set divisor to 1 (lo byte) 115.2kbaud
		io_outb(port + 1, 0x00);	//					(hi byte)
		io_outb(port + 3, 0x03);	// 8 bits, no parity, one stop bit
		io_outb(port + 2, 0xC7);	// Enable FIFO, clear them, with 14-byte threshold
		io_outb(port + 4, 0x0B);	// IRQs enabled, RTS/DSR set
		io_outb(port + 1, 0x03);	// Enable RX threshold and TX empty interrupts

		rs232_buffer_ptrs[i].tx_buf = (uint8_t *) kmalloc(RS232_BUF_SIZE);
		rs232_buffer_ptrs[i].rx_buf = (uint8_t *) kmalloc(RS232_BUF_SIZE);

		rs232_buffer_ptrs[i].tx_buf_off = 0;
		rs232_buffer_ptrs[i].rx_buf_off = 0;
	}
}

/*
 * Sets up IRQs once IDT is set up
 */
void rs232_set_up_irq() {
	// Set up IRQs
	sys_set_idt_gate(IRQ_3, (uint32_t) sys_rs232_irq_handler1, 0x08, 0x8E); // COM 2, 4
	sys_set_idt_gate(IRQ_4, (uint32_t) sys_rs232_irq_handler2, 0x08, 0x8E); // COM 1, 3

	sys_pic_irq_clear_mask(3);
	sys_pic_irq_clear_mask(4);
}

/*
 * Sets the baud rate on the specified RS232 port.
 */
void rs232_set_baud(rs232_port_t port, rs232_baud_t baudrate) {
	volatile uint16_t portnum = rs232_to_io_map[port-1];

	uint16_t divisor = (uint16_t) baudrate;

	io_outb(portnum + 3, 0x80); // Enable DLAB (set baud rate divisor)
	io_outb(portnum + 0, (divisor & 0xFF)); // Set divisor (lo byte) 115.2kbaud
	io_outb(portnum + 1, (divisor >> 0x08) & 0xFF);    // (hi byte)
	io_outb(portnum + 3, 0x03); // 8 bits, no parity, one stop bit (disable DLAB)

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
 *
 * Returns: Number of bytes actually read
 */
int rs232_read(rs232_port_t port, size_t num_bytes, void* out, bool timeout) {
	volatile uint16_t portnum = rs232_to_io_map[port-1];

	uint8_t* data_write = out;
	static uint8_t counter;
	static int timeout_counter;

	for(int i = 0; i < num_bytes; i++) {
		while(!(io_inb(portnum + 5) & 1)) {
			if(timeout) timeout_counter++;

			if(timeout_counter > RS232_READ_TIMEOUT) return i;
		}

		data_write[i] = io_inb(portnum);

		timeout_counter = 0;
	}

	return num_bytes;
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
 * Returns some information about the specified port. 
 */
rs232_port_info_t rs232_get_port_info(rs232_port_t port) {
	rs232_port_info_t info;
	rs232_buffer_t buffer = rs232_buffer_ptrs[port-1];

	info.tx_buf_off = buffer.tx_buf_off;
	info.rx_buf_off = buffer.rx_buf_off;
	info.delta_flags = buffer.delta_flags;
	info.line_status = buffer.line_status;
	info.io_port = rs232_to_io_map[port-1];

	return info;
}

/*
 * Gets the IO address of the specified RS232 port.
 */
uint16_t rs232_get_io_addr(rs232_port_t port) {
	return rs232_to_io_map[((int) port) - 1];
}

/*
 * RS232 IRQ handler
 *
 * portSet is a parameter passed by the assembly ISR wrapper: It's set to 1 for ports
 * COM2 and 4, and set to 0 for COM1 and 3.
 */
void rs232_irq_handler(uint32_t portSet) {
	uint16_t port_addr[2][2] = {
		{rs232_to_io_map[0], rs232_to_io_map[2]},
		{rs232_to_io_map[1], rs232_to_io_map[3]}
	};

	// Read IRQ registers
	uint8_t irq_port1 = io_inb(port_addr[portSet & 0x01][0]+2);
	uint8_t irq_port2 = io_inb(port_addr[portSet & 0x01][1]+2);

	// Determine which port triggered it
	uint8_t triggered_port = 0;
	if((irq_port2 & 0x01)) triggered_port = 0;
	else if((irq_port1 & 0x01)) triggered_port = 1;
	else return; // none of the two ports wants to ack the interrupt

	// Shift the entire value right one bit, and get low 3 bits only
	// We could use the high two bits to determine if FIFOs are on, but without
	// them we wouldn't have a working driver
	uint8_t irq = (((triggered_port == 0) ? irq_port1 : irq_port2) >> 0x01) & 0x07;
	uint16_t port = port_addr[portSet & 0x03][triggered_port];

	// Get the struct
	rs232_buffer_t *port_info = &rs232_buffer_ptrs[portSet + (triggered_port << 1)];

	terminal_write_string("IRQ : 0x");
	terminal_write_byte(irq);

	// Service appropriate IRQ
	switch(irq) {
		case 0: { // Modem bits have changed
			port_info->delta_flags = io_inb(port + 6);
			break;
		}
		
		case 1: { // Transmitter FIFO empty

			break;
		}
		
		case 2: { // RX FIFO threshold (14 bytes) reached or Data Ready

			break;
		}
		
		case 3: { // Status changed
			port_info->line_status = io_inb(port + 5);
			break;
		}
		
		case 6: { // No RX FIFO for 4 words, but data is available

			break;
		}

		// well son you dun fucked up good if you get here
		default:
			PANIC("Got unrecognised RS232 interrupt");
			break;
	}
}

/*
 * Shifts the read or write buffer of the buffer structure by the specified
 * number of bytes.
 * 
 * Note that this discards the bytes at the *start* of the buffer, as they
 * are the oldest: the buffers work as FIFOs.
 */
static void rs232_shift_buffer(rs232_buffer_t* buffer, bool tx, size_t bytes) {
	uint8_t *buf = tx ? buffer->tx_buf : buffer->rx_buf;
}