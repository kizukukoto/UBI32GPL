if [ $# -ne 1 ]
then
  echo "Usage: `basename $0` { - | <vmlinux.txt output from ip3kprof> }"
  exit
fi

cat $1 | egrep '^DR' | egrep 'vmlinux:' | \
{ \
	while read Loc Time Sum Total MIPS Haz IBLK DBLK rest; \
	do test $IBLK != 0.0 && echo $rest; \
	done; \
} | { \
	while IFS=\: read before after; \
	do echo $after | \
		{ \
			IFS=' ' read name rest; \
			echo \*\(.text.$name\); \
		}; \
	done; \
}
