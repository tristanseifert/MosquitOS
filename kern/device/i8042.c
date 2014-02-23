#include <types.h>
#include "i8042.h"

#include <bus/platform.h>
#include <io/io.h>

#define PS2_DATA 0x60
#define PS2_CMD 0x64

// Function declarations
static int i8042_kbc_init(bus_t *platform_bus);
static int i8042_port_init(bool secondPort, dev_i8042_t *device);

// Driver definition
static platform_driver_t i8042_driver = {
	.d.name = "i8042 Keyboard Controller Driver",

	.supported_devices[0] = {
		.class = PLATFORM_CLASS_HID,
		.subclass = PLATFORM_SUBCLASS_KBC
	}
};

/*
 * Performs required initialisation.
 */
static int i8042_module_init(void) {
	bus_register_driver((driver_t *) &i8042_driver, BUS_NAME_PLATFORM);
	return 0;

	// return i8042_kbc_init(platform_bus);
}

module_init(i8042_module_init);

/*
 * Waits for the PS2 controller to have data available.
 */
static inline void i8042_wait_io() {
	while(!(io_inb(PS2_CMD) & 0x01));
}
/*
 * Waits for the PS2 controller to be ready to accept more data or a command.
 */
static inline void i8042_wait_write_rdy() {
	while(io_inb(PS2_CMD) & 0x02);
}

/*
 * Initialises a port on an i8042. This does not actually do much more than
 * enabling the port in the i8042, as device communication is handled by
 * individual device drivers, and we can reset these devices later when the
 * PS2 bus is enumerated.
 */
static int i8042_port_init(bool secondPort, dev_i8042_t *device) {
	if(!secondPort) {
		i8042_wait_write_rdy();
		io_outb(PS2_CMD, 0xAE);

		// Enable IRQs for this port
		device->config_byte |= 0x01;
	} else {
		i8042_wait_write_rdy();
		io_outb(PS2_CMD, 0xA8);

		device->config_byte |= 0x02;
	}

	// Write the config byte back to the PS2 controller.
	i8042_wait_write_rdy();
	io_outb(PS2_CMD, 0x60);
	i8042_wait_write_rdy();
	io_outb(PS2_DATA, device->config_byte);

	return 0;
}

/*
 * Initialises an i8042 KBC at the default system IO addresses, 0x60 and 0x64.
 */
static int i8042_kbc_init(bus_t *platform_bus) {
	uint8_t ps2_config, port_state;

	// Allocate memory for the PS2 controller device.
	dev_i8042_t *device = (dev_i8042_t *) kmalloc(sizeof(dev_i8042_t));
	memclr(device, sizeof(dev_i8042_t));

	// Disable devices on PS2 controller
	i8042_wait_write_rdy();
	io_outb(PS2_CMD, 0xAD);
	i8042_wait_write_rdy();
	io_outb(PS2_CMD, 0xA7);

	// Discard any bytes remaining in receive buffer, for example, from BIOS
	io_inb(PS2_DATA);

	// Read out the controller config byte
	i8042_wait_write_rdy();
	io_outb(PS2_CMD, 0x20);
	i8042_wait_io();
	ps2_config = io_inb(PS2_DATA);

	// Check if the controller has two ports
	device->isDualChannel = (ps2_config & 0x20) ? true : false;

	// Clear bits 0, 1, and 6 (no IRQ, no translation)
	ps2_config &= 0xBC;

	// Write the byte back to the PS2 controller
	i8042_wait_write_rdy();
	io_outb(PS2_CMD, 0x60);
	i8042_wait_write_rdy();
	io_outb(PS2_DATA, ps2_config);

	device->config_byte = ps2_config;

	// Send the "controller self test" command.
	io_outb(PS2_CMD, 0xAA);

	// Wait for the controller to reply back with 0x55.
	i8042_wait_io();
	if(io_inb(PS2_DATA) != 0x55) {
		errno = I8042_POST_FAILED;
		goto error;
	}

	// Some i8042's might be idiots and falsely report as single-channel
	if(!device->isDualChannel) {
		// Try to enable the second PS2 port.
		i8042_wait_write_rdy();
		io_outb(PS2_CMD, 0xA8);

		// Re-read config byte
		i8042_wait_write_rdy();
		io_outb(PS2_CMD, 0x20);
		i8042_wait_io();
		ps2_config = io_inb(PS2_DATA);

		// If bit 5 is clear, we DO have a dual-port controller
		device->isDualChannel = (ps2_config & 0x20) ? false : true;

		// Disable second port again.
		i8042_wait_write_rdy();
		io_outb(PS2_CMD, 0xA7);
	}

	// Test the ports.
	i8042_wait_write_rdy();
	io_outb(PS2_CMD, 0xAB);

	i8042_wait_io();
	port_state = io_inb(PS2_DATA);

	// If the port is good, port_state is 0. Otherwise, it contains an error.
	device->port_state[0] = port_state;

	// Only test the second port if it exists
	if(device->isDualChannel) {
		i8042_wait_write_rdy();
		io_outb(PS2_CMD, 0xA9);

		i8042_wait_io();
		port_state = io_inb(PS2_DATA);
		device->port_state[1] = port_state;
	}

	// Ensure we actually have at least one good port.
	if(device->port_state[0] != 0 && device->port_state[1] != 0) {
		errno = I8042_PORTS_FAILED;
		goto error;
	}

	// Initialise those ports that work.
	if(device->port_state[0] == 0) {
		errno = i8042_port_init(false, device);

		// If there was an error initialising the port, die.
		if(errno) {
			goto error;
		}
	}

	if(device->port_state[1] == 0) {
		errno = i8042_port_init(true, device);

		if(errno) {
			goto error;
		}
	}

	ps2_config = device->config_byte;

	// At this point, we can start enumerating the devices.

	return 0;

	error: ;
	kfree(device);
	return errno;
}