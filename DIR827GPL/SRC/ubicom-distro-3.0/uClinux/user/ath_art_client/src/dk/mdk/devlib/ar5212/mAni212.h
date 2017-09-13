/*
 *  Copyright © 2003 Atheros Communications, Inc.,  All Rights Reserved.
 *
 *  mAni(5)212.h - Type definitions needed for device specific ANI functions 
 */

#ifndef	_MANI212_H
#define	_MANI212_H

A_BOOL configArtAniLadderAr5212
(
 A_UINT32 devNum,
 A_UINT32 artAniType		// NI/BI/SI
);

A_BOOL	enableArtAniAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
);

A_BOOL	disableArtAniAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
);

A_BOOL	setArtAniLevelAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType, 
 A_UINT32 artAniLevel
);

A_BOOL	setArtAniLevelMaxAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
);

A_BOOL	setArtAniLevelMinAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
);

A_BOOL	incrementArtAniLevelAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
);

A_BOOL	decrementArtAniLevelAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
);

A_BOOL	programCurrArtAniLevelAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
);

A_UINT32 getArtAniLevelAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
);

A_BOOL measArtAniFalseDetectsAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
);

A_BOOL isArtAniOptimizedAr5212
(
 A_UINT32 devNum, 
 A_UINT32 artAniFalseDetectType   // ofdm or cck
);

A_UINT32 getArtAniFalseDetectsAr5212
(A_UINT32 devNum, 
 A_UINT32 artAniType
);


A_BOOL setArtAniFalseDetectIntervalAr5212
(
 A_UINT32 devNum,
 A_UINT32 artAniFalseDetectInterval		// in millisecond
);

#endif //_MCFG212D_H
