#ifdef VXWORKS
#include "vxworks.h"
#endif
#ifdef ECOS
#include "ecosdrv.h"
#endif
#include "wlantype.h"
#include "endian_func.h"

// use to convert big endian data to little endian data and vice versa
A_UINT32 ltob_l(A_UINT32 num)
{
	return (((num & 0xff) << 24) | ((num & 0xff00) << 8) | ((num & 0xff0000) >> 8) | ((num & 0xff000000) >> 24));
	
}

A_UINT16 ltob_s(A_UINT16 num)
{
	return (((num & 0xff) << 8) | ((num & 0xff00) >> 8));
	
}

A_UINT32 btol_l(A_UINT32 num)
{
	return (((num & 0xff) << 24) | ((num & 0xff00) << 8) | ((num & 0xff0000) >> 8) | ((num & 0xff000000) >> 24));
	
}

A_UINT16 btol_s(A_UINT16 num)
{
	return (((num & 0xff) << 8) | ((num & 0xff00) >> 8));
	
}

A_UINT32 swap_l(A_UINT32 num)
{
	return (((num & 0xff) << 24) | ((num & 0xff00) << 8) | ((num & 0xff0000) >> 8) | ((num & 0xff000000) >> 24));
	
}

A_UINT16 swap_s(A_UINT16 num)
{
	return (((num & 0xff) << 8) | ((num & 0xff00) >> 8));
	
}

// swap all the data in the memory block pointed by src
// and write the swapped data in dest
void swapAndCopyBlock_l(void *dest,void *src,A_UINT32 size)
{
	A_INT32 i;
	A_INT32 noWords;
	A_UINT32 *srcPtr;
	A_UINT32 *destPtr;
			
		noWords = size / sizeof(A_UINT32);
		srcPtr = (A_UINT32 *)src;
		destPtr = (A_UINT32 *)dest;
			
		for (i = 0;i < noWords; i++)
		{
			*destPtr = swap_l(*srcPtr);
			srcPtr++;
			destPtr++;
		}

	return;
	
}

// swap all the data in the memory block pointed by src
void swapBlock_l(void *src,A_UINT32 size)
{
	A_INT32 i;
	A_INT32 noWords;
	A_UINT32 *srcPtr;
				
		noWords = size / sizeof(A_UINT32);
		srcPtr = (A_UINT32 *)src;
			
        for (i = 0;i < noWords; i++)
		{
			*srcPtr = swap_l(*srcPtr);
			srcPtr++;
		}

	return;
	
}
