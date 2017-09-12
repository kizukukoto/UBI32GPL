/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997,2008 Oracle.  All rights reserved.
 *
 * $Id: ReplicationJoinFailureException.java,v 1.1.1.1 2009/10/09 03:04:54 jack Exp $
 */
package com.sleepycat.db;

import com.sleepycat.db.internal.DbEnv;

/**
Thrown if a new master has been chosen but the client is unable to synchronize
with the new master (possibly because the client has been configured
with the {@link ReplicationConfig#NOAUTOINIT} setting to turn-off automatic
internal initialization).
*/
public class ReplicationJoinFailureException extends DatabaseException {
    /* package */ ReplicationJoinFailureException(final String s,
                                   final int errno,
                                   final DbEnv dbenv) {
        super(s, errno, dbenv);
    }
}
