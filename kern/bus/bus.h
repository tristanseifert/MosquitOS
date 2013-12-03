#ifndef BUS_H
#define BUS_H

#include <types.h>
#include "runtime/list.h"

typedef struct node node_t;
typedef struct bus bus_t;
typedef struct device device_t;
typedef struct driver driver_t;

struct device {
	bus_t *bus; // the bus this device is on
	void *bus_info; // pointer to bus-specific structure
	void *device_info; // pointer to device-specific info
};

struct driver {

};

struct node {
	char *name;
	node_t *parent;
	list_t *children;
};

struct bus {
	node_t node;

	list_t *drivers;
	list_t *devices;
	list_t *unknown_devices;

	// This checks if the driver will work for the device
	int (*match)(device_t*, driver_t*);
};

void bus_register(bus_t *bus, char *name);

#endif

