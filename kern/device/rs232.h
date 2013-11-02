#ifndef RS232_H
#define RS232_H

#include <types.h>

/*
 * RS232 driver
 */

typedef enum {
	kRS232_Null = 0,
	kRS232_COM1 = 1,
	kRS232_COM2,
	kRS232_COM3,
	kRS232_COM4,
} rs232_port_t;

typedef enum {
	kRS232_Baud115200 = 1,
	kRS232_Baud57600 = 2,
	kRS232_Baud38400 = 3,
	kRS232_Baud28800 = 4,
	kRS232_Baud19200 = 6,
	kRS232_Baud14400 = 8,
	kRS232_Baud9600 = 12,
	kRS232_Baud4800 = 24,
	kRS232_Baud2400 = 48,
	kRS232_Baud1200 = 96,
	kRS232_Baud300 = 384,
} rs232_baud_t;

void rs232_init();
void rs232_set_baud(rs232_port_t port, rs232_baud_t baudrate);
void rs232_write(rs232_port_t port, size_t num_bytes, void* data);
void rs232_read(rs232_port_t port, size_t num_bytes, void* out, bool timeout);

uint16_t rs232_get_io_addr(rs232_port_t port);

#endif