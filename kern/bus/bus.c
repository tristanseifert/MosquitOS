#include <types.h>
#include "runtime/hashmap.h"
#include "bus.h"

static list_t *bus_list;

/*
 * The parent of all busses.
 */
static node_t root = {
	.name = "/",
	.parent = NULL
};

/*
 * Initialises the bus subsystem.
 */
static int bus_sys_init(void) {
	bus_list = list_allocate();
	root.children = list_allocate();
	kprintf("Bus subsystem initialised\n");

	return 0;
}

module_early_init(bus_sys_init);

/*
 * Registers a bus with the system.
 */
void bus_register(bus_t *bus, char *name) {
	bus->drivers = list_allocate();
	bus->devices = list_allocate();
	bus->unknown_devices = list_allocate();

	bus->node.name = name;
	bus->node.parent = &root;
	bus->node.children = list_allocate();

	list_add(bus_list, bus);
	list_add(root.children, bus);
}