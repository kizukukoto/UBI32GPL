/* mDevtbl.c - contians configuration table for all the devices supported by devlib */
/* Copyright (c) 2001 Atheros Communications, Inc., All Rights Reserved */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/devlib/mDevtbl.c#201 $, $Header: //depot/sw/src/dk/mdk/devlib/mDevtbl.c#201 $"

/* 
Revsision history
--------------------
1.0       Created.
*/
#ifdef VXWORKS
#include "vxworks.h"
#endif

#include "wlantype.h"
#include "athreg.h"
#include "manlib.h"
#include "mDevtbl.h"
#include "mData210.h"
#include "mCfg210.h"
#include "mData211.h"
#include "mCfg211.h"
#include "mData212.h"
#include "mData513.h"
#include "mCfg212.h"
#include "mCfg212d.h"
#include "mAni212.h"
#include "mCfg413.h"
#include "mCfg5416.h"
#include "mData5416.h"
#ifdef LINUX 
#include "mCfg6000.h"
#endif
#include "mIds.h"

#ifndef MDK_AP
#ifndef CUSTOMER_REL
//static ATHEROS_REG_FILE ar5k0007_init[] = {
//#include "dk_crete_fez.ini"
//};
#endif //CUSTOMER_REL


//static ATHEROS_REG_FILE boss_0012[] = {	//new version 2 ini file
//#include "dk_boss_0012.ini"
//};
//static MODE_INFO boss_0012_mode[] = {	//new version 2 mode ini file
//#include "dk_boss_0012.mod"
//};

//static ATHEROS_REG_FILE venice[] = {	//new version 2 ini file
//#include "dk_boss_0013.ini"
//};
//static MODE_INFO venice_mode[] = {	//new version 2 mode ini file
//#include "dk_boss_0013.mod"
//};
//static ATHEROS_REG_FILE venice_derby[] = {	//new version 2 ini file
//#include "dk_0014.ini"
//};
//static MODE_INFO venice_derby_mode[] = {	//new version 2 mode ini file
//#include "dk_0014.mod"
//};
//static ATHEROS_REG_FILE venice_derby2_1_ear[] = {	//new version 2 ini file
//#include "dk_0016_2_1_ear.ini"
//};
//static MODE_INFO venice_derby2_1_mode_ear[] = {	//new version 2 mode ini file
//#include "dk_0016_2_1_ear.mod"
//};
static ATHEROS_REG_FILE venice_derby2_1[] = {	//new version 2 ini file
#include "dk_0016_2_1.ini"
};
static MODE_INFO venice_derby2_1_mode[] = {	//new version 2 mode ini file
#include "dk_0016_2_1.mod"
};
static ATHEROS_REG_FILE hainan_derby2_1[] = {	//new version 2 ini file
#include "dk_0017_2_1.ini"
};
static MODE_INFO hainan_derby2_1_mode[] = {	//new version 2 mode ini file
#include "dk_0017_2_1.mod"
};
static ATHEROS_REG_FILE hainan_derby2_1_ear[] = {	//new version 2 ini file
#include "dk_0017_2_1_ear.ini"
};
static MODE_INFO hainan_derby2_1_mode_ear[] = {	//new version 2 mode ini file
#include "dk_0017_2_1_ear.mod"
};
static ATHEROS_REG_FILE griffin2[] = {	//new version 2 ini file
#include "dk_0018_2.ini"
};
static MODE_INFO griffin2_mode[] = {	//new version 2 mode ini file
#include "dk_0018_2.mod"
};
static ATHEROS_REG_FILE eagle[] = {	//new version 2 ini file
#include "dk_0019.ini"
};
static MODE_INFO eagle_mode[] = {	//new version 2 mode ini file
#include "dk_0019.mod"
};
static ATHEROS_REG_FILE eagle2[] = {	//new version 2 ini file
#include "dk_0019_2.ini"
};
static MODE_INFO eagle2_mode[] = {	//new version 2 mode ini file
#include "dk_0019_2.mod"
};
//#ifdef PREDATOR_BUILD
static ATHEROS_REG_FILE predator_derby2_1[] = {	//new version 2 ini file
#include "dk_00b0_2_1.ini"
};
static MODE_INFO predator_derby2_1_mode[] = {	//new version 2 mode ini file
#include "dk_00b0_2_1.mod"
};
//#endif   // PREDATOR_BUILD

static ATHEROS_REG_FILE condor[] = {	//new version 2 ini file
#include "dk_0020.ini"
};
static MODE_INFO condor_mode[] = {	//new version 2 mode ini file
#include "dk_0020.mod"
};

static ATHEROS_REG_FILE owl_fowl[] = {
#include "dk_0023_1_0.ini"
};
static MODE_INFO owl_fowl_mode[] = {
#include "dk_0023_1_0.mod"
};

static ATHEROS_REG_FILE owl2_fowl[] = {
#include "dk_0023_2_0.ini"
//#include "dk_0023_2_0_perf.ini"
};
static MODE_INFO owl2_fowl_mode[] = {
#include "dk_0023_2_0.mod"
};

static ATHEROS_REG_FILE sowlEmulation_fowl[] = {
#include "dk_sowl_emulation.ini"
};
static MODE_INFO sowlEmulation_fowl_mode[] = {
#include "dk_sowl_emulation.mod"
};

static ATHEROS_REG_FILE sowl_fowl[] = {
#include "dk_0027.ini"
//#include "sowl_addac.ini"
};
static MODE_INFO sowl_fowl_mode[] = {
#include "dk_0027.mod"
};

static ATHEROS_REG_FILE nala[] = {	//  new version 2 ini file
#include "dk_0026.ini"
};
static MODE_INFO nala_mode[] = {	//  new version 2 mode ini file
#include "dk_0026.mod"
};

static ATHEROS_REG_FILE merlin[] = {
#include "dk_0029.ini"
};
static MODE_INFO merlin_mode[] = {
#include "dk_0029.mod"
};
static ATHEROS_REG_FILE kite[] = {
#include "dk_002b.ini"
};
static MODE_INFO kite_mode[] = {
#include "dk_002b.mod"
};

static ATHEROS_REG_FILE kite1_1[] = {
#include "dk_002b.ini"
};
static MODE_INFO kite_mode1_1[] = {
#include "dk_002b.mod"
};

static ATHEROS_REG_FILE kite1_2[] = {
#include "dk_002b_2.ini"
};
static MODE_INFO kite_mode1_2[] = {
#include "dk_002b_2.mod"
};

static ATHEROS_REG_FILE merlin2_0[] = {
#include "dk_0029_2_0.ini"
};
static MODE_INFO merlin2_0_mode[] = {
#include "dk_0029_2_0.mod"
};

static ATHEROS_REG_FILE kiwi1_0[] = {
#include "dk_002d.ini"
};
static MODE_INFO kiwi1_0_mode[] = {
#include "dk_002d.mod"
};

static ATHEROS_REG_FILE kiwi1_1[] = {
#include "dk_002e.ini"
};
static MODE_INFO kiwi1_1_mode[] = {
#include "dk_002e.mod"
};

static ATHEROS_REG_FILE kiwi1_2[] = {
#include "dk_002e_2.ini"
};
static MODE_INFO kiwi1_2_mode[] = {
#include "dk_002e_2.mod"
};

#ifndef CUSTOMER_REL
static ATHEROS_REG_FILE kiwi_Merlin_Emulation[] = {
#include "dk_kiwi_merlin_emulation.ini"
};
static MODE_INFO kiwi_Merlin_Emulation_mode[] = {
#include "dk_kiwi_merlin_emulation.mod"
};
#endif //CUSTOMER_REL

#ifndef CUSTOMER_REL
static ATHEROS_REG_FILE merlinEmulation_fowl[] = {
#include "dk_merlin_fowl_emulation.ini"
};
static MODE_INFO merlinEmulation_fowl_mode[] = {
#include "dk_merlin_fowl_emulation.mod"
};
#endif //CUSTOMER_REL

#ifdef LINUX
static ATHEROS_REG_FILE dragon[] = {	//new version 2 ini file
#include "dk_0022.ini"
};
static MODE_INFO dragon_mode[] = {	//new version 2 mode ini file
#include "dk_0022.mod"
};
#endif

#ifndef CUSTOMER_REL
//static ATHEROS_REG_FILE venice_emul[] = {	//new version 2 ini file
//#include "dk_venice_emul.ini"
//};
//static MODE_INFO venice_emul_mode[] = {	//new version 2 mode ini file
//#include "dk_venice_emul.mod"
//};
//
//static ATHEROS_REG_FILE hainan_sb_emul[] = {	//new version 2 ini file
//#include "dk_hainan_sb_emul.ini"
//};
//static MODE_INFO hainan_sb_emul_mode[] = {	//new version 2 mode ini file
//#include "dk_hainan_sb_emul.mod"
//};
//
//static ATHEROS_REG_FILE hainan_derby_emul[] = {	//new version 2 ini file
//#include "dk_hainan_derby_emul.ini"
//};
//static MODE_INFO hainan_derby_emul_mode[] = {	//new version 2 mode ini file
//#include "dk_hainan_derby_emul.mod"
//};

#endif //CUSTOMER_REL
#endif //MDK_AP

#ifdef AP11_AP
static ATHEROS_REG_FILE ar5k0007_init[] = {
#include "dk_crete_fez.ini"
};
#endif //AP11_AP

#ifdef SPIRIT_AP 
static ATHEROS_REG_FILE ar5k0011_spirit1_som2_e4[] = {
#include "dk_spirit1_som2_e4.ini"
};
#endif // SPIRIT_AP

#ifdef AP22_AP
static ATHEROS_REG_FILE boss_0012[] = {	//new version 2 ini file
#include "dk_boss_0012.ini"
};
static MODE_INFO boss_0012_mode[] = {	//new version 2 mode ini file
#include "dk_boss_0012.mod"
};
static ATHEROS_REG_FILE venice[] = {	//new version 2 ini file
#include "dk_boss_0013.ini"
};
static MODE_INFO venice_mode[] = {	//new version 2 mode ini file
#include "dk_boss_0013.mod"
};
static ATHEROS_REG_FILE venice_derby[] = {	//new version 2 ini file
#include "dk_0014.ini"
};
static MODE_INFO venice_derby_mode[] = {	//new version 2 mode ini file
#include "dk_0014.mod"
};
static ATHEROS_REG_FILE venice_derby2_1[] = {	//new version 2 ini file
#include "dk_0016_2_1.ini"
};
static MODE_INFO venice_derby2_1_mode[] = {	//new version 2 mode ini file
#include "dk_0016_2_1.mod"
};
static ATHEROS_REG_FILE venice_derby2_1_ear[] = {	//new version 2 ini file
#include "dk_0016_2_1_ear.ini"
};
static MODE_INFO venice_derby2_1_mode_ear[] = {	//new version 2 mode ini file
#include "dk_0016_2_1_ear.mod"
};
static ATHEROS_REG_FILE hainan_derby2_1[] = {	//new version 2 ini file
#include "dk_0017_2_1.ini"
};
static MODE_INFO hainan_derby2_1_mode[] = {	//new version 2 mode ini file
#include "dk_0017_2_1.mod"
};
static ATHEROS_REG_FILE hainan_derby2_1_ear[] = {	//new version 2 ini file
#include "dk_0017_2_1_ear.ini"
};
static MODE_INFO hainan_derby2_1_mode_ear[] = {	//new version 2 mode ini file
#include "dk_0017_2_1_ear.mod"
};
#endif //AP22_AP

#if (defined(FREEDOM_AP)||defined(THIN_CLIENT_BUILD))&&!defined(COBRA_AP)
//#if !defined(COBRA_AP)
//static ATHEROS_REG_FILE boss_0012[] = {	//new version 2 ini file
//#include "dk_boss_0012.ini"
//};
//static MODE_INFO boss_0012_mode[] = {	//new version 2 mode ini file
//#include "dk_boss_0012.mod"
//};
//static ATHEROS_REG_FILE freedom1_derby[] = {	//new version 2 ini file
//#include "dk_freedom1_derby.ini"
//};
//static MODE_INFO freedom1_derby_mode[] = {	//new version 2 mode ini file
//#include "dk_freedom1_derby.mod"
//};
//static ATHEROS_REG_FILE freedom2_derby[] = {	//new version 2 ini file
//#include "dk_freedom2_derby.ini"
//};
//static MODE_INFO freedom2_derby_mode[] = {	//new version 2 mode ini file
//#include "dk_freedom2_derby.mod"
//};
//static ATHEROS_REG_FILE freedom2_sombrero[] = {	//new version 2 ini file
//#include "dk_freedom2_sombrero.ini"
//};
//static MODE_INFO freedom2_sombrero_mode[] = {	//new version 2 mode ini file
//#include "dk_freedom2_sombrero.mod"
//};
static ATHEROS_REG_FILE freedom2_derby2_1[] = {	//new version 2 ini file
#include "dk_freedom2_derby2_1.ini"
};
static MODE_INFO freedom2_derby2_1_mode[] = {	//new version 2 mode ini file
#include "dk_freedom2_derby2_1.mod"
};
#ifndef SOC_LINUX
static ATHEROS_REG_FILE freedom2_derby2[] = {	//new version 2 ini file
#include "dk_freedom2_derby2.ini"
};
static MODE_INFO freedom2_derby2_mode[] = {	//new version 2 mode ini file
#include "dk_freedom2_derby2.mod"
};
static ATHEROS_REG_FILE viper_derby2_1[] = {	//new version 2 ini file
#include "dk_viper_derby2_1.ini"
};
static MODE_INFO viper_derby2_1_mode[] = {	//new version 2 mode ini file
#include "dk_viper_derby2_1.mod"
};

