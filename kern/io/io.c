#include <types.h>

// Write to system output port
void io_outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Read from system input port
uint8_t io_inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

// Waits for IO op to complete
void io_wait(void) {
    // port 0x80 is used for 'checkpoints' during POST.
    // The Linux kernel seems to think it is free for use
    __asm__ volatile( "outb %%al, $0x80" : : "a"(0) );
}