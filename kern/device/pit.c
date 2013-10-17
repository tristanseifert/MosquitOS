#include <types.h>
#include <io/io.h>
#include "pit.h"

#define PIT_CH0		0x40
#define PIT_CH1		0x41
#define PIT_CH2		0x42
#define PIT_CTRL	0x43

/*
 * Initialises a channel.
 */
void sys_pit_init(uint8_t channel, uint8_t mode) {
	uint8_t mask = 0x34;

	mask |= (channel & 0x03) << 0x06;
	mask |= (mode & 0x07) << 0x01;

	io_outb(PIT_CTRL, mask); // write out control byte
}

/*
 * Sets the count for the specific channel.
 */
void sys_pit_set_reload(uint8_t ch, uint16_t count) {
	io_outb(PIT_CH0, (count & 0x00FF)); // write low byte
	io_outb(PIT_CH0, (count & 0xFF) >> 0x08); // write high byte
}
