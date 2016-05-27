#!/bin/sh
GPIO=$1
GPIODIR=/sys/class/gpio/gpio$GPIO
RELAY=$2
#check if the gpio is already exported
if [ ! -e "$GPIODIR" ]
then
        echo $GPIO > /sys/class/gpio/export
        echo out > /sys/class/gpio/gpio$GPIO/direction
        echo 0 > /sys/class/gpio/gpio$GPIO/value
        echo "RELAY $RELAY configured "
else
        echo "RELAY $RELAY configured "
fi
