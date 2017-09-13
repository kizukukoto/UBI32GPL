/* lock.h - function prototypes for locking structure access */

/* Copyright (c) 2001 Atheros Communications, Inc., All Rights Reserved */


#ident  "ACI $Id: //depot/sw/src/dk/mdk/common/include/lock.h#1 $, $Header: //depot/sw/src/dk/mdk/common/include/lock.h#1 $"

/* 
modification history
--------------------
00a    14may01    fjc    Created.
*/

/**************************************************************************
* initializeLock - Initialize resource lock mechanism
*
* This will initialize a spinlock for use within the kernel environment
*
* RETURNS: N/A
*/
void initializeLock
	(
	void **ppLock
	);


/**************************************************************************
* acquireLock - Aquire lock for resource
*
* This will acquire the spinlock for the resource within the kernel environment
*
* RETURNS: N/A
*/
void acquireLock
	(
	void *pLock
	);

/**************************************************************************
* releaseLock - release lock for resource
*
* This will release the spinlock for the resource within the kernel environment
*
* RETURNS: N/A
*/
void releaseLock
	(
	void *pLock
	);


/**************************************************************************
* deleteLock - delete lock for resource
*
* This will delete the spinlock within the kernel environment
*
* RETURNS: N/A
*/
void deleteLock
	(
	void *pLock
	);

/**************************************************************************
* printMsg - Print within kernel mode
*
* 
*
* RETURNS: N/A
*/
void printMsg
	(
	A_CHAR *msg
	);




