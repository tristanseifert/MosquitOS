#include <types.h>
#include "io.h"
#include "ps2.h"
#include "device/pic.h"
#include "sys/system.h"

#define PS2_DATA 0x60
#define PS2_CMD 0x64

// assembly IRQ handlers
extern void ps2_irq_ch1(void);
extern void ps2_irq_ch2(void);

uint16_t ps2_ch1_buffer_off;
uint8_t *ps2_ch1_buffer;
uint16_t ps2_ch2_buffer_off;
uint8_t *ps2_ch2_buffer;

static uint16_t ps2_device1_type;
static uint16_t ps2_device2_type;
static uint8_t ps2_config_byte;
static bool ps2_keyboard_ok;

/*
 * Waits for the PS2 controller to have data available
 */
static void ps2_wait_io() {
	// Wait for bit 0 (output buffer full) to become 1
	while(!(io_inb(PS2_CMD) & 0x01));
}
/*
 * Waits for the PS2 controller to be ready to accept more data
 */
static void ps2_wait_write_rdy() {
	// Wait for bit 1 (input buffer full) to become 0
	while(io_inb(PS2_CMD) & 0x02);
}

/*
 * Initialises the driver and associated PS2 hardware
 */
int ps2_init() {
	// Disable both channel's devices
	io_outb(PS2_CMD, 0xAD);
	ps2_wait_write_rdy();
	io_outb(PS2_CMD, 0xA7);

	// Clear receive buffer
	io_inb(PS2_DATA);

	// Read configuration byte
	io_outb(PS2_CMD, 0x20);
	ps2_wait_io();
	ps2_config_byte = io_inb(PS2_DATA);

	// Clear bits 0, 1, and 6 (no IRQ, no translation)
	ps2_config_byte &= 0xBC;

	// Write back the byte
	io_outb(PS2_CMD, 0x60);
	ps2_wait_write_rdy();
	io_outb(PS2_DATA, ps2_config_byte);

	// Perform controller self test
	io_outb(PS2_CMD, 0xAA);

	ps2_wait_io();

	uint32_t counter = 0x00;
	while(io_inb(PS2_DATA) != 0x55) {
		counter++;
		if(counter > PS2_INIT_TIMEOUT) return -1;
	}

	// Initialise second channel if existant
	if(ps2_config_byte & (1 << 5)) {
		io_outb(PS2_CMD, 0xA8);		
	}

	// Test if ports work
	io_outb(PS2_CMD, 0xAB);
	ps2_wait_io();
	if(io_inb(PS2_DATA) != 0x00) {
		return -2; // error out if port is broken
	}

	// Enable IRQs
	ps2_config_byte |= 0x03;

	io_outb(PS2_CMD, 0x60);
	ps2_wait_write_rdy();
	io_outb(PS2_DATA, ps2_config_byte);

	// Enable devices
	io_outb(PS2_CMD, 0xAE);
	ps2_wait_write_rdy();
	io_outb(PS2_CMD, 0xA8);
	ps2_wait_write_rdy();

	// Reset device 1 and discard ACK
	if(ps2_write(0xFF, false, true, false)) return -3;
	ps2_wait_io();
	uint8_t temp = io_inb(PS2_DATA);

	if(temp == 0xFA) {
		terminal_write_string("Keyboard test OK\n");
		ps2_keyboard_ok = true;

		// We will probably get 0xAA too
		ps2_wait_io();
		io_inb(PS2_DATA);
	} else if(temp == 0xFC) {
		ps2_keyboard_ok = false;
		terminal_write_string("Keyboard failure\n");
		return -16;
	}

	// Identify device
	ps2_device1_type = ps2_id_device(false);
	ps2_device2_type = ps2_id_device(true);

	// If we have a keyboard, initialise it
	if((ps2_device1_type & 0xFF00) == 0xAB00) {
		// Enable scanning
		if(ps2_write(0xF4, false, true, true)) return -4;

		// Set keycode set to set 1
		ps2_write(0xF0, false, true, false);
		ps2_write(0x01, false, true, true);
	}

	// Set up IRQ handlers and PIC
	sys_pic_irq_clear_mask(1);
	sys_pic_irq_clear_mask(12);

	// Set up IRQ handlers
	sys_set_idt_gate(IRQ_1, (uint32_t) ps2_irq_ch1, 0x08, 0x8E);
	sys_set_idt_gate(IRQ_12, (uint32_t) ps2_irq_ch2, 0x08, 0x8E);

	terminal_write_string("PS2 Cfg: 0x");
	terminal_write_byte(ps2_config_byte);

	terminal_write_string("\nPS2 Device 1 Type: 0x");
	terminal_write_word(ps2_device1_type);
	terminal_write_string("\nPS2 Device 2 Type: 0x");
	terminal_write_word(ps2_device2_type);

	// Set up buffers
	ps2_ch1_buffer_off = 0;
	ps2_ch2_buffer_off = 0;

	ps2_ch1_buffer = (uint8_t *) kmalloc(PS2_BUF_SIZE);
	ps2_ch2_buffer = (uint8_t *) kmalloc(PS2_BUF_SIZE);

	return 0;
}

