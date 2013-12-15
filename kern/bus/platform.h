#ifndef PLATFORM_H
#define PLATFORM_H

#include <types.h>
#include "bus.h"

extern bus_t *platform_bus;

#define PLATFORM_CLASS_HID 0xD000
#define PLATFORM_SUBCLASS_KBC 0xCACA

typedef struct platform_driver platform_driver_t;
typedef struct platform_device platform_device_t;
typedef struct platform_devid platform_devid_t;

struct platform_devid {
	uint16_t class;
	uint16_t subclass;

	uint32_t device_id;
};

struct platform_driver {
	driver_t d;
	platform_devid_t supported_devices[8];
};

struct platform_device {
	device_t d;
	platform_devid_t dev;
};

#endif