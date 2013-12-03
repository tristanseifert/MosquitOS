#ifndef DEVICE_H
#define DEVICE_H

#include <types.h>

// Include bus registration functions
#include "bus/bus.h"

typedef struct {
	bus_t *bus; // the bus this device is on
	void *bus_info; // pointer to bus-specific structure
	void *device_info; // pointer to device-specific info
} device_t;

#endif