#!/bin/sh
# Copyright (C) 2006 OpenWrt.org
# Copyright (C) 2006 Fokus Fraunhofer <carsten.tittel@fokus.fraunhofer.de>

#alias debug=${DEBUG:-:}
#alias mount='busybox mount'

# newline
N="
"

_C=0
NO_EXPORT=1
LOAD_STATE=1
LIST_SEP=" "

hotplug_dev() {
	env -i ACTION=$1 INTERFACE=$2 /sbin/hotplug-call net
}

append() {
	 var="$1"
	 value="$2"
	 sep="${3:- }"

	 var_value=$(eval "echo \${$var}")                          
	 [ -n "$var_value" ] &&                                     
	   eval "export ${NO_EXPORT:+-n} \"$var=\${$var}$sep$value\""
	 [ -n "$var_value" ] ||                                            
	   eval "export ${NO_EXPORT:+-n} \"$var=$value\""            
#	 echo "export ${NO_EXPORT:+-n} \"$var=\${$var}$sep$value\""
#	 eval "export ${NO_EXPORT:+-n} -- \"$var=\${$var:+\${$var}\${value:+\$sep}}\$value\""
}

list_contains() {
	 var="$1"
	 str="$2"
	 val=

	eval "val=\" \${$var} \""
	[ "${val%% $str *}" != "$val" ]
}

list_remove() {
	 var="$1"
	 remove="$2"
	 val=

	eval "val=\" \${$var} \""
	val1="${val%% $remove *}"
	[ "$val1" = "$val" ] && return
	val2="${val##* $remove }"
	[ "$val2" = "$val" ] && return
	val="${val1## } ${val2%% }"
	val="${val%% }"
	eval "export ${NO_EXPORT:+-n} \"$var=\$val\""
}

config_load() {
	[ -n "$IPKG_INSTROOT" ] && return 0
	uci_load "$@"
}

reset_cb() {
	config_cb() { return 0; }
	option_cb() { return 0; }
	list_cb() { return 0; }
}
reset_cb

package() {
	return 0
}

config () {
	 cfgtype="$1"
	 name="$2"
	
	export ${NO_EXPORT:+-n} CONFIG_NUM_SECTIONS=$((CONFIG_NUM_SECTIONS + 1))
	name="${name:-cfg$CONFIG_NUM_SECTIONS}"
	append CONFIG_SECTIONS "$name"
	[ -n "$NO_CALLBACK" ] || config_cb "$cfgtype" "$name"
	export ${NO_EXPORT:+-n} CONFIG_SECTION="$name"
	export ${NO_EXPORT:+-n} "CONFIG_${CONFIG_SECTION}_TYPE=$cfgtype"
}

option () {
	 varname="$1"; shift
	 value="$*"

	export ${NO_EXPORT:+-n} "CONFIG_${CONFIG_SECTION}_${varname}=$value"
	[ -n "$NO_CALLBACK" ] || option_cb "$varname" "$*"
}

list() {
	 varname="$1"; shift
	 value="$*"
	 len=

	config_get len "$CONFIG_SECTION" "${varname}_LENGTH" 
	len_val="${len:-0}"
	len="$((len_val + 1))"
	config_set "$CONFIG_SECTION" "${varname}_ITEM$len" "$value"
	config_set "$CONFIG_SECTION" "${varname}_LENGTH" "$len"
	append "CONFIG_${CONFIG_SECTION}_${varname}" "$value" "$LIST_SEP"
	list_cb "$varname" "$*"
}

config_rename() {
	 OLD="$1"
	 NEW="$2"
	 oldvar=
	 newvar=
	
	[ -n "$OLD" -a -n "$NEW" ] || return
	set > /tmp/set.txt
	for oldvar in `cat /tmp/set.txt | grep ^CONFIG_${OLD}_ | sed -e 's/\(.*\)=.*$/\1/'` ; do
		newvar="CONFIG_${NEW}_${oldvar##CONFIG_${OLD}_}"
		eval "export ${NO_EXPORT:+-n} \"$newvar=\${$oldvar}\""
		unset "$oldvar"
	done
	export ${NO_EXPORT:+-n} CONFIG_SECTIONS="$(echo " $CONFIG_SECTIONS " | sed -e "s, $OLD , $NEW ,")"
	
	[ "$CONFIG_SECTION" = "$OLD" ] && export ${NO_EXPORT:+-n} CONFIG_SECTION="$NEW"
}

