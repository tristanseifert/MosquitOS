/*
 * Support for the Pentium 2's APIC (Advanced Programmable Interrupt Controller)
 * for interrupt handling in place of the old 8259 PIC and 8253/8254 PIT.
 */

#ifndef APIC_H
#define APIC_H

#include <types.h>

bool apic_supported();

#endif