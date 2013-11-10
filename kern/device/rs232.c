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
static void rs232_shift_buffer(rs232_buffer_t* buffer, bool tx, size_t bytes);

extern void sys_rs232_irq_handler1(void);
extern void sys_rs232_irq_handler2(void);

static uint16_t rs232_to_io_map[4] = {0x3F8, 0x2F8, 0x3E8, 0x2E8};

static bool tx_fifo_free;

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

		rs232_buffer_ptrs[i].tx_buf = (uint8_t *) kmalloc(RS232_BUF_SIZE);
		rs232_buffer_ptrs[i].rx_buf = (uint8_t *) kmalloc(RS232_BUF_SIZE);

		rs232_buffer_ptrs[i].tx_buf_off = 0;
		rs232_buffer_ptrs[i].rx_buf_off = 0;

		io_outb(port + 1, 0x00);	// Disable all interrupts
		io_outb(port + 3, 0x80);	// Enable DLAB (set baud rate divisor)
		io_outb(port + 0, 0x01);	// Set divisor to 1 (lo byte) 115.2kbaud
		io_outb(port + 1, 0x00);	//					(hi byte)
		io_outb(port + 3, 0x03);	// 8 bits, no parity, one stop bit
		io_outb(port + 2, 0xC7);	// Enable FIFO, clear them, with 14-byte threshold
		io_outb(port + 4, 0x0F);	// IRQs enabled, RTS/DSR set
		io_outb(port + 1, 0x0F);	// Enable all interrupts
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
 * Up to 16 bytes of data are written directly to the port's FIFO, with the
 * remainder of data ending up in the TX buffer.
 */
void rs232_write(rs232_port_t port, size_t num_bytes, void* data) {
	volatile uint16_t portnum = rs232_to_io_map[port-1];

	uint8_t* data_read = data;
	static uint8_t counter;

	// If less than or equal to 16 total bytes, write directly
	if(num_bytes <= 16) {
		for(int i = 0; i < num_bytes; i++) {
			io_outb(portnum, *data_read++);
		}
	} else {
		for(int i = 0; i < 16; i++) {
			io_outb(portnum, *data_read++);
		}

		// Get buffer info struct
		rs232_buffer_t *bufInfo = &rs232_buffer_ptrs[port-1];

		// Loop through the rest of the data
		for(int i = 0; i < num_bytes-16; i++) {
			bufInfo->tx_buf[bufInfo->tx_buf_off++] = *data_read++;
		}
	}
}

/*
 * Writes a single character to the RS232 port.
 */
void rs232_putchar(rs232_port_t port, char value) {
	volatile uint16_t portnum = rs232_to_io_map[port-1];

	if(!(io_inb(portnum + 5) & 0x20)) {
		// Get buffer info struct
		rs232_buffer_t *bufInfo = &rs232_buffer_ptrs[port-1];

		// Stuff into buffer
		bufInfo->tx_buf[bufInfo->tx_buf_off++] = value;
	} else {
		io_outb(portnum, value);
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

process_irq: ; // gcc is stupid
	// Determine which port triggered it
	uint8_t triggered_port = 0;
	if((irq_port2 & 0x01)) triggered_port = 0;
	else if((irq_port1 & 0x01)) triggered_port = 1;
	else return; // none of the two ports wants to ack the interrupt

	// Shift the entire value right one bit, and get low 3 bits only
	uint8_t irq = ((triggered_port == 0) ? irq_port1 : irq_port2);
	uint16_t port = port_addr[portSet & 0x01][triggered_port];

	if(irq != 0xFF) {
		irq = (irq >> 0x01) & 0x07;
	} else {
		goto done;
	}

	// Get the struct
	rs232_buffer_t *port_info = &rs232_buffer_ptrs[portSet + (triggered_port << 1)];


	// Service appropriate IRQ
	switch(irq) {
		case 0: { // Modem bits have changed (Read MSR to service)
			port_info->delta_flags = io_inb(port + 6);
			break;
		}
		
		case 1: { // Transmitter FIFO empty (Write to THR/read IIR)
			// Check if we have any data in the TX buffer
			if(port_info->tx_buf_off == 0) break;

			int offset = port_info->tx_buf_off;
			uint8_t *data_read = port_info->tx_buf;

			// Write 14 bytes if more than 14 are pending
			int num_bytes_write = (offset > 14) ? 14 : offset;

			for(int i = 0; i < num_bytes_write; i++) {
				io_outb(port, *data_read++);
			}

			// Shift buffer
			rs232_shift_buffer(port_info, true, num_bytes_write);

			break;
		}
		
		case 2: { // RX FIFO threshold (14 bytes) reached (Read RBR to service)
			// TODO: replace with real stuff
			for(int i = 0; i < 16; i++) {
				io_inb(port);
			}
			break;
		}
		
		case 3: { // Status changed (Read LSR to service)
			port_info->line_status = io_inb(port + 5);
			break;
		}
		
		case 6: { // No RX act for 4 words, but data avail (Read RBR to service)

			break;
		}

		// well son you dun fucked up good if you get here
		default:
			PANIC("Got unrecognised RS232 interrupt");
			break;
	}

	// Re-read the IRQ states for both ports
	irq_port1 = io_inb(port_addr[portSet & 0x01][0]+2);
	irq_port2 = io_inb(port_addr[portSet & 0x01][1]+2);

	// If any of them do NOT have bit 0 clear, process IRQ again
	if(!(irq_port1 & 0x01)) goto process_irq;
	if(!(irq_port2 & 0x01)) goto process_irq;

	done: ;
	sys_pic_irq_eoi(3); // send int ack to PIC
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

	for(int i = 0; i < RS232_BUF_SIZE-bytes; i++) {
		buf[i] = buf[i+bytes];
	}
}