#if 0 // Uncomment it ...
static ATHEROS_REG_FILE freedom2_derby2_ear[] = {	//new version 2 ini file
#include "dk_freedom2_derby2_ear.ini"
};
static MODE_INFO freedom2_derby2_mode_ear[] = {	//new version 2 mode ini file
#include "dk_freedom2_derby2_ear.mod"
};
#endif
#endif
#endif

#ifdef SENAO_AP
static ATHEROS_REG_FILE venice_derby2_1_ear[] = {	//new version 2 ini file
#include "dk_0016_2_1_ear.ini"
};
static MODE_INFO venice_derby2_1_mode_ear[] = {	//new version 2 mode ini file
#include "dk_0016_2_1_ear.mod"
};
static ATHEROS_REG_FILE venice_derby2_1[] = {	//new version 2 ini file
#include "dk_0016_2_1.ini"
};
static MODE_INFO venice_derby2_1_mode[] = {	//new version 2 mode ini file
#include "dk_0016_2_1.mod"
};
static ATHEROS_REG_FILE hainan_derby2_1[] = {	//new version 2 ini file
#include "dk_0017_2_1.ini"
};
static MODE_INFO hainan_derby2_1_mode[] = {	//new version 2 mode ini file
#include "dk_0017_2_1.mod"
};
static ATHEROS_REG_FILE hainan_derby2_1_ear[] = {	//new version 2 ini file
#include "dk_0017_2_1_ear.ini"
};
static MODE_INFO hainan_derby2_1_mode_ear[] = {	//new version 2 mode ini file
#include "dk_0017_2_1_ear.mod"
};
static ATHEROS_REG_FILE griffin2[] = {	//new version 2 ini file
#include "dk_0018_2.ini"
};
static MODE_INFO griffin2_mode[] = {	//new version 2 mode ini file
#include "dk_0018_2.mod"
};
static ATHEROS_REG_FILE eagle2[] = {	//new version 2 ini file
#include "dk_0019_2.ini"
};
static MODE_INFO eagle2_mode[] = {	//new version 2 mode ini file
#include "dk_0019_2.mod"
};

#endif //SENAO_AP


#ifdef COBRA_AP

#ifdef SPIDER_AP

#ifdef VER1
static ATHEROS_REG_FILE spider[] = {	//new version 2 ini file
#include "dk_spider1_0.ini"
};
static MODE_INFO spider_mode[] = {	//new version 2 mode ini file
#include "dk_spider1_0.mod"
};
#endif

#ifdef VER2
static ATHEROS_REG_FILE spider2_0[] = {	//new version 2 ini file
#include "dk_spider2_0.ini"
};
static MODE_INFO spider2_0_mode[] = {	//new version 2 mode ini file
#include "dk_spider2_0.mod"
};
#endif

#else

static ATHEROS_REG_FILE cobra[] = {	//new version 2 ini file
#include "dk_cobra1_0.ini"
};
static MODE_INFO cobra_mode[] = {	//new version 2 mode ini file
#include "dk_cobra1_0.mod"
};

#endif

#ifdef PCI_INTERFACE
static ATHEROS_REG_FILE eagle2[] = {	//new version 2 ini file
#include "dk_0019_2.ini"
};
static MODE_INFO eagle2_mode[] = {	//new version 2 mode ini file
#include "dk_0019_2.mod"
};
#endif

#endif //COBRA_AP

#ifdef OWL_AP
#ifdef VER1
static ATHEROS_REG_FILE owl_fowl[] = {
#include "dk_0023_1_0.ini"
};
static MODE_INFO owl_fowl_mode[] = {
#include "dk_0023_1_0.mod"
};
#else //VER1
static ATHEROS_REG_FILE owl2_fowl[] = {
#include "dk_0023_2_0.ini"
};
static MODE_INFO owl2_fowl_mode[] = {
#include "dk_0023_2_0.mod"
};

static ATHEROS_REG_FILE nala[] = {	
#include "dk_0026.ini"
};
static MODE_INFO nala_mode[] = {	
#include "dk_0026.mod"
};

static ATHEROS_REG_FILE sowl_fowl[] = {
#include "dk_0027.ini"
};
static MODE_INFO sowl_fowl_mode[] = {
#include "dk_0027.mod"
};

static ATHEROS_REG_FILE merlin2_0[] = {
#include "dk_0029_2_0.ini"
};
static MODE_INFO merlin2_0_mode[] = {
#include "dk_0029_2_0.mod"
};

static ATHEROS_REG_FILE kite[] = {
#include "dk_002b.ini"
};
static MODE_INFO kite_mode[] = {
#include "dk_002b.mod"
};

static ATHEROS_REG_FILE kite1_1[] = {
#include "dk_002b.ini"
};
static MODE_INFO kite_mode1_1[] = {
#include "dk_002b.mod"
};

static ATHEROS_REG_FILE kite1_2[] = {
#include "dk_002b_2.ini"
};
static MODE_INFO kite_mode1_2[] = {
#include "dk_002b_2.mod"
};

static ATHEROS_REG_FILE kiwi1_1[] = {
#include "dk_002e.ini"
};
static MODE_INFO kiwi1_1_mode[] = {
#include "dk_002e.mod"
};

static ATHEROS_REG_FILE kiwi1_2[] = {
#include "dk_002e_2.ini"
};
static MODE_INFO kiwi1_2_mode[] = {
#include "dk_002e_2.mod"
};

#endif //VER1
#endif


#ifdef OWL_PB42
static ATHEROS_REG_FILE howl_fowl[] = {
#include "dk_a027.ini"
};

static MODE_INFO howl_fowl_mode[] = {
#include "dk_a027.mod"
};
#endif

static MAC_API_TABLE creteAPI = {
	macAPIInitAr5210,
	eepromReadAr5210,
	eepromWriteAr5210,

	hwResetAr5210,
	pllProgramAr5210,

	setRetryLimitAr5210,
	setupAntennaAr5210,
	sendTxEndPacketAr5210,
	setDescriptorAr5210,
	setStatsPktDescAr5210,
	setContDescriptorAr5210,
	txBeginConfigAr5210,
	txBeginContDataAr5210,
	txBeginContFramedDataAr5210,
	txEndContFramedDataAr5210,
	beginSendStatsPktAr5210,
	writeRxDescriptorAr5210,
	rxBeginConfigAr5210,
	rxCleanupConfigAr5210,
	txCleanupConfigAr5210,
	txGetDescRateAr5210,
	setPPM5210,
	isTxdescEvent5210,
	isRxdescEvent5210,
	isTxComplete5210,
	enableRx5210,
	disableRx5210,
	setQueueAr5210,
	mapQueueAr5210,
	clearKeyCacheAr5210,
	AGCDeafAr5210,
	AGCUnDeafAr5210
};

static MAC_API_TABLE maui1API = {
	macAPIInitAr5210,
	eepromReadAr5210,
	eepromWriteAr5210,

	hwResetAr5210,
	pllProgramAr5210,

	setRetryLimitAr5210,
	setupAntennaAr5210,
	sendTxEndPacketAr5210,
	setDescriptorAr5210,
	setStatsPktDescAr5210,
	setContDescriptorAr5210,
	txBeginConfigAr5210,
	txBeginContDataAr5210,
	txBeginContFramedDataAr5210,
	txEndContFramedDataAr5210,
	beginSendStatsPktAr5210,
	writeRxDescriptorAr5210,
	rxBeginConfigAr5210,
	rxCleanupConfigAr5210,
	txCleanupConfigAr5210,
	txGetDescRateAr5210,
	setPPM5210,
	isTxdescEvent5210,
	isRxdescEvent5210,
	isTxComplete5210,
	enableRx5210,
	disableRx5210,
	setQueueAr5210,
	mapQueueAr5210,
	clearKeyCacheAr5210,
	AGCDeafAr5210,
	AGCUnDeafAr5210
};

static MAC_API_TABLE maui2API = {
	macAPIInitAr5211,
	eepromReadAr5211,		
	eepromWriteAr5211,

	hwResetAr5211,
	pllProgramAr5211,
	
	setRetryLimitAllAr5211,
	setupAntennaAr5211,
	sendTxEndPacketAr5211,
	setDescriptorAr5211,
	setStatsPktDescAr5211,
	setContDescriptorAr5211,
	txBeginConfigAr5211,
	txBeginContDataAr5211,
	txBeginContFramedDataAr5211,
	txEndContFramedDataAr5211,
	beginSendStatsPktAr5211,
	writeRxDescriptorAr5211,
	rxBeginConfigAr5211,
	rxCleanupConfigAr5211,
	txCleanupConfigAr5211,
	txGetDescRateAr5211,
	setPPM5211,
	isTxdescEvent5211,
	isRxdescEvent5211,
	isTxComplete5211,
	enableRx5211,
	disableRx5211,
	setQueueAr5211,
	mapQueueAr5211,
	clearKeyCacheAr5211,
	AGCDeafAr5211,
	AGCUnDeafAr5211
};

static MAC_API_TABLE veniceAPI = {
	macAPIInitAr5212,
	eepromReadAr5211,		
	eepromWriteAr5211,

	hwResetAr5211,
	pllProgramAr5212,
	
	setRetryLimitAllAr5211,
	setupAntennaAr5211,
	sendTxEndPacketAr5211,
	setDescriptorAr5212,
	setStatsPktDescAr5212,
	setContDescriptorAr5212,
	txBeginConfigAr5211,
	txBeginContDataAr5211,
	txBeginContFramedDataAr5211,
	txEndContFramedDataAr5211,
	beginSendStatsPktAr5211,
	writeRxDescriptorAr5211,
	rxBeginConfigAr5212,
	rxCleanupConfigAr5211,
	txCleanupConfigAr5211,
	txGetDescRateAr5212,
	setPPM5211,
	isTxdescEvent5211,
	isRxdescEvent5211,
	isTxComplete5211,
	enableRx5211,
	disableRx5211,
	setQueueAr5211,
	mapQueueAr5211,
	clearKeyCacheAr5211,
	AGCDeafAr5211,
	AGCUnDeafAr5211
};

//changing pll programming
static MAC_API_TABLE eagleAPI = {
	macAPIInitAr5212,
	eepromReadAr5211,		
	eepromWriteAr5211,

	hwResetAr5211,
	pllProgramAr5413,
	
	setRetryLimitAllAr5211,
	setupAntennaAr5211,
	sendTxEndPacketAr5211,
	setDescriptorAr5212,
	setStatsPktDescAr5212,
	setContDescriptorAr5212,
	txBeginConfigAr5211,
	txBeginContDataAr5211,
	txBeginContFramedDataAr5211,
	txEndContFramedDataAr5211,
	beginSendStatsPktAr5211,
	writeRxDescriptorAr5211,
	rxBeginConfigAr5212,
	rxCleanupConfigAr5211,
	txCleanupConfigAr5211,
	txGetDescRateAr5212,
	setPPM5211,
	isTxdescEvent5211,
	isRxdescEvent5211,
	isTxComplete5211,
	enableRx5211,
	disableRx5211,
	setQueueAr5211,
	mapQueueAr5211,
	clearKeyCacheAr5211,
	AGCDeafAr5211,
	AGCUnDeafAr5211
};
#ifdef LINUX
#ifndef SOC_LINUX
// Different PLL, synth, eep, descs...
static MAC_API_TABLE dragonAPI = {
	macAPIInitAr5513,
	eepromReadAr5416,		
	eepromWriteAr5416,

	hwResetAr5211,
	pllProgramAr5212, // Call down to thin-client ala predator
	
	setRetryLimitAllAr5211,
	setupAntennaAr5513,
	sendTxEndPacketAr5513,
	setDescriptorAr5513,
	setStatsPktDescAr5513,
	setContDescriptorAr5513,
	txBeginConfigAr5513,
	txBeginContDataAr5513,
	txBeginContFramedDataAr5513,
	txEndContFramedDataAr5211,
	beginSendStatsPktAr5513,
	writeRxDescriptorAr5513,
	rxBeginConfigAr5513,
	rxCleanupConfigAr5513,
	txCleanupConfigAr5211,
	txGetDescRateAr5212,
	setPPM5513,
	isTxdescEvent5211,
	isRxdescEvent5211,
	isTxComplete5211,
	enableRx5211,
	disableRx5211,
	setQueueAr5211,
	mapQueueAr5211,
	clearKeyCacheAr5211,
	AGCDeafAr5211,
	AGCUnDeafAr5211
};
#endif
#endif

