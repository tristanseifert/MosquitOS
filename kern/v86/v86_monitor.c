#include "v86_monitor.h"
#include "v86_setup.h"
#include "v86_internal.h"
#include <sys/cpuid.h>
#include <types.h>

static int v86m_handler_exit(v86_regs *registers);

static v86m_swi_handler v86m_swi_special[8] = {
	v86m_handler_exit
};

/* 
 * Pops a 16-bit value from the VM stack.
 */
uint16_t v86m_pop16(v86_regs *registers) {

}

/*
 * Pushes a 16-bit value onto the VM stack.
 */
void v86m_push16(v86_regs *registers, uint16_t value) {

}

/*
 * Performs a softare interrupt in the virtual machine. Note that executing any
 * software interrupt above 0xF8 actually causes the monitor to subtract 0xF8
 * from the interrupt number and use it as an offset into the v86m_swi_special
 * call table.
 */
void v86m_int(v86_regs *registers, uint8_t interrupt) {
	if(interrupt >= 0xF8) { // Handle specially
		int status = (v86m_swi_special[interrupt-0xF8](registers));
	} else { // Handle as normal

	}
}

/*
 * Terminates the Virtual 8086 code.
 */
static int v86m_handler_exit(v86_regs *registers) {
	return 0;
}

/*
 * Returns from an interrupt request.
 */
void v86m_iret(v86_regs *registers) {

}