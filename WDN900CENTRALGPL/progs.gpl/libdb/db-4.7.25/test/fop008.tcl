# See the file LICENSE for redistribution information.
#
# Copyright (c) 2005,2008 Oracle.  All rights reserved.
#
# $Id: fop008.tcl,v 1.1.1.1 2009/10/09 03:06:35 jack Exp $
#
# TEST	fop008
# TEST	Test file system operations on named in-memory databases.
# TEST	Combine two ops in one transaction.
proc fop008 { method args } {
	eval {fop006 $method 1} $args
}



