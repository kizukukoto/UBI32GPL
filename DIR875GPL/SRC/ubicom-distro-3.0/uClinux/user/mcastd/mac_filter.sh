#!/bin/sh
if [ -x /tmp/mac_filter.sh ];then
        /tmp/mac_filter.sh
        exit $?
fi

# This script add the acl rule to drop mld snooping join packet from wired client when MAC filter enabled list_deny.
# The MAC filter have 24(00~23) rules.
# In ACL rules, It will use ACL rule No.70~95 for MAC filter.
# If need to change ACL rule number range, please change $acl_start and $acl_end values.
acl_start=70  #Louis_Wang modify. This modify is for white_list
			  #white_list will add more than black_list one rule so index will from acl_start=70 to acl_end=95
acl_end=95

type=`nvram get mac_filter_type`
debug=0

list_deny()
{
	acl_n=$acl_start
	mac_n=0
	#for mac_n in $(seq 0 23); do
	while [ $mac_n -le 23 ]; do
		if [ $mac_n -lt 10 ]; then
			en=`nvram get mac_filter_0$mac_n | cut -d '/' -f 1`
			mac=`nvram get mac_filter_0$mac_n | cut -d '/' -f 3`
		else
			en=`nvram get mac_filter_$mac_n | cut -d '/' -f 1`
			mac=`nvram get mac_filter_$mac_n | cut -d '/' -f 3`
		fi

		if [ $debug -eq 1 ]; then
			echo "MAC Filter Number: $mac_n"
			echo "Enable: $en"
			echo "MAC: $mac"
		fi

# ubicom issue
en=`echo $en | cut -d '=' -f 2`
en=`echo $en | sed 's/^ *\(.*\) *$/\1/'`



		if [ $en -eq 1 ]; then
			if [ $debug -eq 1 ]; then
				echo "# mcastd acl add -n $acl_n -s $mac -j 7 -p 0x1E"
			fi
			mcastd acl add -n $acl_n -s $mac -j 7 -p 0x1E
			acl_n=$(($acl_n+1))
		fi
		mac_n=$(($mac_n+1))
	done
}


list_allow()
{
	mac_n=0
	while [ $mac_n -le 23 ]; do
		if [ $mac_n -lt 10 ]; then
			en=`nvram get mac_filter_0$mac_n | cut -d '/' -f 1`
		else
			en=`nvram get mac_filter_$mac_n | cut -d '/' -f 1`
		fi

# ubicom issue
en=`echo $en | cut -d '=' -f 2`
en=`echo $en | sed 's/^ *\(.*\) *$/\1/'`

		
		if [ $en -eq 1 ]; then
			last_mac_n=$mac_n
		fi
		mac_n=$(($mac_n+1))
	done

	if [ $debug -eq 1 ]; then
		echo "last mac no: $last_mac_n"
	fi


	acl_n=$acl_start
	mac_n=0
	while [ $mac_n -le 23 ]; do
		if [ $mac_n -lt 10 ]; then
			en=`nvram get mac_filter_0$mac_n | cut -d '/' -f 1`
			mac=`nvram get mac_filter_0$mac_n | cut -d '/' -f 3`
		else
			en=`nvram get mac_filter_$mac_n | cut -d '/' -f 1`
			mac=`nvram get mac_filter_$mac_n | cut -d '/' -f 3`
		fi

		if [ $debug -eq 1 ]; then
			echo "MAC Filter Number: $mac_n"
			echo "Enable: $en"
			echo "MAC: $mac"
		fi

# ubicom issue
en=`echo $en | cut -d '=' -f 2`
en=`echo $en | sed 's/^ *\(.*\) *$/\1/'`


		if [ $en -eq 1 ]; then
			if [ $debug -eq 1 ]; then
				echo "# mcastd acl add -n $acl_n -s $mac -j 7 -p 0x1E"
			fi

			if [ $mac_n != $last_mac_n ]; then
				mcastd acl add -n $acl_n -s $mac -j 0 -p 0x1E
				acl_n=$(($acl_n+1))
			else
				mcastd acl add -n $acl_n -i -s $mac -j 0 -p 0x1E
				acl_n=$(($acl_n+1))
				mcastd acl add -n $acl_n -i -s $mac -j 7 -p 0x1E
			fi
		fi
		mac_n=$(($mac_n+1))
	done
}


disable_all()
{
	num=$acl_start
	#for num in $(seq 70 95); do
	while [ $num -le $acl_end ]; do
		if [ "$debug" -eq 1 ]; then
			echo "# mcastd acl disable $num"
		fi
		mcastd acl disable $num
		num=$(($num+1))
	done
}

#-------------------------------------------------------
disable_all

# ubicom issue
type=`echo $type | cut -d '=' -f 2`

type=`echo $type | sed 's/^ *\(.*\) *$/\1/'`


if [ $type = "list_deny" ]; then
	list_deny
elif [ $type = "list_allow" ]; then
	list_allow
fi
