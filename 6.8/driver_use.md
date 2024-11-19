# steps to load the driver

*** ftdi_sio driver file is already used to manage USB serial, disable this driver

1. sudo rmmod /lib/modules/6.8.0-48-generic/kernel/drivers/usb/serial/ftdi_sio.ko

2. ls /dev/tty*

Load our_driver to manage the USB FT232

1. make clean
2. make
3. sudo rmmod raspberryusb
4. sudo insmod raspberryusb.ko
5. ls /dev/tty*

*** additional checks

1. lsusb (Vendor ID : Product ID)
2. lsmod | grep raspberryusb (raspberryusb=name & use serial port for comunication)