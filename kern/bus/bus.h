#ifndef BUS_H
#define BUS_H

#include <types.h>

typedef enum {
	kBusTypeNone,
	kBusTypePCI,
	kBusTypeISA,
	kBusTypeATA,
	kBusTypeUSB,
	kBusTypeUnknown = 0xFFFFFFFF
} bus_type_t;

typedef struct {
	// Based on the below field, the pointer that was given to us can be cast
	// to one of the bus_*_t structs below.
	bus_type_t bus_type;
} bus_t;

#endif