static MAC_API_TABLE owl_fowlAPI = {
	macAPIInitAr5416,
	eepromReadAr5416,		
	eepromWriteAr5416,

	hwResetAr5416,
	pllProgramAr5416,
	
	setRetryLimitAllAr5211,
	setupAntennaAr5416,

	sendTxEndPacketAr5416,
	setDescriptorAr5416,
	setStatsPktDescAr5416,
	setContDescriptorAr5416,
	txBeginConfigAr5416,
	txBeginContDataAr5416,
	txBeginContFramedDataAr5416,
	txEndContFramedDataAr5211,
	beginSendStatsPktAr5416,
	writeRxDescriptorAr5416,
	rxBeginConfigAr5416,
	rxCleanupConfigAr5416,
	txCleanupConfigAr5211,
	txGetDescRateAr5416,
	setPPM5416,
	isTxdescEvent5211,
	isRxdescEvent5211,
	isTxComplete5211,
	enableRx5211,
	disableRx5211,
	setQueueAr5211,
	mapQueueAr5211,
	clearKeyCacheAr5211,
	AGCDeafAr5211,
	AGCUnDeafAr5211
};

static RF_API_TABLE owl_fowlRfAPI = {
	initPowerAr5416,
	setSinglePowerAr5211,
	setChannelAr5416
};

static RF_API_TABLE merlinRfAPI = {
	initPowerAr5416,
	setSinglePowerAr5211,
	setChannelAr928x
};

static RF_API_TABLE kiwiRfAPI = {
	initPowerAr5416,
	setSinglePowerAr5211,
	setChannelSwCfgFilter
};

static RF_API_TABLE fezAPI = {
	initPowerAr5210,
	setSinglePowerAr5210,
	setChannelAr5210
};

static RF_API_TABLE sombreroAPI = {
	initPowerAr5211,
	setSinglePowerAr5211,
	setChannelAr5211
};

static RF_API_TABLE sombrero_beanieAPI = {
	initPowerAr5211,
	setSinglePowerAr5211,
	setChannelAr5211_beanie
};

static RF_API_TABLE derbyAPI = {
	initPowerAr5212,
	setSinglePowerAr5211,
	setChannelAr5212
};

static RF_API_TABLE griffinRfAPI = {
	initPowerAr2413,
	setSinglePowerAr5211,
	setChannelAr2413
};
#ifdef LINUX
#ifndef SOC_LINUX

#ifdef LINUX
static RF_API_TABLE dragonRfAPI = {
	initPowerAr6000,
	setSinglePowerAr5211,
	setChannelAr6000
};
#endif
#endif
#endif
static ART_ANI_API_TABLE veniceArtAniAPI = {
	configArtAniLadderAr5212,
	enableArtAniAr5212,
	disableArtAniAr5212,
	setArtAniLevelAr5212,
	setArtAniLevelMaxAr5212,
	setArtAniLevelMinAr5212,
	incrementArtAniLevelAr5212,
	decrementArtAniLevelAr5212,
	getArtAniLevelAr5212,
	measArtAniFalseDetectsAr5212,
	isArtAniOptimizedAr5212,
	getArtAniFalseDetectsAr5212,
	setArtAniFalseDetectIntervalAr5212,
	programCurrArtAniLevelAr5212
};

ANALOG_REV fezRevs[] = { 
	{0, 0x8},
    {0, 0x9}
};
const A_UINT16 numFezRevs = sizeof(fezRevs)/sizeof(ANALOG_REV);

ANALOG_REV sombreroRevs[] = {
	{1, 0x5},
	{1, 0x6},
	{1, 0x7}
};
const A_UINT16 numSombreroRevs = sizeof(sombreroRevs)/sizeof(ANALOG_REV);

ANALOG_REV derby1Revs[] = {
	{3, 0x1},
	{3, 0x2}
};
const A_UINT16 numDerby1Revs = sizeof(derby1Revs)/sizeof(ANALOG_REV);

ANALOG_REV derby1_2Revs[] = {
	{3, 0x3},
	{3, 0x4},
//	{0, 0} 
};
const A_UINT16 numderby1_2Revs = sizeof(derby1_2Revs)/sizeof(ANALOG_REV);

ANALOG_REV derby2Revs[] = {
	{3, 0x5},
	{4, 0x5},
//	{3, 0x6},
//	{4, 0x6},
	{0, 0}   // dummy to handle AP31 with no 5G derby
};
const A_UINT16 numderby2Revs = sizeof(derby2Revs)/sizeof(ANALOG_REV);

ANALOG_REV derby2_1Revs[] = {
	{3, 0x6},
	{4, 0x6},
	{0, 0}   // dummy to handle AP31 with no 5G derby
};
const A_UINT16 numderby2_1Revs = sizeof(derby2_1Revs)/sizeof(ANALOG_REV);

ANALOG_REV fowlRevs[] = {
	{0xC, 0x0},  // 5133 3x3 a/g
	{0xD, 0x0},  // 2133 3x3 g
	{0xE, 0x0},  // 5122 2x2 a/g
	{0xF, 0x0},  // 2122 2x2 g
    {0x0, 0x0},  // for emulation
};
const A_UINT16 numFowlRevs = sizeof(fowlRevs)/sizeof(ANALOG_REV);

A_UINT32 veniceRevs[] = {
	0x50, 0x51, 0x53, 0x56
};

const A_UINT16 numVeniceRevs = sizeof(veniceRevs)/sizeof(A_UINT32);

A_UINT32 predatorRevs[] = {
	0x00,  // For emulation
	0x80,  // Predator 1.0
	0x81,  // Predator 1.1
};

const A_UINT16 numPredatorRevs = sizeof(predatorRevs)/sizeof(A_UINT32);

A_UINT32 hainanRevs[] = {
	0x55,
	0x59
};
const A_UINT16 numHainanRevs = sizeof(hainanRevs)/sizeof(A_UINT32);

A_UINT32 griffinRevs[] = {
	0x74,  // griffin 1.0
    0x75,  // griffin 1.1
    0x76,  // griffin 2.0
	0x78,  //griffin lite
	0x79,   //griffin 2.1
//	0xa0,   //special build additions
	0x00
};
const A_UINT16 numGriffinRevs = sizeof(griffinRevs)/sizeof(A_UINT32);

A_UINT32 falconRevs[] = {
	0x60,
	0x61,
	0x62
};
const A_UINT16 numFalconRevs = sizeof(falconRevs)/sizeof(A_UINT32);

A_UINT32 eagleRevs[] = {
	0xa0,   //eagle 1.0
	0xa1,  // eagle 1.0
	0x00,
};

const A_UINT16 numEagleRevs = sizeof(eagleRevs)/sizeof(A_UINT32);

A_UINT32 eagle2Revs[] = {
	0xa2,  // eagle 2.0
	0xa3,  // eagle 2.0
	0xa4,  // eagle 2.1 lite
	0xa5,  // eagle 2.1 super
};

const A_UINT16 numEagle2Revs = sizeof(eagle2Revs)/sizeof(A_UINT32);

A_UINT32 cobraRevs[] = {
	0xb0,  // cobra 1.0
};

A_UINT32 dragonRevs[] = {
	0x00,  // dragon 1.0
};
const A_UINT16 numDragonRevs = sizeof(dragonRevs)/sizeof(A_UINT32);

ANALOG_REV griffinAnalogRevs[] = {
	{5, 0x1},
	{5, 0x2},
};
const A_UINT16 numGriffinAnalogRevs = sizeof(griffinAnalogRevs)/sizeof(ANALOG_REV);

ANALOG_REV griffin1_1_AnalogRevs[] = {
	{5, 0x3},
	{5, 0x4},
};
const A_UINT16 numGriffin1_1_AnalogRevs = sizeof(griffin1_1_AnalogRevs)/sizeof(ANALOG_REV);

ANALOG_REV griffin2_AnalogRevs[] = {
	{5, 0x5},
	{5, 0x6},
};
const A_UINT16 numGriffin2_AnalogRevs = sizeof(griffin2_AnalogRevs)/sizeof(ANALOG_REV);

ANALOG_REV eagleAnalogRevs[] = {
	{6, 0x0},
	{6, 0x1},
	{6, 0x2},
	{6, 0x3},
};
const A_UINT16 numEagleAnalogRevs = sizeof(eagleAnalogRevs)/sizeof(ANALOG_REV);

ANALOG_REV eagle2AnalogRevs[] = {
	{6, 0x2},
	{6, 0x3},
	{0xb, 0x0},
};
const A_UINT16 numEagle2AnalogRevs = sizeof(eagle2AnalogRevs)/sizeof(ANALOG_REV);

ANALOG_REV cobraAnalogRevs[] = {
	{7, 0x0},
};

ANALOG_REV spiderAnalogRevs[] = {
	{8, 0x3},
	{8, 0x4},
	{8, 0x5},
};

ANALOG_REV dragonAnalogRevs[] = {
	{9, 0x0},
};
const A_UINT16 numDragonAnalogRevs = sizeof(dragonAnalogRevs)/sizeof(ANALOG_REV);

ANALOG_REV swanAnalogRevs[] = {	
    {0, 0},
};

const A_UINT16 numSwanAnalogRevs = sizeof(swanAnalogRevs)/sizeof(ANALOG_REV);

A_UINT32 condorRevs[] = {
	0x92,  //condor 1.0 lite
	0x93,  //condor 1.0 full
	0x9a,  //condor 2.0 lite
	0x9b,  //condor 2.0 full
};

const A_UINT16 numCondorRevs = sizeof(condorRevs)/sizeof(A_UINT32);

ANALOG_REV condorAnalogRevs[] = {
	{6, 0x3},
	{7, 0x1},
	{0xa, 0x2}, //condor 2.0
};
const A_UINT16 numCondorAnalogRevs = sizeof(condorAnalogRevs)/sizeof(ANALOG_REV);

A_UINT32 nalaRevs[] = {
	0xf0
};

const A_UINT16 numNalaRevs = sizeof(nalaRevs) / sizeof(A_UINT32);


A_UINT32 owlRevs[] = {
    0xd8,  // Owl 1.0 PCI 3x3
    0xd0,  // Owl 1.0 PCI 2x2
    0xc8,  // Owl 1.0 PCIE 3x3
    0xd8,  // Owl 1.0 PCIE 2x2
};
const A_UINT16 numOwlRevs = sizeof(owlRevs)/sizeof(A_UINT32);

A_UINT32 owl2Revs[] = {
    0xd9,  // Owl 2.1 PCI
    0xc9,  // Owl 2.1 PCIE
    0xda,  // Owl 2.2 PCI
    0xca,  // Owl 2.2 PCIE
};


A_UINT32 howlRevs[] = {
    0x140,  // Howl 1.0
};



const A_UINT16 numOwl2Revs = sizeof(owl2Revs)/sizeof(A_UINT32);
const A_UINT16 numHowlRevs = sizeof(howlRevs)/sizeof(A_UINT32);

A_UINT32 sowlEmulationRevs[] = {
    0x4f0ff,  // SOWL Emulation board
};
const A_UINT16 numSowlEmulationRevs = sizeof(sowlEmulationRevs)/sizeof(A_UINT32);

A_UINT32 sowlRevs[] = {
    0x000400ff,  // sowl PCI 2 chains
    0x000410ff,  // sowl PCI 3 chains
    0x000420ff,  // sowl PCIE 2 chains
    0x000430ff,  // sowl PCIE 3 chains
    0x000401ff,  // sowl 1.1 PCI 2 chains
    0x000411ff,  // sowl 1.1 PCI 3 chains
    0x000421ff,  // sowl 1.1 PCIE 2 chains
    0x000431ff,  // sowl 1.1 PCIE 3 chains
};
const A_UINT16 numSowlRevs = sizeof(sowlRevs)/sizeof(A_UINT32);

A_UINT32 merlinFowlEmulationRevs[] = {
    0x8f0ff,  // Merlin_Fowl Emulation board
};
const A_UINT16 numMerlinFowlEmulationRevs = sizeof(merlinFowlEmulationRevs)/sizeof(A_UINT32);

/* analog rev info
 * Merlin regs are only 3 bits as oppossed to the historical 8 bits and so they 
 * no longer have a product ID.  In order not to create another struct to just
 * keep using ANALOG_REV and ignore the product id
 */
