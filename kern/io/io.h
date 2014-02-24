/*
 * Header defining common IO routines that can easily be inlined
 */

#ifndef IO_H
#define IO_H
#include <types.h>

/*
 * Write a byte to system IO port
 */
static inline void io_outb(uint16_t port, uint8_t val) {
	__asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * Read a byte from a system IO port
 */
static inline uint8_t io_inb(uint16_t port) {
	uint8_t ret;
	__asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

/*
 * Write a word to system IO port
 */
static inline void io_outw(uint16_t port, uint16_t val) {
	__asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * Read a word from a system IO port
 */
static inline uint16_t io_inw(uint16_t port) {
	uint16_t ret;
	__asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

/*
 * Write a dword to system IO port
 */
static inline void io_outl(uint16_t port, uint32_t val) {
	__asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * Read a dword from a system IO port
 */
static inline uint32_t io_inl(uint16_t port) {
	uint32_t ret;
	__asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

/*
 * Wait for a system IO operation to complete.
 */
static inline void io_wait(void) {
	// port 0x80 is used for 'checkpoints' during POST.
	// The Linux kernel seems to think it is free for use
	__asm__ volatile("outb %%al, $0x80" : : "a"(0));
}
#endif