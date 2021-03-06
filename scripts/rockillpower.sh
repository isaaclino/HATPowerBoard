#!/bin/sh
### BEGIN INIT INFO
# Provides:          rockillPower
# Required-Start:    
# Required-Stop: halt
# Default-Start: 2 3 4 5  
# Default-Stop:      0 
# x-Stop-Before: halt
# x-Stop-After: rocpowerd
# Short-Description: kill the raspi power 
# Description:       kills power to the raspi by calling rocmpd with --power-off
### END INIT INFO

# for Wheezy (sys-v), this script should be placed in /etc/init.d and installed with update-rc.d.
# for Jessy, (systemd) this script should be placed in /lib/systemd/system-shutdown

NAME=rockillpower
DAEMON=/sbin/rocpmd
CONFIG=/etc/rocpmd.conf


power_off()
{
    echo "Powering Down..."

    if [ -e $DAEMON ]
    then
        $DAEMON --config-path $CONFIG --power-off
    else
        echo "Executable '$DAEMON' not found unable to start rocpmd."
    fi
}

case "$1" in
  start|reboot|halt|kexec)
	echo -n "Start nothing..."
	;;
  stop|poweroff)
	power_off
	;;
  restart)
	echo -n "Restarting nothing..."
	;;
  *)
	echo "Usage: $0 {start|stop|restart}"
	exit 1
esac

exit $?

