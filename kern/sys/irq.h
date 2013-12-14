#ifndef IRQ_H
#define IRQ_H

#define MAX_IRQ 16

#include <types.h>

typedef void (*irq_t)(void*);

void irq_init(void);
bool irq_register(uint8_t number, irq_t function, void* context);

#endif