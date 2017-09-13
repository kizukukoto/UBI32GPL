#!/bin/sh

while [ $# -gt 0 ]; do
	case $1 in
	-D*|-I*)	Defs="$Defs $1"	;;
	-SDK)	SDK_DIR=$2;	shift	;;
	-o)	OUT_F=$2;	shift	;;
	-*)	exit -1	;;
	*)	IN_F=$1	;;
	esac
	shift
done

# default argument is input C for data structure definitions, such as
#include <this_struct.h>
#	#define	DATA_FIELD1	&((struct this_struct*)0)->data_field1
#	#define	DATA_FIELD2	&((struct this_struct*)0)->data_field2
# etc.
# restriction:	definitions (output) must match data files (right most)
#	#define	XXYY	&((struct this_struct*)0)->abc_XYZ
# will output
#	#define	ABC_XYZ	###

FileBody=`expr $IN_F : "\(.*\)[.]."`	# ignore the extension

# argument is the output file for included new header file name.
# if omitted, it will be $1 with extension change to .h
# because SHELL does not like empty redirection
[ -z "$OUT_F" ] &&	OUT_F=${2-$FileBody.h}

TMPF=/tmp/dfconv.$$.c
# adding data assignment for conversion
#	const int	xyz = XYZ;
awk -F">" '{ if (NF!=2 || !index($1, "#define")) print; else {print $0; n=split($1, a, " "); printf("const int	%s = (int)%s;\n", $2, a[2])}	}' $IN_F > $TMPF

[ $? -eq 0 ] && {
	# project dependent include -- ipUIGen/src is for i18_lang*.h
	Defs="-Ibuild/include -Iconfig -I$SDK_DIR/pkg/ipUIGen/src $Defs"
	# gen assemble definitions
	ubicom32-elf-gcc -fno-builtin -S $TMPF -I. $Defs -o $FileBody.s

	# parsing assemble definitions to C header file definitions
	awk 'BEGIN{FieldData=0} {if (FieldData!=0 && NF==2) {printf("#define %s %d\n", toupper(FieldData), $2);	FieldData=0}	\
		else if (NF==1 && index($1, "#")!=1 && index($1, ".")!=1) {FieldData=substr($1, 1, length($1)-1)} else {FieldData=0}}' $FileBody.s > $OUT_F
}

rm -f $FileBody.s $TMPF
