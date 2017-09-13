/*
 *  Copyright © 2001 Atheros Communications, Inc.,  All Rights Reserved.
 */
/* manlibInst.h - Exported functions and defines for mInst.c */

#ifndef	__INCmanlibinsth
#define	__INCmanlibinsth
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ident  "ACI $Id: //depot/sw/src/dk/mdk/manlib/manlibInst.h#1 $, $Header: //depot/sw/src/dk/mdk/manlib/manlibInst.h#1 $"

// temperature control models
#define TMP_LB300                       0x01

// spectrum analyzer models
#define SPA_E4404B                      0x01
#define SPA_E4405B                      SPA_E4404B  // define them to be same for now
#define SPA_8595E                       0x02
#define SPA_R3162                       0x03
#define SPA_FSL                         0x04

#define SPA_DET_NEG                     0x01
#define SPA_DET_POS                     0x02
#define SPA_DET_SAMPLE                  0x03

#define SPA_E4404B_MAX_SWEEP_POINTS     401
#define SPA_E4405B_MAX_SWEEP_POINTS     401
#define SPA_8595E_MAX_SWEEP_POINTS      401
#define SPA_R3162_MAX_SWEEP_POINTS      1001
#define SPA_MAX_SWEEP_POINTS            SPA_R3162_MAX_SWEEP_POINTS
#define SPA_FSL_MAX_SWEEP_POINTS		501


// power meter models
#define PM_436A                         0x01
#define PM_E4416A                       0x02
#define PM_4531                         0X03
#define NRP_Z11                         0x04

// attenuator models
#define ATT_11713A                      0x01
#define ATT_11713A_110                  0x02

// power supply models
#define PS_E3631A                       0x01
#define PS_P6V                          0x01
#define PS_P25V                         0x02
#define PS_N25V                         0x03

// digital multi_meters models
#define DMM_FLUKE_45				    0x01

#ifndef SWIG

// Gpib Functions
MANLIB_API int gpibRsp(const int ud);
MANLIB_API void gpibSendIFC(const int board);
MANLIB_API long gpibClear(const int ud);
MANLIB_API long gpibRSC(const int board, const int v);
MANLIB_API long gpibRSP(const int ud, char *spr);
MANLIB_API long gpibSIC(const int board);
MANLIB_API long gpibONL(const int ud, const int v);
MANLIB_API char *gpibRead(const int ud, ...);
MANLIB_API long gpibWrite(const int ud, char *wrt);
MANLIB_API char *gpibQuery(const int ud, char *wrt, ...);

// Temperature Functions
MANLIB_API unsigned long tempOpen(char *port, const int adr);
MANLIB_API int tempClose(const unsigned long);
MANLIB_API double tempMeasTemp(const unsigned long);
MANLIB_API unsigned long tempSet(const unsigned long, const int setpoint);

// Power Supply Functions
MANLIB_API int psInit(const int adr, const int model); 
MANLIB_API int psSetOutputState(const int ud, const int on_off);
MANLIB_API int psSetVoltage(const int ud, const int channel, 
							  const double voltage, const double current_limit);
MANLIB_API double psMeasCurrent(const int ud, const int channel);

// Attenuator Functions
MANLIB_API int attInit(const int adr, const int model); 
MANLIB_API int attSet(const int ud, const int value);

// Power Meter Functions
MANLIB_API int pmInit(const int adr, const int model); 
MANLIB_API int pmPreset(const int ud, const double trigger_level, const double trigger_delay, const double gate_start, const double gate_length, const double trace_display_length);
MANLIB_API double pmMeasAvgPower(const int ud, const int reset);
MANLIB_API double pmMeasPeakPower(const int ud, const int reset);

// Current Meter (DMM) Functions
MANLIB_API int dmmInit(const int adr, const int model);
MANLIB_API double dmmMeasCurr(const int ud, const int model);