ANALOG_REV merlinAnalogRevs[] = {
    {0, 0x0}, /* merlin  */
    {0, 0x1},  /* merlinX */
    {0, 0x2},  /* merlin 2.0 */
    {0, 0x3},  /* merlin 2.1 */
};
const A_UINT16 numMerlinAnalogRevs = sizeof(merlinAnalogRevs)/sizeof(ANALOG_REV);

/* mac rev info for Merlin 1.0 */
A_UINT32 merlinRevs[] = {
    0x000800ff,  /* merlin PCIE 1 chain single band  */
    0x000810ff,  /* merlin PCIE 2 chains single band */
    0x000820ff,  /* merlin PCI 1 chain single band   */
    0x000830ff,  /* merlin PCI 2 chains single band  */
    0x000840ff,  /* merlin PCI 1 chain dual band     */
    0x000850ff,  /* merlin PCI 2 chains dual band    */
    0x000860ff,  /* merlin PCIE 1 chain dual band    */
    0x000870ff,  /* merlin PCIE 2 chains dual band   */
};
const A_UINT16 numMerlinRevs = sizeof(merlinRevs)/sizeof(A_UINT32);

/* mac rev info for Merlin 2.0 */
A_UINT32 merlin2_0MacRevs[] = {
    0x000801ff,  /* merlin PCIE 1 chain single band  */
    0x000811ff,  /* merlin PCIE 2 chains single band */
    0x000821ff,  /* merlin PCI 1 chain single band   */
    0x000831ff,  /* merlin PCI 2 chains single band  */
    0x000841ff,  /* merlin PCIE 1 chain dual band     */
    0x000851ff,  /* merlin PCIE 2 chains dual band    */
    0x000861ff,  /* merlin PCI 1 chain dual band    */
    0x000871ff,  /* merlin PCI 2 chains dual band   */

    /* merlin 2.1 */
    0x000802ff,  /* merlin PCIE 1 chain single band  */
    0x000812ff,  /* merlin PCIE 2 chains single band */
    0x000822ff,  /* merlin PCI 1 chain single band   */
    0x000832ff,  /* merlin PCI 2 chains single band  */
    0x000842ff,  /* merlin PCIE 1 chain dual band     */
    0x000852ff,  /* merlin PCIE 2 chains dual band    */
    0x000862ff,  /* merlin PCI 1 chain dual band    */
    0x000872ff,  /* merlin PCI 2 chains dual band   */
};

const A_UINT16 numMerlin2_0MacRevs = sizeof(merlin2_0MacRevs)/sizeof(A_UINT32);
/*====================Kite===============================*/

ANALOG_REV KiteAnalogRevs[] = {
    {0, 0x0},
    {0, 0x1},
    {0, 0x2},
    {0, 0x3},
};

//const A_UINT16 numKiteAnalogRevs = sizeof(KiteAnalogRevs)/sizeof(ANALOG_REV);
A_UINT16 numKiteAnalogRevs = sizeof(KiteAnalogRevs)/sizeof(ANALOG_REV);

A_UINT32 KiteMacRevs[] = {
		0x000c10ff,
};

//const A_UINT16 numKiteMacRevs = sizeof(KiteMacRevs)/sizeof(A_UINT32);
A_UINT16 numKiteMacRevs = sizeof(KiteMacRevs)/sizeof(A_UINT32);

A_UINT32 Kite1_1MacRevs[] = {
		0x000c11ff,
};

const A_UINT16 numKite1_1MacRevs = sizeof(Kite1_1MacRevs)/sizeof(A_UINT32);

A_UINT32 Kite1_2MacRevs[] = {
	0x000c12ff,
};

const A_UINT16 numKite1_2MacRevs = sizeof(Kite1_2MacRevs)/sizeof(A_UINT32);

/******************** Begin KIWI revs *****************************/
/* analog rev info */
ANALOG_REV kiwiAnalogRevs[] = {
    {0, 0x0},  /* kiwi 1.0, rev id is a 3-bit field  */
    {0, 0x1},  /* kiwi 1.1, rev id is a 3-bit field  */
};
const A_UINT16 numKiwiAnalogRevs = sizeof(kiwiAnalogRevs)/sizeof(ANALOG_REV);

/* mac rev info for Kiwi 1.0 */
A_UINT32 kiwi1_0MacRevs[] = {
    0x001810ff,  /* kiwi PCIE, 2 chain, single band */
    0x001830ff   /* kiwi PCI,  2 chain, single band */
};
const A_UINT16 numKiwi1_0MacRevs = sizeof(kiwi1_0MacRevs)/sizeof(A_UINT32);

/* mac rev info for Kiwi 1.1 */
A_UINT32 kiwi1_1MacRevs[] = {
    0x001811ff,  /* kiwi PCIE, 2 chain, single band */
    0x001831ff   /* kiwi PCI,  2 chain, single band */
};
const A_UINT16 numKiwi1_1MacRevs = sizeof(kiwi1_1MacRevs)/sizeof(A_UINT32);

/* mac rev info for Kiwi 1.2 */
A_UINT32 kiwi1_2MacRevs[] = {
    0x001802FF,   // Kiwi 1.2 single band PCIE 1 chain
    0x001812FF,   // Kiwi 1.2 single band PCIE 2 chain
    0x001822FF,   // Kiwi 1.2 single band PCI 1 chain
    0x001832FF    // Kiwi 1.2 single band PCI 2 chain
};
const A_UINT16 numKiwi1_2MacRevs = sizeof(kiwi1_2MacRevs)/sizeof(A_UINT32);

/* mac rev info for Kiwi_Merlin Emulation */
A_UINT32 kiwi_Merlin_EmulationMacRevs[] = {
    0x000e10ff  /* kiwi PCIE, 2 chain, single band */
};
const A_UINT16 numKiwi_Merlin_EmulationMacRevs = sizeof(kiwi_Merlin_EmulationMacRevs)/sizeof(A_UINT32);

/******************** End KIWI revs *****************************/

A_UINT32 Kite11g[] = {
	0x000c02ff, // for kite 11g only
};

const A_UINT16 numKite11g = sizeof(Kite11g)/sizeof(A_UINT32);