config_unset() {
	config_set "$1" "$2" ""
}

config_clear() {
	 SECTION="$1"
	 oldvar=

	list_remove CONFIG_SECTIONS "$SECTION"
	eval "export \${NO_EXPORT:+-n} CONFIG_SECTIONS=\"\${SECTION:+$CONFIG_SECTIONS}\""

	set > /tmp/set.txt
	varlist=$(eval "cat /tmp/set.txt | grep ^CONFIG_\${SECTION:+${SECTION}_}")
	for oldvar in `echo "$varlist" | sed -e 's/\(.*\)=.*$/\1/'` ; do 
		unset $oldvar 
	done
}

config_get() {
	case "$3" in
		"") eval "echo \"\${CONFIG_${1}_${2}}\"";;
		*)  eval "export ${NO_EXPORT:+-n} \"$1=\${CONFIG_${2}_${3}}\"";;
	esac
}

# config_get_bool <variable> <section> <option> [<default>]
config_get_bool() {
	 _tmp=
	config_get "_tmp" "$2" "$3"
	case "$_tmp" in
		1|on|true|enabled) export ${NO_EXPORT:+-n} "$1=1";;
		0|off|false|disabled) export ${NO_EXPORT:+-n} "$1=0";;
		*) eval "$1=$4";;
	esac
}

config_set() {
	 section="$1"
	 option="$2"
	 value="$3"
	 old_section="$CONFIG_SECTION"

	CONFIG_SECTION="$section"
	option "$option" "$value"
	CONFIG_SECTION="$old_section"
}

config_foreach() {
	 function="$1"
	[ "$#" -ge 1 ] && shift
	 type="$1"
	[ "$#" -ge 1 ] && shift
	 section= cfgtype=
	
	[ -z "$CONFIG_SECTIONS" ] && return 0
	for section in ${CONFIG_SECTIONS}; do
		config_get cfgtype "$section" TYPE
		[ -n "$type" -a "x$cfgtype" != "x$type" ] && continue
		eval "$function \"\$section\" \"\$@\""
	done
}

config_list_foreach() {
	[ "$#" -ge 3 ] || return 0
	 section="$1"; shift
	 option="$1"; shift
	 function="$1"; shift
	 val=
	 len=
	 c=1

	config_get len "${section}" "${option}_LENGTH"
	[ -z "$len" ] && return 0
	while [ $c -le "$len" ]; do
		config_get val "${section}" "${option}_ITEM$c"
		eval "$function \"\$val\" \"$@\""
		c="$((c + 1))"
	done
}

load_modules() {
	[ -d /etc/modules.d ] && {
		cd /etc/modules.d
		sed 's/^[^#]/insmod &/' $* | ash 2>&- || :
	}
}

include() {
	 file=
	
	for file in $(ls $1/*.sh 2>/dev/null); do
		. $file
	done
}

find_mtd_part() {
	 PART="$(grep "\"$1\"" /proc/mtd | awk -F: '{print $1}')"
	 PREFIX=/dev/mtdblock
	
	PART="${PART##mtd}"
	[ -d /dev/mtdblock ] && PREFIX=/dev/mtdblock/
	echo "${PART:+$PREFIX$PART}"
}

strtok() { # <string> { <variable> [<separator>] ... }
	 tmp=
	 val="$1"
	 count=0

	shift

	while [ $# -gt 1 ]; do
		tmp="${val%%$2*}"

		[ "$tmp" = "$val" ] && break

		val="${val#$tmp$2}"

		export ${NO_EXPORT:+-n} "$1=$tmp"; count=$((count+1))
		shift 2
	done

	if [ $# -gt 0 -a -n "$val" ]; then
		export ${NO_EXPORT:+-n} "$1=$val"; count=$((count+1))
	fi

	return $count
}


jffs2_mark_erase() {
	 part="$(find_mtd_part "$1")"
	[ -z "$part" ] && {
		echo Partition not found.
		return 1
	}
	echo -e "\xde\xad\xc0\xde" | mtd -qq write - "$1"
}

uci_apply_defaults() {
	cd /etc/uci-defaults || return 0
	files="$(ls)"
	[ -z "$files" ] && return 0
	mkdir -p /tmp/.uci
	for file in $files; do
		. "./$(basename $file)" && rm -f "$file"
	done
	uci commit
}

[ -z "$IPKG_INSTROOT" -a -f /lib/config/uci.sh ] && . /lib/config/uci.sh
