#include <types.h>

uint8_t io_inb(uint16_t port);
void io_outb(uint16_t port, uint8_t val);
void io_wait(void);