/*
 * Identifies the device on the given port.
 */
uint16_t ps2_id_device(bool secondPort) {
	uint8_t temp;

	ps2_write(0xF5, secondPort, true, true); // disable scanning
	ps2_write(0xF2, secondPort, true, true); // Identify command

	ps2_wait_io();

	// Now, read identification data
	uint8_t identify_1, identify_2;
	identify_1 = io_inb(PS2_DATA);

	// The device *may* not send a second byte, so use timeout
	uint16_t timeout = 0;
	while(!(io_inb(PS2_CMD) & 0x01)) {
		timeout++;
		// We only got one byte, so return just that
		if(timeout > PS2_READ_TIMEOUT) return identify_1;
	}

	identify_2 = io_inb(PS2_DATA);

	return identify_1 << 8 | identify_2;
}

/*
 * IRQ handler for both PS2 devices
 */
void ps2_irq_handler(bool secondPort) {
	uint8_t readValue = io_inb(PS2_DATA);

	ps2_ch1_buffer_off = readValue;

	if(!secondPort) { // first port
		ps2_ch1_buffer[ps2_ch1_buffer_off++] = readValue;
	} else { // second port
		ps2_ch2_buffer[ps2_ch2_buffer_off++] = readValue;
	}

	sys_pic_irq_eoi(secondPort ? 0x0C : 0x01); // send int ack
}

/*
 * Write a byte to the PS2 device.
 * If shouldWait is set, the function will wait for the input buffer bit to
 * indicate it's empty before writing.
 */
int ps2_write(uint8_t value, bool secondPort, bool shouldWait, bool waitACK) {
	uint16_t timeout = 0;

	// Wait for controller to be ready to accept data or timeout
	if(shouldWait) {
		while(io_inb(PS2_CMD) & 0x02) {
			timeout++;

			if(timeout > PS2_READ_TIMEOUT) return -1;
		}
	}

	// Write data out
	if(!secondPort) {
		io_outb(PS2_DATA, value);
	} else {
		io_outb(PS2_CMD, 0xD4); // write port 2

		// Wait for controller to accept data
		timeout = 0;
		while(io_inb(PS2_CMD) & 0x02) {
			timeout++;

			if(timeout > PS2_READ_TIMEOUT) return -1;
		}

		io_outb(PS2_DATA, value);
	}

	// If desired, wait for ACK (0xFA) from device
	if(waitACK) {
		ps2_wait_io();

		uint8_t temp;
		while((temp = io_inb(PS2_DATA)) != 0xFA) {
			ps2_wait_io();
		}
	}

	return 0;
}

/*
 * Reads the oldest byte (start of buffer) from the buffer specific to the device,
 * or returns 0x8000 if there is nothing to read.
 */
uint16_t ps2_read(bool secondPort) {
	uint8_t value;

	return ps2_ch1_buffer_off;

	// Read a byte from the buffer, and shift every byte in the buffer forward.
	if(!secondPort) {
		// First, check if we have any data to read
		if(!ps2_ch1_buffer_off) return 0x8000;

		value = ps2_ch1_buffer[0];

		// Shift buffer
		for(int i = 0; i < PS2_BUF_SIZE; i++) {
			ps2_ch1_buffer[i] = ps2_ch1_buffer[i+1];
		}

		ps2_ch1_buffer_off--;
	} else {
		if(!ps2_ch2_buffer_off) return 0x8000;

		value = ps2_ch2_buffer[0];

		for(int i = 0; i < PS2_BUF_SIZE; i++) {
			ps2_ch2_buffer[i] = ps2_ch2_buffer[i+1];
		}

		ps2_ch2_buffer_off--;
	}

	return value;
}

/*
 * Set keyboard LEDs.
 */
void ps2_set_led(uint8_t ledState) {
	ps2_write(false, false, true, false); // LED SET command
	ps2_write(ledState, false, true, true); // LED state
}