#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev;

    // Create the udev object
    udev = udev_new();
    if (!udev) {
        fprintf(stderr, "Can't create udev\n");
        exit(1);
    }

    // Create a list of the devices in the 'usb' subsystem.
    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "usb");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

    // For each item, get its device and print its information
    udev_list_entry_foreach(dev_list_entry, devices) {
        const char *path;

        // Get the filename of the /sys entry for the device
        path = udev_list_entry_get_name(dev_list_entry);
        // Get the device from the path
        dev = udev_device_new_from_syspath(udev, path);

        // Get VID, PID, bus number, and device number
        const char *vid = udev_device_get_sysattr_value(dev, "idVendor");
        const char *pid = udev_device_get_sysattr_value(dev, "idProduct");
        const char *busnum = udev_device_get_sysattr_value(dev, "busnum");
        const char *devnum = udev_device_get_sysattr_value(dev, "devnum");

        if (vid && pid && busnum && devnum) {
            printf("VID: %4.4s, PID: %4.4s\t", vid, pid);
            printf("Bus Number: %2.2s, Device Number: %2.2s\t", busnum, devnum);
            printf("Device Node Path: %s\n", udev_device_get_devnode(dev));
        }

        udev_device_unref(dev);
    }

    // Free the enumerator object
    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    return 0;
}
