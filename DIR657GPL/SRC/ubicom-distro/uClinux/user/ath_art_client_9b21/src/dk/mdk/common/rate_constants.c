#ifdef __ATH_DJGPPDOS__
#include <unistd.h>
#ifndef EILSEQ
    #define EILSEQ EIO
#endif  // EILSEQ

 #define __int64    long long
 #define HANDLE long
 typedef unsigned long DWORD;
 #define Sleep  delay
 #include <bios.h>
 #include <dir.h>
#endif  // #ifdef __ATH_DJGPPDOS__

#include "wlantype.h"
#include "athreg.h"
#include "manlib.h"
#include "rate_constants.h"

const A_UCHAR rateValues[numRateCodes] = {
// 00-07   6   9  12  18 24  36 48  54
          11, 15, 10, 14, 9, 13, 8, 12,
// 08-14  1L   2L   2S   5.5L 5.5S 11L  11S
          0x1b,0x1a,0x1e,0x19,0x1d,0x18,0x1c,
// 15-23  0.25 0.5 1  2  3
          3,   7,  2, 6, 1, 0, 0, 0, 0,
// 24-31
    0, 0, 0, 0, 0, 0, 0, 0,
// 32-43  MCS 20 rates 0 - 15
          0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
          0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
// 44-63  MCS 40 rates 0 - 15
          0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
          0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f
};

const A_UCHAR rateCodes[numRateCodes] =  {
    6,    9,    12,   18,  24,   36,   48,    54,
    0xb1, 0xb2, 0xd2, 0xb5, 0xd5, 0xbb, 0xdb,
//  XR0.25 XR0.5  XR1   XR2   XR3
    0xea, 0xeb,  0xe1, 0xe2, 0xe3, 0, 0, 0, 0,
    0, 0 ,0 ,0 ,0, 0, 0, 0,
//  MCS 20 rates 0 - 15
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
//  MCS 40 rates 0 - 15 - same rate encoding though 40 bit is also set in descriptor
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f
};

A_UINT32 rate2bin(A_UINT32 rateCode) {
    A_UINT32 rateBin = UNKNOWN_RATE_CODE, i;

    if (rateCode == 0) {
        // provides access to generic stats bin for all rates
        return 0;
    }
    for(i = 0; i < numRateCodes; i++) {
        if(rateCode == rateCodes[i]) {
            rateBin = i + 1;
            break;
        }
    }
    return rateBin;
}

A_UINT32 descRate2RateIndex(A_UINT32 descRateCode, A_BOOL ht40) {
    A_UINT32 rateBin = UNKNOWN_RATE_CODE, i;

    for(i = 0; i < numRateCodes; i++) {
        if(descRateCode == rateValues[i]) {
            rateBin = i + 1;
            break;
        }
    }
    /* HT20 & 40 share same code, extra bit decides if it's a 40 rate */
    if ((i >= 32) && (i <= 47) && ht40) {
        rateBin += 16;
    }
    return rateBin;
}

A_UINT32 descRate2bin(A_UINT32 descRateCode) {
	A_UINT32 rateBin = 0, i;
	//A_UCHAR rateValues[]={6, 10};
	for(i = 0; i < NUM_RATES; i++) {
		if(descRateCode == rateValues[i]) {
			rateBin = i+1;
			break;
		}
	}
	return rateBin;
}