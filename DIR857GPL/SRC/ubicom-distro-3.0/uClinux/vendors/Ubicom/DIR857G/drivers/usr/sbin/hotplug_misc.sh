#!/bin/sh

# SAMBA
SMBCONF=/usr/sbin/sxsambaconf

# test code
# DMSG="${ACTION}:${SHAREPATH}:${DEVPATH}"
# echo "hotplug.misc ${DMSG}" | logger -p 4

GET_FILE_ACCESS_ENABLE=`nvram get file_access_enable | cut -d " " -f 3`

case "$ACTION" in
"BEFOREMNT")
        ;;
"AFTERMNT")
        $SMBCONF -c "${DEVPATH}/smb.dir.conf" -d "/etc/samba/smb.def.conf"
	if [ "$GET_FILE_ACCESS_ENABLE" = "1" ]; then
		killall lighttpd
		STORAGEPATH=`grep path /tmp/smb.dir.conf | sed s/path=// | sed '2d'`
		sed "s,^server.upload-dirs = .*,server.upload-dirs = ( \"${STORAGEPATH}\" ),g" -i /var/tmp/lighttpd-wa/lighttpd.conf
		lighttpd -f /etc/lighttpd.conf -A&
	fi
        ;;
esac

