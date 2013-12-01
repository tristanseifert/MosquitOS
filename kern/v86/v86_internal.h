#ifndef V86_PVT_H
#define V86_PVT_H

#include <types.h>
#include "v86_monitor.h"
#include "v86_setup.h"

typedef struct {

} __attribute__((packed)) v86_regs;

typedef int (*v86m_swi_handler)(v86_regs* regs);

uint16_t v86_peekw(v86_address_t address);
void v86_pokeb(v86_address_t address, uint8_t value);
uint8_t v86_peekb(v86_address_t address);
void v86_pokew(v86_address_t address, uint16_t value);
#endif