#include "wlantype.h"
#include "dk_common.h"

#if defined(SPIRIT_AP) || defined(FREEDOM_AP)
#include "misclib.h"
#endif

A_INT32 m_loadAndRunCode
(
	A_UINT32 loadFlag,
	A_UINT32 pPhyAddr,
	A_UINT32 length,
 	A_UCHAR  *pBuffer,
	A_UINT32 devIndex
)
{
#if defined(SPIRIT_AP) || defined(FREEDOM_AP)
	return loadAndRunCode(loadFlag,pPhyAddr,length,pBuffer);
#else
#if defined(PREDATOR_BUILD)
	return loadAndRunCode(loadFlag,pPhyAddr,length,pBuffer, devIndex);
#else
	uiPrintf("Load and run code not implemented \n");
	return -1;
#endif
#endif
}

void m_writeNewProdData
(
	A_UINT32 devNum,
	A_INT32  argList[16],
	A_UINT32 numArgs
)
{
#ifdef FREEDOM_AP
	writeNewProdData(devNum,argList,numArgs);
#else
	uiPrintf("Write New Prod Data (Radio Mask, etc...) not implemented \n");
#endif
	return;
}

void m_writeProdData
(
	A_UINT32 devNum,
	A_UCHAR wlan0Mac[6],
	A_UCHAR wlan1Mac[6],
	A_UCHAR enet0Mac[6],
	A_UCHAR enet1Mac[6]
)
{
#ifdef VXWORKS
 	writeProdData(devNum,wlan0Mac,enet0Mac, enet1Mac);
#elif SPIRIT_AP
	writeProdData(devNum,wlan0Mac,enet0Mac, enet1Mac);
#elif FREEDOM_AP
	writeProdData(devNum,wlan0Mac, wlan1Mac, enet0Mac, enet1Mac);
#elif OWL_PB42
	writeProdDataMacAddr(devNum, enet0Mac, enet1Mac);
#else
	uiPrintf("Write Prod Data not implemented \n");
#endif
	return;
}

A_INT32 m_ftpDownloadFile
(
 A_CHAR *hostname,
 A_CHAR *user,
 A_CHAR *passwd,
 A_CHAR *remotefile,
 A_CHAR *localfile
)
{
#ifdef FREEDOM_AP
	return ftpDownloadFile(hostname, user, passwd, remotefile, localfile);
#else
	uiPrintf("ftp download file not implemented \n");
	return 0;
#endif
}


// Screenign test 
A_INT32 m_runScreeningTest
(	
	A_UINT32 testId
)
{
#ifdef INCLUDE_SCREENING_TESTS
	return runScreeningTest(testId);
#else
	uiPrintf("Screening test not implemented \n");
	return -1; // return 0 if screening test is not included
#endif // SCREENING_TEST
}


