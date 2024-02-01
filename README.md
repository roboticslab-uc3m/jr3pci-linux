Imported from https://sourceforge.net/projects/jr3pci-linux/

This is a linux driver for the JR3 force/torque sensor (PCI card), under 2.4 kernels. Currently, two cards are supported:

- Single channel PCI
- Double channel PCI (for two sensors, or one 12DOF sensor)

Only filtered reads and zero offsets setup are supported.

For compiling it, edit jr3pci-driver.h and set PCI_DEVICE_ID_JR3 to be that of your card.
Then, edit the Makefile, and set the path to your kernel sources.
Finally, type 'make'.

The module is compiled in the file jr3pci-driver.o. For inserting it into the kernel, type 'insmod ./jr3pci-driver.o' for kernels <=2.4, or 'insmod ./jr3pci-driver.ko' for 2.6 kernels

After inserting it, look at the green leds of your PCI card. If they are on, you're in chance!!! :)

Also included is a simple test program 'jr3mon' which reads force values infinitelly. For user space programs to work, you must first create the device file into /dev directory with the command:

mknod /dev/jr3 c 39 0

Alternatively, you can try 'make node'.

Problems, suggestions or anything else: mprats@icc.uji.es

More install and troubleshooting at: https://github.com/roboticslab-uc3m/installation-guides/blob/master/docs/install-jr3.md
