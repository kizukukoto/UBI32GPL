#!/bin/sh

# **** config ****
NAS_CONFPATH=
SMB_CTRLDATA=
SMB_GBL_CONF="/tmp/smb.global.conf"
SMB_DIR_CONF="/tmp/smb.dir.conf"
SMB_DEF_CONF="/etc/samba/smb.def.conf"
SXSMBCONF="/usr/sbin/sxsambaconf"

# **** create global config file ****
create_global_conf(){
	TMP=`sxsysconf -f ${SMB_CTRLDATA} SMB_WORK_GROUP`
	if [ -n "$TMP" ]; then
		echo "workgroup = ${TMP}" >> ${SMB_GBL_CONF}
	fi
	
	TMP=`sxsysconf -f ${SMB_CTRLDATA} SMB_SERVER_STRING`
	if [ -n "$TMP" ]; then
		echo "server string = ${TMP}" >> ${SMB_GBL_CONF}
	fi

	TMP=`sxsysconf -f ${SMB_CTRLDATA} SMB_WINS_PRIMARY`
	if [ -n "$TMP" -a "$TMP" != "0.0.0.0" ]; then
		echo "wins server = ${TMP}" >> ${SMB_GBL_CONF}
	fi

	TMP=`sxsysconf -f ${NAS_CONFPATH} SMB_ADMIN`
	if [ -n "$TMP" ]; then
		echo "guest account = ${TMP}" >> ${SMB_GBL_CONF}
	fi

	TMP=`sxsysconf -f ${NAS_CONFPATH} SMB_MAXCON`
	if [ -n "$TMP" ]; then
		echo "max connections = ${TMP}" >> ${SMB_GBL_CONF}
	fi

	TMP=`sxsysconf -f ${SMB_CTRLDATA} NAS_ACTRL`
	if [ -n "$TMP" ]; then
		TMP=`echo "$TMP" | tr "A-Z" "a-z"`
		if [ "$TMP" = "enable" ]; then
			echo "security   = user" >> ${SMB_GBL_CONF}
			echo "guest ok   = no"   >> ${SMB_GBL_CONF}
			echo "guest only = no"   >> ${SMB_GBL_CONF}
		fi
	fi

	return 0
}

# **** create directory config file ****
create_directory_conf(){
	${SXSMBCONF} -c ${SMB_DIR_CONF} -d ${SMB_DEF_CONF}
	return 0
}

# start script ***********
if [ "$#" -lt "2" ]; then
	exit 1
fi

# samba data
NAS_CONFPATH="$1"
SMB_CTRLDATA="$2"

# cleanup samba config
echo -n "" > ${SMB_GBL_CONF}
echo -n "" > ${SMB_DIR_CONF}

# create samba config file
create_global_conf
create_directory_conf

