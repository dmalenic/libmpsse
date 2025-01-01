#!/usr/bin/env python

# DS1302 DataSheet: https://www.analog.com/media/en/technical-documentation/data-sheets/DS1302.pdf

from mpsse import *
from datetime import datetime
from time import sleep


def to_bcd(val):
    val %= 100
    return ((val // 10) << 4) + (val % 10)


def init_time_cmd(cmd):

    now = datetime.now()

    regs = bytes([cmd,
                  to_bcd(now.second),
                  to_bcd(now.minute),
                  to_bcd(now.hour),
                  to_bcd(now.day),
                  to_bcd(now.month),
                  (now.isoweekday() % 7) + 1,
                  to_bcd(now.year),
                  0])
    return regs


def print_bytes(*args):
    out = []
    for arg in args:
        out += [b for b in arg]

    formated = "".join([f"{b:02x} " for b in out])
    print(formated)


def main():
    try:

        ds1302 = MPSSE(SPI3, ONE_MHZ, LSB)

        # The DS1302's chip select pin idles low
        ds1302.SetCSIdle(0)

        print("%s initialized at %dHz (SPI mode 3), LSB and CS low on idle." % (ds1302.GetDescription(), ds1302.GetClock()));

        print("Reading control register.")

        # Get the current control register value - one-byte write command
        ds1302.Start()
        ds1302.Write(bytes([0x8F]))
        control = ds1302.Read(1)[0]
        ds1302.Stop()

        print("Writing control register.")

        # single-byte write command
        # disabling write-protect bit is not strictly necessary in this example
        # because it protects RAM registers only, and we are writing Clock registers
        control &= ~0x80

        # Write the new control value to the control register
        ds1302.Start()
        ds1302.Write(bytes([0x8E]))
        ds1302.Write(bytes([control]))
        ds1302.Stop()

        print("Initializing time:\n");

        # Write the initial date
        init = init_time_cmd(0xBF)
        ds1302.Start()
        ds1302.Write(init)
        ds1302.Stop()

        print_bytes(init[1:])

        print("Reading time in one second intervals (stop with Ctrl+C):");

        # Loop to print the time elapsed every second
        while True:

            try:
                sleep(1)

                # Read in the elapsed seconds
                ds1302.Start()
                ds1302.Write(bytes([0xBF]))
                data = ds1302.Read(8)
                ds1302.Stop()

                print_bytes(data)

            except KeyboardInterrupt:
                break

        ds1302.Close()
        print("Exiting...")

    except Exception as e:
        print("Error reading from DS1302:", e)


if __name__ == "__main__":
    main()

