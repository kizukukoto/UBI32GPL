#!/bin/sh
debug=0
base=/internalhd
log_path=${base}/tmp/var/log
max_log_files=5
[ "${debug}" = "1" ] && echo "Rotate logs..." > /dev/console
test -f ${log_path}/error.log && rm -f ${log_path}/error.log
test -f ${log_path}/access.log && rm -f ${log_path}/access.log
test -f ${log_path}/error_ssl.log && rm -f ${log_path}/error_ssl.log
test -f ${log_path}/access_ssl.log && rm -f ${log_path}/access_ssl.log
tmp=/var/rotatelog.tmp
log_list="error access error_ssl access_ssl"
for n in ${log_list}; do
	ls -1 ${log_path}/${n}.* 2>/dev/null > ${tmp}
	i=1
	while read f; do
		if [ "${i}" -le "${max_log_files}" ]; then
			[ "${debug}" = "1" ] && echo "file ${i}: ${f}"
		else
			[ "${debug}" = "1" ] && echo "rm ${i}: ${f}"
			rm -f "${f}"
		fi
		i=`expr ${i} + 1`
	done < ${tmp}
done
xmldbc -k rotatelog
xmldbc -t rotatelog:60:"${base}/root/rotatelog.sh"
exit 0
