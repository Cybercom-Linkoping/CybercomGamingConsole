#!/bin/bash

for i in 1 2 ; do
        case "$i" in
        "1") CFG="0xe3c1" ;; #works with 0x00c1 as well
        "2") CFG="0xe3d1" ;;
        esac
        sudo i2cset -y 1 0x48 1 $CFG w
        RES=`sudo i2cget -y 1 0x48 1 w`
        until [ "$RES" = "$CFG" ] ; do
                RES=`sudo i2cget -y 1 0x48 1 w`
        done
        X=`sudo i2cget -y 1 0x48 0 w`
        XX="0x0`echo $X | cut -c 5-6``echo $X | cut -c 3`"
        printf "AIN$i = %d\n" $XX
        dec=$(printf "%d" $XX)
        mVolt=`expr $dec \* 3`
        printf "AIN$i = %d mV\n" $mVolt
done
exit


