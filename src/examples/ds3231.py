#!/usr/bin/env python


from mpsse import *
from datetime import datetime
from time import sleep


def to_bcd(val):
    val %= 100
    return ((val // 10) << 4) + (val % 10)


def init_time_cmd(WRCMD):

    now = datetime.now()

    regs = bytes([to_bcd(now.second),
                  to_bcd(now.minute),
                  to_bcd(now.hour),
                  (now.isoweekday() % 7) + 1,
                  to_bcd(now.day),
                  to_bcd(now.month),
                  to_bcd(now.year),
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])
    return WRCMD + regs


def print_bytes(*args):
    out = []
    for arg in args:
        out += [b for b in arg]

    formated = "".join([f"{b:02x} " for b in out])
    print(formated)


def main():
    try:

        ds3231 = MPSSE(I2C, FOUR_HUNDRED_KHZ)
        #ds3231 = MPSSE()
        #ds3231.OpenUsbDev(I2C, 1, 4)
        #if (ds3231.SetClock(FOUR_HUNDRED_KHZ) != MPSSE_OK) or (ds3231.SetMode(1) != MPSSE_OK) or (not ds3231.context.open):
        #       print("Error opening Bus 1 Device 4")
        #       exit(1)

        print("%s initialized at %dHz (I2C) open: %d" % (ds3231.GetDescription(), ds3231.GetClock(), ds3231.context.open))

        try:

            # Initialize date-time on the DS3231 module to a system date-time
            WRCMD1 = init_time_cmd(bytes([0xD0, 0]))

            print("Initializing time:")
            print_bytes(WRCMD1)

            ds3231.Start()
            ds3231.Write(WRCMD1)

            if ds3231.GetAck() == ACK:

                ds3231.Stop()

                print()
                print("Reading time in one second intervals (stop with Ctrl+C):")
                # write has been accepted, the device is present and communicating, start reading loop

                while True:

                    sleep(1)

                    WRCMD2 = bytes([0xD0, 0])
                    RDCMD  = bytes([0xD1])

                    # I2C Write-Read transaction,
                    # first write DS3231 register address
                    ds3231.Start()
                    ds3231.Write(WRCMD2)

                    # and if acknowledged, send the read command without stopping the transacton
                    if ds3231.GetAck() == ACK:
                        # send read command to the DS3231
                        ds3231.Start()
                        ds3231.Write(RDCMD)

                        if ds3231.GetAck() == ACK:

                            # all received bytes, except the last one must be acknowledged
                            ds3231.SendAcks()
                            data = ds3231.Read(18)

                            # send the NACK for the last byte to let DS3231 know that enough
                            # data has been transmitted
                            ds3231.SendNacks()
                            d = ds3231.Read(1)

                            # normal end of I2C transaction, send stop signal and release the line
                            ds3231.Stop()

                            print_bytes(data, d)

                        else:
                            ds3231.Stop()
                            raise Exception("Received read command NACK!")
                    else:
                        ds3231.Stop()
                        raise Exception("Received write command NACK!")

                else:
                    raise Exception("Received initial write command NACK!")

        except KeyboardInterrupt:
            print(" exiting...")
        finally:
            print("Closing I2C connection")
            ds3231.Close()

    except Exception as e:
        print("MPSSE failure", e)


if __name__ == "__main__":
    main()

