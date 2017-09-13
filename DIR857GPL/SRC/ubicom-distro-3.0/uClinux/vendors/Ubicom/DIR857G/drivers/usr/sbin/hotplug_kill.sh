#!/bin/sh

ACTION=$1
DIRTY=$2

case $ACTION in
"start" )
	if [ $DIRTY = 1 ]; then
		
	fi
	;;
"stop" )
#	if [ $DIRTY = 1 ]; then
		killall smbd
		killall ftpd
#	fi
	;;
esac
