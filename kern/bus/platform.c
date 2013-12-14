#include <types.h>
#include "platform.h"

// Function declarations
static bool platform_match(device_t* i_device, driver_t* i_driver);

// Global variables
bus_t *platform_bus;

/*
 * Initialises the platform bus, and adds the standard devices that an x86
 * machine has, as well as those in the DSDT that match a list of PNP IDs.
 */
static int platform_init(void) {
	platform_bus = (bus_t *) kmalloc(sizeof(platform_bus));
	memclr(platform_bus, sizeof(platform_bus));

	platform_bus->match = platform_match;

	// Register bus
	bus_register(platform_bus, "platform");

	return 0;
}

/*
 * Checks if the platform driver is compatible with the given platform bus
 * device. This involves checking the device class and subclass against those
 * supported by the driver.
 */
static bool platform_match(device_t* i_device, driver_t* i_driver) {
	platform_device_t* device = (platform_device_t *) i_device;
	platform_driver_t* driver = (platform_driver_t *) i_driver;

	for(int i = 0; i < 7; i++) {
		if(device->dev.class == driver->supported_devices[i].class) {
			if(device->dev.subclass == driver->supported_devices[i].subclass) {
				return true;
			}
		}
	}

	return false;
}

module_bus_init(platform_init);