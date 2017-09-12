# See the file LICENSE for redistribution information.
#
# Copyright (c) 1999,2008 Oracle.  All rights reserved.
#
# $Id: test081.tcl,v 1.1.1.1 2009/10/09 03:06:58 jack Exp $
#
# TEST	test081
# TEST	Test off-page duplicates and overflow pages together with
# TEST	very large keys (key/data as file contents).
proc test081 { method {ndups 13} {tnum "081"} args} {
	source ./include.tcl

	eval {test017 $method 1 $ndups $tnum} $args
}