DEVICE_INIT_DATA ar5kInitData[] = {
#ifndef MDK_AP
//	{0x0012, NULL, 0, NULL, 0, SW_DEVICE_ID_BOSS_0012,								//Identifiers
//	 boss_0012, sizeof(boss_0012)/sizeof(ATHEROS_REG_FILE),			//register file
//	 &maui2API, &sombrero_beanieAPI, &veniceArtAniAPI,				//APIs
//	 2, boss_0012_mode, sizeof(boss_0012_mode)/sizeof(MODE_INFO),   //Mode file
//	 CFG_VERSION_STRING_0012 },										//configuraton string

#ifndef CUSTOMER_REL
//	{0x1107, fezRevs, numFezRevs, NULL, 0, SW_DEVICE_ID_CRETE_FEZ, 
//	 ar5k0007_init, sizeof(ar5k0007_init)/sizeof(ATHEROS_REG_FILE), 
//	 &creteAPI, &fezAPI, &veniceArtAniAPI,	0, NULL, 0, NULL },

//	{0x0105, fezRevs, numFezRevs, NULL, 0, SW_DEVICE_ID_CRETE_FEZ, 
//	 ar5k0007_init, sizeof(ar5k0007_init)/sizeof(ATHEROS_REG_FILE), 
//	 &creteAPI, &fezAPI, &veniceArtAniAPI,	0, NULL, 0, NULL },

//	{0x0107, fezRevs, numFezRevs, NULL, 0, SW_DEVICE_ID_CRETE_FEZ, 
//	 ar5k0007_init, sizeof(ar5k0007_init)/sizeof(ATHEROS_REG_FILE), 
//	 &creteAPI, &fezAPI, &veniceArtAniAPI,	0, NULL, 0, NULL },

//	{0x0007, fezRevs, numFezRevs, NULL, 0, SW_DEVICE_ID_CRETE_FEZ, 
//	 ar5k0007_init, sizeof(ar5k0007_init)/sizeof(ATHEROS_REG_FILE), 
//	 &creteAPI, &fezAPI, &veniceArtAniAPI,	0, NULL, 0, NULL },

//	{0x0000, fezRevs, numFezRevs, NULL, 0, SW_DEVICE_ID_CRETE_FEZ, 
//	 ar5k0007_init, sizeof(ar5k0007_init)/sizeof(ATHEROS_REG_FILE), 
//	 &creteAPI, &fezAPI, &veniceArtAniAPI,	0, NULL, 0, NULL },
#endif //CUSTOMER_REL

//	{0xff12, NULL, 0, NULL, 0, SW_DEVICE_ID_BOSS_0012,						    	//Identifiers
//	 boss_0012, sizeof(boss_0012)/sizeof(ATHEROS_REG_FILE),         //Register file
//	 &maui2API, &sombrero_beanieAPI, &veniceArtAniAPI,	            //APIs
//	 2, boss_0012_mode, sizeof(boss_0012_mode)/sizeof(MODE_INFO),   //Mode file
//	 CFG_VERSION_STRING_0012 },                                     //configuration string

#ifndef CUSTOMER_REL
//	{0xf013, NULL, 0, NULL, 0, 0xf013,								//Identifiers
//	 venice_emul,  sizeof(venice_emul)/sizeof(ATHEROS_REG_FILE),    //Register file
//	 &veniceAPI, &sombrero_beanieAPI, &veniceArtAniAPI,	     		//APIs
//	 2, venice_emul_mode, sizeof(venice_emul_mode)/sizeof(MODE_INFO), //Mode file
//	 CFG_VERSION_STRING_f013 },										//configuration string

//	{0xfb13, NULL, 0, NULL, 0, 0xfb13,									//Identifiers							    	
//	 venice_emul,  sizeof(venice_emul)/sizeof(ATHEROS_REG_FILE),		//Register file
//	 &veniceAPI, &sombrero_beanieAPI, &veniceArtAniAPI,					//APIs
//	 2, venice_emul_mode, sizeof(venice_emul_mode)/sizeof(MODE_INFO),	//Mode file
//	 CFG_VERSION_STRING_f013 },											//configuration string
#endif //CUSTOMER_REL

//	{DONT_MATCH, derby1_2Revs, numderby1_2Revs, veniceRevs, numVeniceRevs, SW_DEVICE_ID_VENICE_DERBY1, //Identifiers
//	 venice_derby,  sizeof(venice_derby)/sizeof(ATHEROS_REG_FILE),		//Register file
//	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,							//APIs
//	 3, venice_derby_mode, sizeof(venice_derby_mode)/sizeof(MODE_INFO), //Mode file
//	 CFG_VERSION_STRING_0014 },											//configuration string


	{DONT_MATCH, derby2_1Revs, numderby2_1Revs, veniceRevs, numVeniceRevs, SW_DEVICE_ID_VENICE_DERBY2, //Identifiers
	 venice_derby2_1,  sizeof(venice_derby2_1)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,							//APIs
	 3, venice_derby2_1_mode, sizeof(venice_derby2_1_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d016 },											//configuration string

	//will only point to this structure if want to load EAR from EEPROM
//	{DONT_MATCH, derby2Revs, numderby2Revs, veniceRevs, numVeniceRevs, SW_DEVICE_ID_VENICE_DERBY2, //Identifiers
//	 venice_derby2_1_ear,  sizeof(venice_derby2_1_ear)/sizeof(ATHEROS_REG_FILE),		//Register file
//	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,							//APIs
//	 3, venice_derby2_1_mode_ear, sizeof(venice_derby2_1_mode_ear)/sizeof(MODE_INFO), //Mode file
//	 CFG_VERSION_STRING_d016_EAR },											//configuration string

//	{DONT_MATCH, derby1_2Revs, numderby1_2Revs, veniceRevs, numVeniceRevs, SW_DEVICE_ID_VENICE_DERBY1, //Identifiers
//	 venice_derby,  sizeof(venice_derby)/sizeof(ATHEROS_REG_FILE),		//Register file
//	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,							//APIs
//	 3, venice_derby_mode, sizeof(venice_derby_mode)/sizeof(MODE_INFO), //Mode file
//	 CFG_VERSION_STRING_0014 },											//configuration string

//	{DONT_MATCH, derby1Revs, numDerby1Revs, veniceRevs, numVeniceRevs, SW_DEVICE_ID_VENICE_DERBY1, //Identifiers
//	 venice_derby_old,  sizeof(venice_derby_old)/sizeof(ATHEROS_REG_FILE),	//Register file
//	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,								//APIs
//	 2, venice_derby_old_mode, sizeof(venice_derby_old_mode)/sizeof(MODE_INFO), //Mode file
//	 CFG_VERSION_STRING_e014 },												//configuration string

//	{DONT_MATCH, derby2Revs, numderby2Revs, hainanRevs, numHainanRevs, SW_DEVICE_ID_HAINAN_DERBY, //Identifiers
//	 hainan_derby2_0,  sizeof(hainan_derby2_0)/sizeof(ATHEROS_REG_FILE),	//Register file
//	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,								//APIs
//	 3, hainan_derby2_0_mode, sizeof(hainan_derby2_0_mode)/sizeof(MODE_INFO), //Mode file
//	 CFG_VERSION_STRING_0017 },												//configuration string


	{DONT_MATCH, derby2_1Revs, numderby2_1Revs, hainanRevs, numHainanRevs, SW_DEVICE_ID_HAINAN_DERBY, //Identifiers
	 hainan_derby2_1,  sizeof(hainan_derby2_1)/sizeof(ATHEROS_REG_FILE),	//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,								//APIs
	 3, hainan_derby2_1_mode, sizeof(hainan_derby2_1_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d017 },												//configuration string

	//will only point to this structure if want to load EAR from EEPROM
	{DONT_MATCH, derby2_1Revs, numderby2_1Revs, hainanRevs, numHainanRevs, SW_DEVICE_ID_HAINAN_DERBY, //Identifiers
	 hainan_derby2_1_ear,  sizeof(hainan_derby2_1_ear)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,							//APIs
	 3, hainan_derby2_1_mode_ear, sizeof(hainan_derby2_1_mode_ear)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d017_EAR },											//configuration string

//	{DONT_MATCH, griffinAnalogRevs, numGriffinAnalogRevs, griffinRevs, numGriffinRevs, SW_DEVICE_ID_GRIFFIN, //Identifiers
//	 griffin,  sizeof(griffin)/sizeof(ATHEROS_REG_FILE),	//Register file
//	 &veniceAPI, &griffinRfAPI,	&veniceArtAniAPI,								//APIs
//	 3, griffin_mode, sizeof(griffin_mode)/sizeof(MODE_INFO), //Mode file
//	 CFG_VERSION_STRING_0018 },												//configuration string

/*
	{DONT_MATCH, griffin1_1_AnalogRevs, numGriffin1_1_AnalogRevs, griffinRevs, numGriffinRevs, SW_DEVICE_ID_GRIFFIN, //Identifiers
	 griffin,  sizeof(griffin)/sizeof(ATHEROS_REG_FILE),	//Register file
	 &veniceAPI, &griffinRfAPI,	&veniceArtAniAPI,								//APIs
	 3, griffin_mode, sizeof(griffin_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_0018 },												//configuration string

*/
//	{DONT_MATCH, griffin1_1_AnalogRevs, numGriffin1_1_AnalogRevs, griffinRevs, numGriffinRevs, SW_DEVICE_ID_GRIFFIN, //Identifiers
//	 griffin1_1,  sizeof(griffin1_1)/sizeof(ATHEROS_REG_FILE),	//Register file
//	 &veniceAPI, &griffinRfAPI,	&veniceArtAniAPI,								//APIs
//	 3, griffin1_1_mode, sizeof(griffin1_1_mode)/sizeof(MODE_INFO), //Mode file
//	 CFG_VERSION_STRING_d018 },												//configuration string


	//TEMP FOR CONDOR
//	{0xff19, NULL, 0, NULL, 0, SW_DEVICE_ID_CONDOR, //Identifiers
//	 griffin2,  sizeof(griffin2)/sizeof(ATHEROS_REG_FILE),	//Register file
//	 &veniceAPI, &griffinRfAPI,	&veniceArtAniAPI,								//APIs
//	 3, griffin2_mode, sizeof(griffin2_mode)/sizeof(MODE_INFO), //Mode file
//	 CFG_VERSION_STRING_c018 },												//configuration string

    {DONT_MATCH, fowlRevs, numFowlRevs, owl2Revs, numOwl2Revs, SW_DEVICE_ID_OWL, //Identifiers							    	
        owl2_fowl,  sizeof(owl2_fowl)/sizeof(ATHEROS_REG_FILE),            //Register file
        &owl_fowlAPI, &owl_fowlRfAPI,  &veniceArtAniAPI,                   //APIs
        3, owl2_fowl_mode, sizeof(owl2_fowl_mode)/sizeof(MODE_INFO),       //Mode file
        CFG_VERSION_STRING_0023},                                         //configuration string

	{DONT_MATCH, fowlRevs, numFowlRevs, owlRevs, numOwlRevs, SW_DEVICE_ID_OWL, //Identifiers							    	
	 owl_fowl,  sizeof(owl_fowl)/sizeof(ATHEROS_REG_FILE),		        //Register file
	 &owl_fowlAPI, &owl_fowlRfAPI,	&veniceArtAniAPI,					//APIs
	 3, owl_fowl_mode, sizeof(owl_fowl_mode)/sizeof(MODE_INFO),	        //Mode file
	 CFG_VERSION_STRING_ff1d },											//configuration string

	//SOWL Emulation
	{DONT_MATCH, fowlRevs, numFowlRevs, sowlEmulationRevs, numSowlEmulationRevs, SW_DEVICE_ID_SOWL, //Identifiers
	 sowlEmulation_fowl,  sizeof(sowlEmulation_fowl)/sizeof(ATHEROS_REG_FILE),		        //Register file
	 &owl_fowlAPI, &owl_fowlRfAPI,	&veniceArtAniAPI,					//APIs
	 3, sowlEmulation_fowl_mode, sizeof(sowlEmulation_fowl_mode)/sizeof(MODE_INFO),	        //Mode file
	 CFG_VERSION_STRING_ffff },											//configuration string

	 // SOWL
	{DONT_MATCH, fowlRevs, numFowlRevs, sowlRevs, numSowlRevs, SW_DEVICE_ID_SOWL, //Identifiers							    	
	 sowl_fowl,  sizeof(sowl_fowl)/sizeof(ATHEROS_REG_FILE),		        //Register file
	 &owl_fowlAPI, &owl_fowlRfAPI,	&veniceArtAniAPI,					//APIs
	 3, sowl_fowl_mode, sizeof(sowl_fowl_mode)/sizeof(MODE_INFO),	        //Mode file
	 CFG_VERSION_STRING_0027 },											//configuration string


	{DONT_MATCH, NULL, 0, owlRevs, numOwlRevs, 0xff1c, //Identifiers							    	
	 owl_fowl,  sizeof(owl_fowl)/sizeof(ATHEROS_REG_FILE),		        //Register file
	 &owl_fowlAPI, &owl_fowlRfAPI,	&veniceArtAniAPI,					//APIs
	 3, owl_fowl_mode, sizeof(owl_fowl_mode)/sizeof(MODE_INFO),	        //Mode file
	 CFG_VERSION_STRING_ff1d },											//configuration string

	 // MERLIN
	{
        DONT_MATCH,
        merlinAnalogRevs,    /* analog rev info */
        numMerlinAnalogRevs, /* num analog revs */
        merlinRevs,          /* mac rev info */
        numMerlinRevs,       /* num mac revs */
        SW_DEVICE_ID_MERLIN, /* Identifier */
        merlin,              /* ptr to ini reg file */
        sizeof(merlin)/sizeof(ATHEROS_REG_FILE), /* size of ini reg file */
        &owl_fowlAPI,        /* mac api */
        &merlinRfAPI,        /* rf api  */
        &veniceArtAniAPI,    /* ani api */
        3,                   /* cfg file version */
        merlin_mode,         /* ptr to mod file */
        sizeof(merlin_mode)/sizeof(MODE_INFO),	/* size of mod file */
        CFG_VERSION_STRING_0029                 /* configuration string */
     },				

	 // KITE1.1
        {
                        DONT_MATCH,
                                KiteAnalogRevs,    /* analog rev info */
                                numKiteAnalogRevs, /* num analog revs */
					Kite1_1MacRevs,    /* mac rev info */
					numKite1_1MacRevs, /* num mac revs */
                                SW_DEVICE_ID_KITE_PCIE, /* Identifier */
					kite1_1,           /* ptr to ini reg file */
					sizeof(kite1_1)/sizeof(ATHEROS_REG_FILE), /* size of ini reg file */
                                &owl_fowlAPI,        /* mac api */
                                &merlinRfAPI,        /* rf api  */
                                &veniceArtAniAPI,    /* ani api */
                                3,                   /* cfg file version */
					kite_mode1_1,      /* ptr to mod file */
					sizeof(kite_mode1_1)/sizeof(MODE_INFO), /* size of mod file */
				CFG_VERSION_STRING_002b                   /* configuration string */
 },
		// KITE1.2
		{
				DONT_MATCH,
					KiteAnalogRevs,    /* analog rev info */
					numKiteAnalogRevs, /* num analog revs */
						Kite1_2MacRevs,    /* mac rev info */
						numKite1_2MacRevs, /* num mac revs */
					SW_DEVICE_ID_KITE_PCIE, /* Identifier */
						kite1_2,           /* ptr to ini reg file */
						sizeof(kite1_2)/sizeof(ATHEROS_REG_FILE), /* size of ini reg file */
					&owl_fowlAPI,        /* mac api */
					&merlinRfAPI,        /* rf api  */
					&veniceArtAniAPI,    /* ani api */
					3,                   /* cfg file version */
						kite_mode1_2,      /* ptr to mod file */
						sizeof(kite_mode1_2)/sizeof(MODE_INFO), /* size of mod file */
						CFG_VERSION_STRING_022b                  /* configuration string */
			},

			// KITE 11g
			{
					DONT_MATCH,
					KiteAnalogRevs,    /* analog rev info */
					numKiteAnalogRevs, /* num analog revs */
					Kite11g,    /* mac rev info */
					numKite11g, /* num mac revs */
					SW_DEVICE_ID_KITE_11g, /* Identifier */
					kite1_2,           /* ptr to ini reg file */
					sizeof(kite1_2)/sizeof(ATHEROS_REG_FILE), /* size of ini reg file */
					&owl_fowlAPI,        /* mac api */
					&merlinRfAPI,        /* rf api  */
					&veniceArtAniAPI,    /* ani api */
					3,                   /* cfg file version */
					kite_mode1_2,      /* ptr to mod file */
					sizeof(kite_mode1_2)/sizeof(MODE_INFO), /* size of mod file */
					CFG_VERSION_STRING_022b                   /* configuration string */
			},

	 // MERLIN 2.0
	{
        DONT_MATCH,
        merlinAnalogRevs,    /* analog rev info */
        numMerlinAnalogRevs, /* num analog revs */
        merlin2_0MacRevs,    /* mac rev info */
        numMerlin2_0MacRevs, /* num mac revs */
        SW_DEVICE_ID_MERLIN, /* Identifier */
        merlin2_0,           /* ptr to ini reg file */
        sizeof(merlin2_0)/sizeof(ATHEROS_REG_FILE), /* size of ini reg file */
        &owl_fowlAPI,        /* mac api */
        &merlinRfAPI,        /* rf api  */
        &veniceArtAniAPI,    /* ani api */
        3,                   /* cfg file version */
        merlin2_0_mode,      /* ptr to mod file */
        sizeof(merlin2_0_mode)/sizeof(MODE_INFO), /* size of mod file */
        CFG_VERSION_STRING_002a                   /* configuration string */
     },				

    // KIWI 1.0
    {
        DONT_MATCH,
        kiwiAnalogRevs,      /* analog rev info */
        numKiwiAnalogRevs,   /* num analog revs */
        kiwi1_0MacRevs,      /* mac rev info */
        numKiwi1_0MacRevs,   /* num mac revs */
        SW_DEVICE_ID_KIWI, /* Identifier */
        kiwi1_0,           /* ptr to ini reg file */
        sizeof(kiwi1_0)/sizeof(ATHEROS_REG_FILE), /* size of ini reg file */
        &owl_fowlAPI,        /* mac api */
        &kiwiRfAPI,        /* rf api  */
        &veniceArtAniAPI,    /* ani api */
        3,                   /* cfg file version */
        kiwi1_0_mode,      /* ptr to mod file */
        sizeof(kiwi1_0_mode)/sizeof(MODE_INFO), /* size of mod file */
        CFG_VERSION_STRING_002d                   /* configuration string */
     },

    // KIWI 1.1
    {
        DONT_MATCH,
        kiwiAnalogRevs,      /* analog rev info */
        numKiwiAnalogRevs,   /* num analog revs */
        kiwi1_1MacRevs,      /* mac rev info */
        numKiwi1_1MacRevs,   /* num mac revs */
        SW_DEVICE_ID_KIWI,   /* Identifier */
        kiwi1_1,             /* ptr to ini reg file */
        sizeof(kiwi1_1)/sizeof(ATHEROS_REG_FILE), /* size of ini reg file */
        &owl_fowlAPI,        /* mac api */
        &kiwiRfAPI,        /* rf api  */
        &veniceArtAniAPI,    /* ani api */
        3,                   /* cfg file version */
        kiwi1_1_mode,       /* ptr to mod file */
        sizeof(kiwi1_1_mode)/sizeof(MODE_INFO), /* size of mod file */
        CFG_VERSION_STRING_002e  /* configuration string */
     },

    // KIWI 1.2
    {
        DONT_MATCH,
        kiwiAnalogRevs,      /* analog rev info */
        numKiwiAnalogRevs,   /* num analog revs */
        kiwi1_2MacRevs,      /* mac rev info */
        numKiwi1_2MacRevs,   /* num mac revs */
        SW_DEVICE_ID_KIWI,   /* Identifier */
        kiwi1_2,             /* ptr to ini reg file */
        sizeof(kiwi1_2)/sizeof(ATHEROS_REG_FILE), /* size of ini reg file */
        &owl_fowlAPI,        /* mac api */
        &kiwiRfAPI,        /* rf api  */
        &veniceArtAniAPI,    /* ani api */
        3,                   /* cfg file version */
        kiwi1_2_mode,       /* ptr to mod file */
        sizeof(kiwi1_2_mode)/sizeof(MODE_INFO), /* size of mod file */
        CFG_VERSION_STRING_022e  /* configuration string */
     },

#ifndef CUSTOMER_REL
    // KIWI_Merlin Emulation 
    {
        DONT_MATCH,
        kiwiAnalogRevs,      /* analog rev info */
        numKiwiAnalogRevs,   /* num analog revs */
        kiwi_Merlin_EmulationMacRevs,      /* mac rev info */
        numKiwi_Merlin_EmulationMacRevs,   /* num mac revs */
        SW_DEVICE_ID_KIWI, /* Identifier */
        kiwi_Merlin_Emulation,           /* ptr to ini reg file */
        sizeof(kiwi_Merlin_Emulation)/sizeof(ATHEROS_REG_FILE), /* size of ini reg file */
        &owl_fowlAPI,        /* mac api */
        &kiwiRfAPI,        /* rf api  */
        &veniceArtAniAPI,    /* ani api */
        3,                   /* cfg file version */
        kiwi_Merlin_Emulation_mode,      /* ptr to mod file */
        sizeof(kiwi_Merlin_Emulation_mode)/sizeof(MODE_INFO), /* size of mod file */
        CFG_VERSION_STRING_dfff                   /* configuration string */
     },
#endif //customer_rel

#ifndef CUSTOMER_REL
	//Merlin Fowl Emulation
	{
		DONT_MATCH, 
		fowlRevs, 
		numFowlRevs, 
		merlinFowlEmulationRevs, 
		numMerlinFowlEmulationRevs, 
		SW_DEVICE_ID_SOWL, //Identifiers
		merlinEmulation_fowl,
		sizeof(merlinEmulation_fowl)/sizeof(ATHEROS_REG_FILE),	//Register file
		&owl_fowlAPI, 
		&owl_fowlRfAPI,	
		&veniceArtAniAPI,	//APIs
		3, 
		merlinEmulation_fowl_mode,
		sizeof(merlinEmulation_fowl_mode)/sizeof(MODE_INFO),	//Mode file
		CFG_VERSION_STRING_efff //configuration string
	},	
#endif //customer_rel

	{DONT_MATCH, condorAnalogRevs, numCondorAnalogRevs, condorRevs, numCondorRevs, SW_DEVICE_ID_CONDOR, //Identifiers
	 condor,  sizeof(condor)/sizeof(ATHEROS_REG_FILE),	//Register file
	 &eagleAPI, &griffinRfAPI,	&veniceArtAniAPI,								//APIs
	 3, condor_mode, sizeof(condor_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_0020 },												//configuration string

	 // For Nala
	{
		DONT_MATCH,
		swanAnalogRevs,
		numSwanAnalogRevs,
		nalaRevs,
		numNalaRevs,
		SW_DEVICE_ID_NALA,
		nala,
		sizeof(nala) / sizeof(ATHEROS_REG_FILE),
		&eagleAPI,
		&griffinRfAPI,
		&veniceArtAniAPI,
		3,
		nala_mode,
		sizeof(nala_mode) / sizeof(MODE_INFO),
		CFG_VERSION_STRING_0026
	},


//	{0xff19, NULL, 0, NULL, 0, SW_DEVICE_ID_CONDOR, //Identifiers
//	 condor,  sizeof(condor)/sizeof(ATHEROS_REG_FILE),	//Register file
//	 &eagleAPI, &griffinRfAPI,	&veniceArtAniAPI,								//APIs
//	 3, condor_mode, sizeof(condor_mode)/sizeof(MODE_INFO), //Mode file
//	 CFG_VERSION_STRING_0020 },												//configuration string

//	{0x001c, NULL, 0, NULL, 0, SW_DEVICE_ID_CONDOR, //Identifiers
//	 condor,  sizeof(condor)/sizeof(ATHEROS_REG_FILE),	//Register file
//	 &eagleAPI, &griffinRfAPI,	&veniceArtAniAPI,								//APIs
//	 3, condor_mode, sizeof(condor_mode)/sizeof(MODE_INFO), //Mode file
//	 CFG_VERSION_STRING_0020 },												//configuration string

	{DONT_MATCH, griffin2_AnalogRevs, numGriffin2_AnalogRevs, griffinRevs, numGriffinRevs, SW_DEVICE_ID_GRIFFIN, //Identifiers
	 griffin2,  sizeof(griffin2)/sizeof(ATHEROS_REG_FILE),	//Register file
	 &veniceAPI, &griffinRfAPI,	&veniceArtAniAPI,								//APIs
	 3, griffin2_mode, sizeof(griffin2_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_c018 },												//configuration string

	//will only point to this structure if want to load EAR from EEPROM
	//This is to support hainan ear, which must use same frozen venice_derby config file
	{DONT_MATCH, griffinAnalogRevs, numGriffinAnalogRevs, griffinRevs, numGriffinRevs, SW_DEVICE_ID_GRIFFIN, //Identifiers
	 hainan_derby2_1_ear,  sizeof(hainan_derby2_1_ear)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &veniceAPI, &griffinRfAPI,	&veniceArtAniAPI,							//APIs
	 3, hainan_derby2_1_mode_ear, sizeof(hainan_derby2_1_mode_ear)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d017_EAR },											//configuration string

	{DONT_MATCH, eagleAnalogRevs, numEagleAnalogRevs, eagleRevs, numEagleRevs, SW_DEVICE_ID_EAGLE, //Identifiers
	 eagle,  sizeof(eagle)/sizeof(ATHEROS_REG_FILE),	//Register file
	 &eagleAPI, &griffinRfAPI,	&veniceArtAniAPI,								//APIs
	 3, eagle_mode, sizeof(eagle_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_0019 },												//configuration string

	//will only point to this structure if want to load EAR from EEPROM
	//This is to support hainan ear, which must use same frozen venice_derby config file
	{DONT_MATCH, eagleAnalogRevs, numEagleAnalogRevs, eagleRevs, numEagleRevs, SW_DEVICE_ID_GRIFFIN, //Identifiers
	 hainan_derby2_1_ear,  sizeof(hainan_derby2_1_ear)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &eagleAPI, &griffinRfAPI,	&veniceArtAniAPI,							//APIs
	 3, hainan_derby2_1_mode_ear, sizeof(hainan_derby2_1_mode_ear)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d017_EAR },											//configuration string
#ifdef LINUX
	{DONT_MATCH, eagle2AnalogRevs, numEagle2AnalogRevs, eagle2Revs, numEagle2Revs, SW_DEVICE_ID_EAGLE, //Identifiers
	 eagle2,  sizeof(eagle2)/sizeof(ATHEROS_REG_FILE),	//Register file
	 &eagleAPI, &griffinRfAPI,	&veniceArtAniAPI,								//APIs
	 3, eagle2_mode, sizeof(eagle2_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d019 },												//configuration string
#endif
	//will only point to this structure if want to load EAR from EEPROM
	//This is to support hainan ear, which must use same frozen venice_derby config file
	{DONT_MATCH, eagle2AnalogRevs, numEagle2AnalogRevs, eagle2Revs, numEagle2Revs, SW_DEVICE_ID_GRIFFIN, //Identifiers
	 hainan_derby2_1_ear,  sizeof(hainan_derby2_1_ear)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &eagleAPI, &griffinRfAPI,	&veniceArtAniAPI,							//APIs
	 3, hainan_derby2_1_mode_ear, sizeof(hainan_derby2_1_mode_ear)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d017_EAR },											//configuration string

#ifdef LINUX
#ifdef LINUX
#ifndef SOC_LINUX
	 {DONT_MATCH, dragonAnalogRevs, numDragonAnalogRevs, dragonRevs, numDragonRevs, SW_DEVICE_ID_DRAGON, //Identifiers
	 dragon,  sizeof(dragon)/sizeof(ATHEROS_REG_FILE),	//Register file
	 &dragonAPI, &dragonRfAPI,	&veniceArtAniAPI,								//APIs
	 3, dragon_mode, sizeof(dragon_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d022 },
#endif
#endif
#endif
//	{DONT_MATCH, sombreroRevs, numSombreroRevs, veniceRevs, numVeniceRevs, SW_DEVICE_ID_VENICE_SOMBRERO,	//Identifiers
//	 venice,  sizeof(venice)/sizeof(ATHEROS_REG_FILE),						//Register file
//	 &veniceAPI, &sombrero_beanieAPI, &veniceArtAniAPI,						//APIs
//	 3, venice_mode, sizeof(venice_mode)/sizeof(MODE_INFO),					//Mode file
//	 CFG_VERSION_STRING_0013 },												//configuration string

//#ifdef PREDATOR_BUILD
// Predator emulation
//	{0xf0b0, derby2_1Revs, numderby2_1Revs, predatorRevs, numPredatorRevs, 0xf0b0, //Identifiers
//	 venice_derby2_1,  sizeof(venice_derby2_1)/sizeof(ATHEROS_REG_FILE),		//Register file
//	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,							//APIs
//	 3, venice_derby2_1_mode, sizeof(venice_derby2_1_mode)/sizeof(MODE_INFO), //Mode file
//	 NULL },											//configuration string

// Predator 
	{DONT_MATCH, derby2_1Revs, numderby2_1Revs, predatorRevs, numPredatorRevs, SW_DEVICE_ID_PREDATOR, //Identifiers
	 predator_derby2_1,  sizeof(predator_derby2_1)/sizeof(ATHEROS_REG_FILE),	//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,								//APIs
	 3, predator_derby2_1_mode, sizeof(predator_derby2_1_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_00b0 },												//configuration string
//#endif   // PREDATOR_BUILD



#ifndef CUSTOMER_REL
//	{0xf015, NULL, 0, NULL, 0, 0xf015,										//Identifiers
//	 hainan_sb_emul,  sizeof(hainan_sb_emul)/sizeof(ATHEROS_REG_FILE),		//Register file
//	 &veniceAPI, &sombrero_beanieAPI, &veniceArtAniAPI,						//APIs
//	 2, hainan_sb_emul_mode, sizeof(hainan_sb_emul_mode)/sizeof(MODE_INFO), //Mode file
//	 CFG_VERSION_STRING_f015 },										//configuration string
//
//	{0xf016, NULL, 0, NULL, 0, 0xf016,										//Identifiers
//	 hainan_derby_emul,  sizeof(hainan_derby_emul)/sizeof(ATHEROS_REG_FILE),//Register file 
//	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,								//APIs
//	 2, hainan_derby_emul_mode, sizeof(hainan_derby_emul_mode)/sizeof(MODE_INFO), //Mode file
//	 CFG_VERSION_STRING_f016 },												//configuration string

#endif //CUSTOMER_REL
#endif //MDK_AP


#ifdef SPIRIT_AP
   	{0x0011, NULL, 0, NULL, 0, 0x0011,
	 ar5k0011_spirit1_som2_e4,  sizeof(ar5k0011_spirit1_som2_e4)/sizeof(ATHEROS_REG_FILE), 
	 &maui2API, &sombreroAPI,&veniceArtAniAPI,0,NULL,0,NULL },
#endif //SPIRIT_AP

#ifdef AP22_AP
	{0x0012, NULL, 0, NULL, 0, SW_DEVICE_ID_BOSS_0012,									//Identifiers
	 boss_0012, sizeof(boss_0012)/sizeof(ATHEROS_REG_FILE),				//register file
	 &maui2API, &sombrero_beanieAPI, &veniceArtAniAPI,					//APIs
	 2, boss_0012_mode, sizeof(boss_0012_mode)/sizeof(MODE_INFO),		//Mode file
	 CFG_VERSION_STRING_0012 },											//configuraton string

	{0xff12, NULL, 0, NULL, 0,	SW_DEVICE_ID_BOSS_0012,						    		//Identifiers
	 boss_0012, sizeof(boss_0012)/sizeof(ATHEROS_REG_FILE),				//Register file
	 &maui2API, &sombrero_beanieAPI, &veniceArtAniAPI,					//APIs
	 2, boss_0012_mode, sizeof(boss_0012_mode)/sizeof(MODE_INFO),		//Mode file
	 CFG_VERSION_STRING_0012 },											//configuration string

	{DONT_MATCH, derby2_1Revs, sizeof(derby2_1Revs)/sizeof(ANALOG_REV), veniceRevs, sizeof(veniceRevs)/sizeof(A_UINT32), SW_DEVICE_ID_VENICE_DERBY2, //Identifiers
	 venice_derby2_1,  sizeof(venice_derby2_1)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,							//APIs
	 3, venice_derby2_1_mode, sizeof(venice_derby2_1_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d016 },											//configuration string

	//will only point to this structure if want to load EAR from EEPROM
	{DONT_MATCH, derby2Revs, sizeof(derby2Revs)/sizeof(ANALOG_REV), veniceRevs, sizeof(veniceRevs)/sizeof(A_UINT32), SW_DEVICE_ID_VENICE_DERBY2, //Identifiers
	 venice_derby2_1_ear,  sizeof(venice_derby2_1_ear)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,							//APIs
	 3, venice_derby2_1_mode_ear, sizeof(venice_derby2_1_mode_ear)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d016_EAR },											//configuration string

	{DONT_MATCH, derby2_1Revs, sizeof(derby2_1Revs)/sizeof(ANALOG_REV), hainanRevs, sizeof(hainanRevs)/sizeof(A_UINT32), SW_DEVICE_ID_HAINAN_DERBY, //Identifiers
	 hainan_derby2_1,  sizeof(hainan_derby2_1)/sizeof(ATHEROS_REG_FILE),	//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,								//APIs
	 3, hainan_derby2_1_mode, sizeof(hainan_derby2_1_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d017 },												//configuration string

	//will only point to this structure if want to load EAR from EEPROM
	//This is to support hainan ear, which must use same frozen venice_derby config file
	{DONT_MATCH, derby2Revs, sizeof(derby2Revs)/sizeof(ANALOG_REV), veniceRevs, sizeof(veniceRevs)/sizeof(A_UINT32), SW_DEVICE_ID_VENICE_DERBY2, //Identifiers
	 hainan_derby2_1_ear,  sizeof(hainan_derby2_1_ear)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,							//APIs
	 3, hainan_derby2_1_mode_ear, sizeof(hainan_derby2_1_mode_ear)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d017_EAR },											//configuration string

	{DONT_MATCH, sombreroRevs, sizeof(sombreroRevs)/sizeof(ANALOG_REV), veniceRevs, sizeof(veniceRevs)/sizeof(A_UINT32), SW_DEVICE_ID_VENICE_SOMBRERO,	//Identifiers
	 venice,  sizeof(venice)/sizeof(ATHEROS_REG_FILE),						//Register file
	 &veniceAPI, &sombrero_beanieAPI, &veniceArtAniAPI,						//APIs
	 3, venice_mode, sizeof(venice_mode)/sizeof(MODE_INFO),					//Mode file
	 CFG_VERSION_STRING_0013 },												//configuration string
#endif // AP22_AP

#if (defined(FREEDOM_AP)||defined(THIN_CLIENT_BUILD))&&!defined(COBRA_AP)
//#if !defined(COBRA_AP)
//	{0x0012,  NULL, 0, NULL, 0, SW_DEVICE_ID_BOSS_0012,										//Identifiers
//	 boss_0012,  sizeof(boss_0012)/sizeof(ATHEROS_REG_FILE),				//Register file
//	 &maui2API, &sombrero_beanieAPI,										//APIs
//	 2, boss_0012_mode, sizeof(boss_0012_mode)/sizeof(MODE_INFO),			//Mode file
//	 CFG_VERSION_STRING_0012 },												//configuration string

//	{0x0012,  NULL, 0, NULL, 0,	SW_DEVICE_ID_BOSS_0012,										//Identifiers
//	 freedom1_derby,  sizeof(freedom1_derby)/sizeof(ATHEROS_REG_FILE),		//Register file
//	 &veniceAPI, &derbyAPI,													//APIs
//	 2, freedom1_derby_mode, sizeof(freedom1_derby_mode)/sizeof(MODE_INFO), //Mode file
//	 "CFG_VERSION_STRING_0014" },																	//configuration string

//	{0xa014,  derby1_2Revs, sizeof(derby1_2Revs)/sizeof(ANALOG_REV), NULL, 0,	0xa014,										//Identifiers
//	 freedom2_derby,  sizeof(freedom2_derby)/sizeof(ATHEROS_REG_FILE),		//Register file
//	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,								//APIs
//	 3, freedom2_derby_mode, sizeof(freedom2_derby_mode)/sizeof(MODE_INFO), //Mode file 
//	 CFG_VERSION_STRING_a014 },																	//configuration string

#ifndef SOC_LINUX
	{0xa014,  derby2Revs, sizeof(derby2Revs)/sizeof(ANALOG_REV), NULL, 0,	0xa016,										//Identifiers
	 freedom2_derby2,  sizeof(freedom2_derby2)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,								//APIs
	 3, freedom2_derby2_mode, sizeof(freedom2_derby2_mode)/sizeof(MODE_INFO), //Mode file 
	 CFG_VERSION_STRING_a016 },																	//configuration string

#if 0
	//will only point to this structure if want to load EAR from EEPROM
	{0xa014,  derby2Revs, sizeof(derby2Revs)/sizeof(ANALOG_REV), NULL, 0,	0xa016,										//Identifiers
	 freedom2_derby2_ear,  sizeof(freedom2_derby2_ear)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,								//APIs
	 3, freedom2_derby2_mode_ear, sizeof(freedom2_derby2_mode_ear)/sizeof(MODE_INFO), //Mode file 
	 CFG_VERSION_STRING_a016_EAR },																	//configuration string
#endif
#endif

	{0xa014,  derby2_1Revs, sizeof(derby2_1Revs)/sizeof(ANALOG_REV), NULL, 0,	0xa016,										//Identifiers
	 freedom2_derby2_1,  sizeof(freedom2_derby2_1)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,								//APIs
	 3, freedom2_derby2_1_mode, sizeof(freedom2_derby2_1_mode)/sizeof(MODE_INFO), //Mode file 
	 CFG_VERSION_STRING_ad16 },																	//configuration string

#ifndef SOC_LINUX
#if 0
	//will only point to this structure if want to load EAR from EEPROM
	{0xa014,  derby2Revs, sizeof(derby2Revs)/sizeof(ANALOG_REV), NULL, 0,	0xa016,										//Identifiers
	 freedom2_derby2_ear,  sizeof(freedom2_derby2_ear)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,								//APIs
	 3, freedom2_derby2_mode_ear, sizeof(freedom2_derby2_mode_ear)/sizeof(MODE_INFO), //Mode file 
	 CFG_VERSION_STRING_a016_EAR },																	//configuration string

	{0xa017,  derby2_1Revs, sizeof(derby2_1Revs)/sizeof(ANALOG_REV), NULL, 0,	0xa017,										//Identifiers
	 viper_derby2_1,  sizeof(viper_derby2_1)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,								//APIs
	 3, viper_derby2_1_mode, sizeof(viper_derby2_1_mode)/sizeof(MODE_INFO), //Mode file 
	 CFG_VERSION_STRING_a017 },																	//configuration string

	//will only point to this structure if want to load EAR from EEPROM
	{0xa017,  derby2Revs, sizeof(derby2Revs)/sizeof(ANALOG_REV), NULL, 0,	0xa017,										//Identifiers
	 freedom2_derby2_ear,  sizeof(freedom2_derby2_ear)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,								//APIs
	 3, freedom2_derby2_mode_ear, sizeof(freedom2_derby2_mode_ear)/sizeof(MODE_INFO), //Mode file 
	 CFG_VERSION_STRING_a016_EAR },																	//configuration string
#endif
//	{0xa013,  NULL, 0, NULL, 0,	0xa013,										//Identifiers 
//	 freedom2_sombrero,  sizeof(freedom2_sombrero)/sizeof(ATHEROS_REG_FILE),//Register file 
//	 &veniceAPI, &sombrero_beanieAPI, &veniceArtAniAPI,						//APIs
//	 2, freedom2_sombrero_mode, sizeof(freedom2_sombrero_mode)/sizeof(MODE_INFO), //Mode file
//	 CFG_VERSION_STRING_a013 },																	//configuration string
#endif
#endif

#ifdef SENAO_AP
	{DONT_MATCH, derby2_1Revs, sizeof(derby2_1Revs)/sizeof(ANALOG_REV), veniceRevs, sizeof(veniceRevs)/sizeof(A_UINT32), SW_DEVICE_ID_VENICE_DERBY2, //Identifiers
	 venice_derby2_1,  sizeof(venice_derby2_1)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,							//APIs
	 3, venice_derby2_1_mode, sizeof(venice_derby2_1_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d016 },											//configuration string

	//will only point to this structure if want to load EAR from EEPROM
	{DONT_MATCH, derby2Revs, sizeof(derby2Revs)/sizeof(ANALOG_REV), veniceRevs, sizeof(veniceRevs)/sizeof(A_UINT32), SW_DEVICE_ID_VENICE_DERBY2, //Identifiers
	 venice_derby2_1_ear,  sizeof(venice_derby2_1_ear)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,							//APIs
	 3, venice_derby2_1_mode_ear, sizeof(venice_derby2_1_mode_ear)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d016_EAR },											//configuration string

	{DONT_MATCH, derby2_1Revs, sizeof(derby2Revs)/sizeof(ANALOG_REV), hainanRevs, sizeof(hainanRevs)/sizeof(A_UINT32), SW_DEVICE_ID_HAINAN_DERBY, //Identifiers
	 hainan_derby2_1,  sizeof(hainan_derby2_1)/sizeof(ATHEROS_REG_FILE),	//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,								//APIs
	 3, hainan_derby2_1_mode, sizeof(hainan_derby2_1_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d017 },												//configuration string

	//will only point to this structure if want to load EAR from EEPROM
	//This is to support hainan ear, which must use same frozen venice_derby config file
	{DONT_MATCH, derby2Revs, sizeof(derby2Revs)/sizeof(ANALOG_REV), veniceRevs, sizeof(veniceRevs)/sizeof(A_UINT32), SW_DEVICE_ID_VENICE_DERBY2, //Identifiers
	 hainan_derby2_1_ear,  sizeof(hainan_derby2_1_ear)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &veniceAPI, &derbyAPI,	&veniceArtAniAPI,							//APIs
	 3, hainan_derby2_1_mode_ear, sizeof(hainan_derby2_1_mode_ear)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d017_EAR },											//configuration string

	{DONT_MATCH, griffin2_AnalogRevs,  sizeof(griffin2_AnalogRevs)/sizeof(ANALOG_REV), griffinRevs,sizeof(griffinRevs)/sizeof(A_UINT32), SW_DEVICE_ID_GRIFFIN, //Identifiers
	 griffin2,  sizeof(griffin2)/sizeof(ATHEROS_REG_FILE),	//Register file
	 &veniceAPI, &griffinRfAPI,	&veniceArtAniAPI,								//APIs
	 3, griffin2_mode, sizeof(griffin2_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_c018 },												//configuration string 

//will only point to this structure if want to load EAR from EEPROM
	//This is to support hainan ear, which must use same frozen venice_derby config file
	{DONT_MATCH, griffinAnalogRevs,sizeof(griffinAnalogRevs)/sizeof(ANALOG_REV),griffinRevs,sizeof(griffinRevs)/sizeof(A_UINT32),SW_DEVICE_ID_GRIFFIN, //Identifiers
	 hainan_derby2_1_ear,  sizeof(hainan_derby2_1_ear)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &veniceAPI, &griffinRfAPI,	&veniceArtAniAPI,							//APIs
	 3, hainan_derby2_1_mode_ear, sizeof(hainan_derby2_1_mode_ear)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d017_EAR },	


	{DONT_MATCH, eagle2AnalogRevs, sizeof(eagle2AnalogRevs)/sizeof(ANALOG_REV), eagle2Revs, sizeof(eagle2Revs)/sizeof(A_UINT32), SW_DEVICE_ID_EAGLE, //Identifiers
	 eagle2,  sizeof(eagle2)/sizeof(ATHEROS_REG_FILE),	//Register file
	 &eagleAPI, &griffinRfAPI,	&veniceArtAniAPI,								//APIs
	 3, eagle2_mode, sizeof(eagle2_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d019 },												//configuration string

	//will only point to this structure if want to load EAR from EEPROM
	//This is to support hainan ear, which must use same frozen venice_derby config file
	{DONT_MATCH, eagle2AnalogRevs, sizeof(eagle2AnalogRevs)/sizeof(ANALOG_REV), eagle2Revs, sizeof(eagle2Revs)/sizeof(A_UINT32), SW_DEVICE_ID_GRIFFIN, //Identifiers
	 hainan_derby2_1_ear,  sizeof(hainan_derby2_1_ear)/sizeof(ATHEROS_REG_FILE),		//Register file
	 &eagleAPI, &griffinRfAPI,	&veniceArtAniAPI,							//APIs
	 3, hainan_derby2_1_mode_ear, sizeof(hainan_derby2_1_mode_ear)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d017_EAR },											//configuration string
#endif // SENAO_AP

#ifdef COBRA_AP
#ifdef SPIDER_AP

#ifdef VER2
	{DEVICE_ID_SPIDER2_0, NULL,  0, NULL, 0, DEVICE_ID_SPIDER2_0, //Identifiers
	 spider2_0,  sizeof(spider2_0)/sizeof(ATHEROS_REG_FILE),	//Register file
	 &veniceAPI, &griffinRfAPI,	&veniceArtAniAPI,								//APIs
	 3, spider2_0_mode, sizeof(spider2_0_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_a056 },												//configuration string
#else
	{DEVICE_ID_COBRA, spiderAnalogRevs,  sizeof(spiderAnalogRevs)/sizeof(ANALOG_REV), cobraRevs, sizeof(cobraRevs)/sizeof(A_UINT32), DEVICE_ID_SPIDER1_0, //Identifiers
	 spider,  sizeof(spider)/sizeof(ATHEROS_REG_FILE),	//Register file
	 &veniceAPI, &griffinRfAPI,	&veniceArtAniAPI,								//APIs
	 3, spider_mode, sizeof(spider_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_a055 },												//configuration string
#endif
#else
	{DEVICE_ID_COBRA, cobraAnalogRevs,  sizeof(cobraAnalogRevs)/sizeof(ANALOG_REV), cobraRevs, sizeof(cobraRevs)/sizeof(A_UINT32), SW_DEVICE_ID_AP51, //Identifiers
	 cobra,  sizeof(cobra)/sizeof(ATHEROS_REG_FILE),	//Register file
	 &veniceAPI, &griffinRfAPI,	&veniceArtAniAPI,								//APIs
	 3, cobra_mode, sizeof(cobra_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_0020 },												//configuration string

#endif

#ifdef PCI_INTERFACE
	{DONT_MATCH, eagle2AnalogRevs, sizeof(eagle2AnalogRevs)/sizeof(ANALOG_REV), eagle2Revs, sizeof(eagle2Revs)/sizeof(A_UINT32), SW_DEVICE_ID_EAGLE, //Identifiers
	 eagle2,  sizeof(eagle2)/sizeof(ATHEROS_REG_FILE),	//Register file
	 &eagleAPI, &griffinRfAPI,	&veniceArtAniAPI,								//APIs
	 3, eagle2_mode, sizeof(eagle2_mode)/sizeof(MODE_INFO), //Mode file
	 CFG_VERSION_STRING_d019 },												//configuration string
#endif

#endif // Cobra AP

#ifdef OWL_AP
#ifdef VER1
    {DONT_MATCH, fowlRevs, sizeof(fowlRevs)/sizeof(ANALOG_REV), owlRevs, sizeof(owlRevs)/sizeof(A_UINT32), SW_DEVICE_ID_OWL, //Identifiers
     owl_fowl,  sizeof(owl_fowl)/sizeof(ATHEROS_REG_FILE), //Register file
     &owl_fowlAPI, &owl_fowlRfAPI,  &veniceArtAniAPI,                 //APIs
     3, owl_fowl_mode, sizeof(owl_fowl_mode)/sizeof(MODE_INFO), //Mode file
     CFG_VERSION_STRING_ff1d },  //configuration string

#else //VER1
	{DONT_MATCH, fowlRevs, sizeof(fowlRevs)/sizeof(ANALOG_REV), owl2Revs, sizeof(owl2Revs)/sizeof(A_UINT32), SW_DEVICE_ID_OWL, //Identifiers							    	
	 owl2_fowl,  sizeof(owl2_fowl)/sizeof(ATHEROS_REG_FILE),		        //Register file
	 &owl_fowlAPI, &owl_fowlRfAPI,	&veniceArtAniAPI,					//APIs
	 3, owl2_fowl_mode, sizeof(owl2_fowl_mode)/sizeof(MODE_INFO),	        //Mode file
	 CFG_VERSION_STRING_0023 },

	 // SOWL
	{DONT_MATCH, fowlRevs, sizeof(fowlRevs)/sizeof(ANALOG_REV), sowlRevs, sizeof(sowlRevs)/sizeof(A_UINT32), SW_DEVICE_ID_SOWL, //Identifiers							    	
	 sowl_fowl,  sizeof(sowl_fowl)/sizeof(ATHEROS_REG_FILE),		        //Register file
	 &owl_fowlAPI, &owl_fowlRfAPI,	&veniceArtAniAPI,					//APIs
	 3, sowl_fowl_mode, sizeof(sowl_fowl_mode)/sizeof(MODE_INFO),	        //Mode file
	 CFG_VERSION_STRING_0027 },
	 
/*	 // For Nala
	{
		DONT_MATCH,
		swanAnalogRevs,
		sizeof(swanAnalogRevs)/sizeof(ANALOG_REV),
		nalaRevs,
		sizeof(nalaRevs) / sizeof(A_UINT32),
		SW_DEVICE_ID_NALA,
		nala,
		sizeof(nala) / sizeof(ATHEROS_REG_FILE),
		&eagleAPI,
		&griffinRfAPI,
		&veniceArtAniAPI,
		3,
		nala_mode,
		sizeof(nala_mode) / sizeof(MODE_INFO),
		CFG_VERSION_STRING_0026
	},											//configuration string
*/	 

#if 0 
         // KITE 1.1
        {
                        DONT_MATCH,
                                KiteAnalogRevs,    /* analog rev info */
                                //numKiteAnalogRevs, /* num analog revs */
sizeof(KiteAnalogRevs)/sizeof(ANALOG_REV),
                                Kite1_1MacRevs,    /* mac rev info */
                                //numKiteMacRevs, /* num mac revs */
sizeof(Kite1_1MacRevs)/sizeof(A_UINT32),
                                SW_DEVICE_ID_KITE_PCIE, /* Identifier */
                                kite1_1,           /* ptr to ini reg file */
                                sizeof(kite1_1)/sizeof(ATHEROS_REG_FILE), /* size of ini reg file */
                                &owl_fowlAPI,        /* mac api */
                                &merlinRfAPI,        /* rf api  */
                                &veniceArtAniAPI,    /* ani api */
                                3,                   /* cfg file version */
                                kite_mode1_1,      /* ptr to mod file */
                                sizeof(kite_mode1_1)/sizeof(MODE_INFO), /* size of mod file */
                                CFG_VERSION_STRING_002b                   /* configuration string */
 },
 #endif

         // KITE 1.2
		{
			DONT_MATCH,
			KiteAnalogRevs,    /* analog rev info */
			//numKiteAnalogRevs, /* num analog revs */
			sizeof(KiteAnalogRevs)/sizeof(ANALOG_REV),
			Kite1_2MacRevs,    /* mac rev info */
			//numKiteMacRevs, /* num mac revs */
			sizeof(Kite1_2MacRevs)/sizeof(A_UINT32),
			SW_DEVICE_ID_KITE_PCIE, /* Identifier */
			kite1_2,           /* ptr to ini reg file */
			sizeof(kite1_2)/sizeof(ATHEROS_REG_FILE), /* size of ini reg file */
			&owl_fowlAPI,        /* mac api */
			&merlinRfAPI,        /* rf api  */
			&veniceArtAniAPI,    /* ani api */
			3,                   /* cfg file version */
			kite_mode1_2,      /* ptr to mod file */
			sizeof(kite_mode1_2)/sizeof(MODE_INFO), /* size of mod file */
			CFG_VERSION_STRING_022b                   /* configuration string */
		},

		// KITE 11g
			{
				DONT_MATCH,
					KiteAnalogRevs,    /* analog rev info */
					// numKiteAnalogRevs, /* num analog revs */
					sizeof(KiteAnalogRevs)/sizeof(ANALOG_REV),
					Kite11g,    /* mac rev info */
					//numKite11g, /* num mac revs */
					sizeof(Kite11g)/sizeof(A_UINT32),
					SW_DEVICE_ID_KITE_11g, /* Identifier */
					kite1_2,           /* ptr to ini reg file */
					sizeof(kite1_2)/sizeof(ATHEROS_REG_FILE), /* size of ini reg file */
					&owl_fowlAPI,        /* mac api */
					&merlinRfAPI,        /* rf api  */
					&veniceArtAniAPI,    /* ani api */
					3,                   /* cfg file version */
					kite_mode1_2,      /* ptr to mod file */
					sizeof(kite_mode1_2)/sizeof(MODE_INFO), /* size of mod file */
					CFG_VERSION_STRING_022b                   /* configuration string */
			},

		 // MERLIN 2.0
	{
          DONT_MATCH,
          merlinAnalogRevs,    /* analog rev info */
          sizeof(merlinAnalogRevs)/sizeof(ANALOG_REV), /* num analog revs */
          merlin2_0MacRevs,    /* mac rev info */
          sizeof(merlin2_0MacRevs)/sizeof(A_UINT32), /* num mac revs */
          SW_DEVICE_ID_MERLIN, /* Identifier */
          merlin2_0,           /* ptr to ini reg file */
          sizeof(merlin2_0)/sizeof(ATHEROS_REG_FILE), /* size of ini reg file */
          &owl_fowlAPI,        /* mac api */
          &merlinRfAPI,        /* rf api  */
          &veniceArtAniAPI,    /* ani api */
          3,                   /* cfg file version */
          merlin2_0_mode,      /* ptr to mod file */
          sizeof(merlin2_0_mode)/sizeof(MODE_INFO), /* size of mod file */
          CFG_VERSION_STRING_002a                   /* configuration string */
        },				

    // KIWI 1.1
    {
        DONT_MATCH,
        kiwiAnalogRevs,      /* analog rev info */
        sizeof(kiwiAnalogRevs)/sizeof(ANALOG_REV),   /* num analog revs */
        kiwi1_1MacRevs,      /* mac rev info */
        sizeof(kiwi1_1MacRevs)/sizeof(A_UINT32),   /* num mac revs */
        SW_DEVICE_ID_KIWI,   /* Identifier */
        kiwi1_1,             /* ptr to ini reg file */
        sizeof(kiwi1_1)/sizeof(ATHEROS_REG_FILE), /* size of ini reg file */
        &owl_fowlAPI,        /* mac api */
        &kiwiRfAPI,        /* rf api  */
        &veniceArtAniAPI,    /* ani api */
        3,                   /* cfg file version */
        kiwi1_1_mode,       /* ptr to mod file */
        sizeof(kiwi1_1_mode)/sizeof(MODE_INFO), /* size of mod file */
        CFG_VERSION_STRING_002e  /* configuration string */
     },

    // KIWI 1.2
    {
        DONT_MATCH,
        kiwiAnalogRevs,      /* analog rev info */
        sizeof(kiwiAnalogRevs)/sizeof(ANALOG_REV),   /* num analog revs */
        kiwi1_2MacRevs,      /* mac rev info */
        sizeof(kiwi1_2MacRevs)/sizeof(A_UINT32),   /* num mac revs */
        SW_DEVICE_ID_KIWI,   /* Identifier */
        kiwi1_2,             /* ptr to ini reg file */
        sizeof(kiwi1_2)/sizeof(ATHEROS_REG_FILE), /* size of ini reg file */
        &owl_fowlAPI,        /* mac api */
        &kiwiRfAPI,        /* rf api  */
        &veniceArtAniAPI,    /* ani api */
        3,                   /* cfg file version */
        kiwi1_2_mode,       /* ptr to mod file */
        sizeof(kiwi1_2_mode)/sizeof(MODE_INFO), /* size of mod file */
        CFG_VERSION_STRING_022e  /* configuration string */
     },
	 
#endif //VER1
#endif

#ifdef OWL_PB42
        {DEVICE_ID_HOWL, fowlRevs, sizeof(fowlRevs)/sizeof(ANALOG_REV), howlRevs, sizeof(howlRevs)/sizeof(A_UINT32), SW_DEVICE_ID_HOWL, //Identifiers
         howl_fowl,  sizeof(howl_fowl)/sizeof(ATHEROS_REG_FILE),                        //Register file
         &owl_fowlAPI, &owl_fowlRfAPI,  &veniceArtAniAPI,                                       //APIs
         3, howl_fowl_mode, sizeof(howl_fowl_mode)/sizeof(MODE_INFO),           //Mode file
         CFG_VERSION_STRING_a027 },
#endif

};

A_UINT32 numDeviceIDs = (sizeof(ar5kInitData)/sizeof(DEVICE_INIT_DATA));