// Spectrum Analyzer Functions
MANLIB_API int spaInit(const int adr, const int model); 
MANLIB_API char *spaMeasPhaseNoise(const int ud, const double center, const double ref_level, const int reset);
MANLIB_API char *spaMeasSpectralFlatness(const int ud, const double center, const double ref_level, const int reset);
MANLIB_API double spaMeasChannelPower(const int ud, const double center, const double span, const double ref_level,
                                      const double rbw, const double vbw, const double int_bw, const int det_mode,
                                      const int averages, const int max_hold, const int reset);
MANLIB_API double spaMeasSpectralDensity(const int ud, const double center, const double ref_level, const int reset);
MANLIB_API char *spaMeasOutOfBandEm(const int ud, const double ref_level, const int reset);
MANLIB_API double spaMeasHarmonics(const int ud, const double center, const double vbw,
							const double ref_level, const int reset);
MANLIB_API double spaMeasBandEdge(const int ud, const double freq_start, const double freq_stop, const double vbw,
							const double ref_level, const int reset);
MANLIB_API double spaMeasTelecSpurious(const int ud, const double freq_start, const double freq_stop,
							const double ref_level, const int reset);
MANLIB_API double spaMeasTelecEmission2(const int ud, const double freq_start, const double freq_stop,
							const double ref_level, const int reset);
MANLIB_API double spaMeasPpsdUnii(const int ud, const double center, const double span,
							const double ref_level, const int reset);
MANLIB_API double spaMeasPpsdDts(const int ud, const double center, const double span,
							const double ref_level, const int reset);
MANLIB_API double spaMeasEbw(const int ud, const double center, const double span, const double ref_level, const int reset);
MANLIB_API double spaMeasPkPwr(const int ud, const double center, const double vbw,
							const double ebw, const double ref_level, const int reset);
MANLIB_API char *spaGetModelNo(const int ud, const int reset);
MANLIB_API char *spaMeasTxSpurs(const int ud, const double freq_start, const double freq_stop, const double spur_thresh, double *, double *, const int reset);
MANLIB_API double spaMeasPkAvgRatio(const int ud, const double center, const double ref_level, const int reset);
MANLIB_API double spaMeasFreqDev(const int ud, const double center, const double ref_level, const int reset);
MANLIB_API char *spaMeasTxSpuriousEm(const int ud, const double ref_level, const int reset);
MANLIB_API char *spaMeasTxSpuriousEmLite(const int ud, const double ref_level, const int reset);
MANLIB_API double spaMeasTxPwrDev(const int ud, const double center, const double ref_level, const int reset);
MANLIB_API char *spaMeasRxSpuriousEm(const int ud, const double ref_level, const int reset);
MANLIB_API char *spaMeasRxSpuriousEmLite(const int ud, const double ref_level, const int reset);
MANLIB_API double spaMeasOBW(const int ud, const double center, const double span, const double ref_level, const int reset);
MANLIB_API char *spaMeasACP(const int ud, const double center, const double ref_level, const int reset);
MANLIB_API int spaMeasSpectralMask (const int ud, const double center, const double span, 
							const int reset, const int verbose, const int output);
MANLIB_API int spaMeasSpectralMaskHt20 (const int ud, const double center, const double span, 
							const int reset, const int verbose, const int output);
MANLIB_API int spaMeasSpectralMaskHt40 (const int ud, const double center, const double span, 
							const int reset, const int verbose, const int output);
MANLIB_API int spaMeasSpectralMask11b (const int ud, const double center, const double span, 
							const int reset, const int verbose, const int output);
MANLIB_API char *spaMeasRestrictedBand (const int ud, const double center, const double span,
							const double edge_freq, const int reset);
MANLIB_API int spaMeasPHSSpectralMask(const int ud, const double center, const double span, const int reset,
                                   const int verbose, const int output);
MANLIB_API int spaMeasDSRCSpectralMask(const int ud, const double center, const double span, const int reset,
								   const int bandwidth, const int verbose, const int output);
MANLIB_API void setCustomMask(short maskArrayLegacy[4][2], short maskArray11nHt20[4][2], short maskArray11nHt40[4][2]);

#endif // #ifndef SWIG

#ifdef __cplusplus
}
#endif

#endif // __INCmanlibinsth
