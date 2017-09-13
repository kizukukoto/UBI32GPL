/*
* Copyright c                  CAMEO Technology Corporation, 2005 
* All rights reserved.                                                    
* 
* Abstract : HTTP type definition
 *
 * $Author: norp_huang $
 *
 * Revision 1.0.0.0  2005/10/03 11:32:07  anderson.chen
 * no message
 *
*/

#ifndef _HTTP_TYPES_H
#define _HTTP_TYPES_H

/*
 * Internal names for basic integral types.  Omit the typedef if
 * not possible for a machine/compiler combination.
 */

typedef unsigned long long	   uint64;
typedef long long	           int64;
typedef unsigned int	       uint32;
typedef int			           int32;
typedef unsigned short	       uint16;
typedef short			       int16;
typedef unsigned char	       uint8;
typedef char			       int8;


#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif			   /* max */

#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif			   /* min */

#endif
