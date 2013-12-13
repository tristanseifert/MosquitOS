/*
 * Exposes devices on the i8042 keyboard controller (PS2) on a device named
 * ps_2, with the attached peripherals as its children.
 *
 * Also takes care to load drivers for the appropriate devices by enummerating
 * the PS2 bus when initialised.
 *
 * All communication with devices on the PS2 bus will be IRQ based, at least
 * for reception of packets. As the i8042 cannot provide an interrupt when its
 * transmit buffer is empty, this driver keeps an internal ring buffer that is
 * pushed to the i8042 through the use of a timer.
 */
#ifndef I8042_H
#define I8042_H

#include <types.h>
#include <bus/bus.h>

#define I8042_POST_FAILED -2
#define I8042_PORTS_FAILED -3

typedef struct i8042 dev_i8042_t;

struct i8042 {
	device_t d;

	uint8_t config_byte;
	bool isDualChannel;

	uint32_t port_state[2];
};

#endif