#include <types.h>
#include "irq.h"
#include "system.h"
#include "runtime/list.h"
#include "runtime/hashmap.h"
#include "device/pic.h"

// Variable written to by IRQ handlers to indicate last IRQ
uint32_t irq_last_request_num;

// Definitions of assembly IRQ handlers.
extern void irq_0(void);
extern void irq_1(void);
extern void irq_2(void);
extern void irq_3(void);
extern void irq_4(void);
extern void irq_5(void);
extern void irq_6(void);
extern void irq_7(void);
extern void irq_8(void);
extern void irq_9(void);
extern void irq_10(void);
extern void irq_11(void);
extern void irq_12(void);
extern void irq_13(void);
extern void irq_14(void);
extern void irq_15(void);

// Type inserted into IRQ handler list.
typedef struct {
	irq_t function;
	void* context;
} irq_handler_t;

// Holds lists with pointers to IRQ handlers.
static list_t* irqHandlerList[MAX_IRQ];

// Pointers to assembly IRQ handlers.
static void* irq_handlers[MAX_IRQ] = {
	irq_0, irq_1, irq_2, irq_3, irq_4, irq_5, irq_6, irq_7,
	irq_8, irq_9, irq_10, irq_11, irq_12, irq_13, irq_14, irq_15
};

/*
 * This IRQ handler is called by assembly routines, with the IRQ number is
 * placed in %eax.
 */
void irq_handler() {
	uint32_t number = irq_last_request_num;
	ASSERT(number < MAX_IRQ);
	irq_handler_t *handler;

	// Run all registered IRQ handlers
	for(int i = 0; i < irqHandlerList[number]->num_entries; i++) {
		handler = (irq_handler_t *) list_get(irqHandlerList[number], i);
		handler->function(handler->context);
	}

	// Now, acknowledge the interrupt.
	sys_pic_irq_eoi(number);
}

/*
 * Initialises the IRQ handler system.
 */
void irq_init(void) {
	for(int i = 0; i < MAX_IRQ; i++) {
		irqHandlerList[i] = list_allocate();
	}

	// Install IRQ handlers
	for(int i = 0; i < MAX_IRQ; i++) {
		sys_set_idt_gate(IRQ_0+i, (uint32_t) irq_handlers[i], 0x08, 0x8E);
	}
}

/*
 * Searches if the IRQ handler is already registered for the specified IRQ
 * number.
 */
static bool irq_is_registered(uint8_t number, irq_t function, void* context) {
	ASSERT(number < MAX_IRQ);

	irq_handler_t *handler;

	for(int i = 0; i < irqHandlerList[number]->num_entries; i++) {
		handler = (irq_handler_t *) list_get(irqHandlerList[number], i);

		if(handler->function == function && handler->context == context) {
			return true;
		}
	}

	return false;
}

/*
 * Registers an IRQ handler for the specified IRQ number, calling function
 * with context as the single argument passed to it.
 */
bool irq_register(uint8_t number, irq_t function, void* context) {
	ASSERT(number < MAX_IRQ);

	if(!irq_is_registered(number, function, context)) {
		irq_handler_t *handler = (irq_handler_t *) kmalloc(sizeof(irq_handler_t));
		memclr(handler, sizeof(irq_handler_t));

		handler->context = context;
		handler->function = function;

		list_add(irqHandlerList[number], handler);
	} else {
		kprintf("Already registered function 0x%X for IRQ %u\n", function, number);
		return false;
	}

	// Unmask the IRQ
	sys_pic_irq_clear_mask(number);

	return true;
}