#include "interface/driver_interface.h"
#include <stdlib.h>
#include <string.h>

// Maximum number of drivers
#define MAX_DRIVERS 16

static struct driver_interface *registered_drivers[MAX_DRIVERS];
static int num_drivers = 0;

// Register a driver
int driver_register(struct driver_interface *driver) {
    if (num_drivers >= MAX_DRIVERS) {
        return -1; // Too many drivers
    }
    registered_drivers[num_drivers++] = driver;
    return 0;
}

// Unregister a driver
void driver_unregister(struct driver_interface *driver) {
    for (int i = 0; i < num_drivers; i++) {
        if (registered_drivers[i] == driver) {
            // Shift remaining drivers
            for (int j = i; j < num_drivers - 1; j++) {
                registered_drivers[j] = registered_drivers[j + 1];
            }
            num_drivers--;
            break;
        }
    }
}

// Probe device for compatible driver
int driver_probe_device(struct os_pci_device *dev, struct driver_interface **driver_out) {
    for (int i = 0; i < num_drivers; i++) {
        struct driver_interface *driver = registered_drivers[i];
        if (driver->probe && driver->probe(dev) == 0) {
            *driver_out = driver;
            return 0;
        }
    }
    return -1; // No compatible driver found
}

// Initialize device with driver
int driver_init_device(struct driver_interface *driver, struct OBJGPU *adev) {
    if (driver->init) {
        return driver->init(adev);
    }
    return -1;
}

// Get list of registered drivers
struct driver_interface **driver_get_list(int *count) {
    *count = num_drivers;
    return registered_drivers;
}