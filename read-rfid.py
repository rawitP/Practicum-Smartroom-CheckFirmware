from practicum import *
from time import sleep

RQ_READ = 1
RQ_READ_LENGTH = 5

devs = findDevices()

if len(devs) == 0:
    print("*** No MCU board found.")
    exit(1)

b = McuBoard(findDevices()[0])
print("*** MCU board found")
print("*** Device manufacturer: %s" % b.getVendorName())
print("*** Device name: %s" % b.getDeviceName())

while True:
    sleep(1)
    print(b.usbRead(RQ_READ, length=RQ_READ_LENGTH))

