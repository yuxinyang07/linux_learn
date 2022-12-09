#!/bin/bash
#
# rotate_desktop.sh
#
# Rotates modern Linux desktop screen and input devices to match. Handy for
# convertible notebooks. Call this script from panel launchers, keyboard
# shortcuts, or touch gesture bindings (xSwipe, touchegg, etc.).
#
# Using transformation matrix bits taken from:
#   https://wiki.ubuntu.com/X/InputCoordinateTransformation
#

# Configure these to match your hardware (names taken from `xinput` output).
#sleep 3
declare -i i=0
i=$(cat /home/heygears/count)
i+=1
sed -i "1c ${i}" /home/heygears/count
sync
TOUCH_DEVICE_ID=$(xinput | grep -iw ILITEK-TP)
if test -z "$TOUCH_DEVICE_ID"
then
    if [ $i -gt 3 ];
    then
    	exit
    else
	sudo reboot
    fi
	
else
i=0
sed -i "1c ${i}" /home/heygears/count
sync
TOUCH_DEVICE_ID=$(echo ${TOUCH_DEVICE_ID#*id=})
TOUCH_DEVICE_ID=$(echo ${TOUCH_DEVICE_ID%% *})   # 最终获取id号
xinput map-to-output $TOUCH_DEVICE_ID eDP-1
echo $TOUCH_DEVICE_ID
TOUCH_DEVICE_ID=$(xinput | grep -iw ILITEK-TP)
TOUCH_DEVICE_ID=$(echo ${TOUCH_DEVICE_ID#*id=})
TOUCH_DEVICE_ID=$(echo ${TOUCH_DEVICE_ID#*id=})
TOUCH_DEVICE_ID=$(echo ${TOUCH_DEVICE_ID%% *})   # 最终获取id号
xinput map-to-output $TOUCH_DEVICE_ID eDP-1
echo $TOUCH_DEVICE_ID

fi


