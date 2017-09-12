/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997,2008 Oracle.  All rights reserved.
 *
 * $Id: ReplicationLeaseExpiredException.java,v 1.1.1.1 2009/10/09 03:04:54 jack Exp $
 */
package com.sleepycat.db;

import com.sleepycat.db.internal.DbEnv;

/**
Thrown if a master lease has expired.
*/
public class ReplicationLeaseExpiredException extends DatabaseException {
    /* package */ ReplicationLeaseExpiredException(final String s,
                                   final int errno,
                                   final DbEnv dbenv) {
        super(s, errno, dbenv);
    }
}
