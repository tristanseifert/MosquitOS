#ifndef PS2_KBD_H
#define PS2_KBD_H

#include <types.h>

#define PS2_INIT_TIMEOUT 0x20000
#define PS2_READ_TIMEOUT 0x2000
#define PS2_BUF_SIZE 0x100

int ps2_init();

uint16_t ps2_id_device(bool secondPort);

void ps2_irq_handler(bool secondPort);

int ps2_write(uint8_t value, bool secondPort, bool shouldWait, bool waitACK);
uint16_t ps2_read(bool secondPort);

void ps2_set_led(uint8_t ledState);

#endif