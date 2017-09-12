/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997,2008 Oracle.  All rights reserved.
 *
 * $Id: RepRemoteHost.java,v 1.1.1.1 2009/10/09 03:04:41 jack Exp $
 */

package db.repquote;

import com.sleepycat.db.ReplicationHostAddress;

public class RepRemoteHost{
    private ReplicationHostAddress addr;
    private boolean isPeer;

    public RepRemoteHost(ReplicationHostAddress remoteAddr, boolean hostIsPeer){
	addr = remoteAddr;
	isPeer = hostIsPeer;
    }

    public ReplicationHostAddress getAddress(){
	return addr;
    }

    public boolean isPeer(){
	return isPeer;
    }
}
