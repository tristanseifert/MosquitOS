#ifndef BUS_H
#define BUS_H

#include <types.h>

typedef struct bus_info {
	int (*bus_init) (struct bus_info*);
	int (*bus_deinit) (struct bus_info*);

	void* (*bus_device_info) (struct bus_device*);
} bus_info_t;

#endif