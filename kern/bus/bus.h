#ifndef BUS_H
#define BUS_H

#include <types.h>
#include "runtime/list.h"

#define BUS_NOT_EXISTANT -1
#define BUS_DRIVER_ALREADY_REGISTERED -2
#define BUS_DEVICE_REGISTERED -3

#define BUS_NAME_PCI "pci_bus"
#define BUS_NAME_PLATFORM "platform"

typedef struct node node_t;
typedef struct bus bus_t;
typedef struct device device_t;
typedef struct driver driver_t;

struct node {
	char *name;
	node_t *parent;
	list_t *children;
};

struct device {
	node_t node;

	void *bus_info; // pointer to bus-specific structure
	void *device_info; // pointer to device-specific info

	void *sysInfo; // hook for system-specific info

	// Resources used by device
	list_t *resources;
};

struct driver {
	char* name;

	bool (*supportQuery)(device_t *); // function to query driver support
};

struct bus {
	node_t node;

	list_t *drivers;

	list_t *devices;
	list_t *unknown_devices;

	// This checks if the driver will work for the device
	bool (*match)(device_t*, driver_t*);
};

// Registers a bus with the driver
void bus_register(bus_t*, char*);
int bus_register_driver(driver_t*, char*);
bus_t *bus_get_by_name(char*);
driver_t *bus_find_driver(device_t*, bus_t*);
int bus_add_device(device_t*, bus_t*);

#endif

