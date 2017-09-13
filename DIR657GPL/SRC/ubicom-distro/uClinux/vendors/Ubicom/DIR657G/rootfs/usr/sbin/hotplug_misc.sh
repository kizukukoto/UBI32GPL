#!/bin/sh

# SAMBA
SMBCONF=/usr/sbin/sxsambaconf

# test code
# DMSG="${ACTION}:${SHAREPATH}:${DEVPATH}"
# echo "hotplug.misc ${DMSG}" | logger -p 4

case "$ACTION" in
"BEFOREMNT")
        ;;
"AFTERMNT")
        $SMBCONF -c "${DEVPATH}/smb.dir.conf" -d "/etc/samba/smb.def.conf"
        ;;
esac

