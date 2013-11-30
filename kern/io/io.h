#include <types.h>

uint8_t io_inb(uint16_t port);
void io_outb(uint16_t port, uint8_t val);
uint16_t io_inw(uint16_t port);
void io_outw(uint16_t port, uint16_t val);
uint32_t io_inl(uint16_t port);
void io_outl(uint16_t port, uint32_t val);
void io_wait(void);
