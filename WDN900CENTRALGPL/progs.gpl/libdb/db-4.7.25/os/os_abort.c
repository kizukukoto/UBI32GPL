/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005,2008 Oracle.  All rights reserved.
 *
 * $Id: os_abort.c,v 1.1.1.1 2009/10/09 03:05:29 jack Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_abort --
 *
 * PUBLIC: void __os_abort __P((ENV *));
 */
void
__os_abort(env)
	ENV *env;
{
	__os_stack(env);		/* Try and get a stack trace. */

#ifdef HAVE_ABORT
	abort();			/* Try and drop core. */
	/* NOTREACHED */
#endif
#ifdef SIGABRT
	(void)raise(SIGABRT);		/* Try and drop core. */
#endif
	exit(1);			/* Quit anyway. */
	/* NOTREACHED */
}
