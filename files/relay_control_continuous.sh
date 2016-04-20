#!/bin/sh /etc/rc.common

while true
do
	line=$(ps | grep 'relay_control_appd' | grep -v grep)
	if [ -z "$line" ]
	then
		/etc/init.d/relay_control_appd start
	else
		sleep 5
	fi
done

