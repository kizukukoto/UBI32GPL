/*
 *  Copyright © 2001 Atheros Communications, Inc.,  All Rights Reserved.
 */
/* mInst.h - Local functions and structures for mInst.c */

#ifndef	__INCminsth
#define	__INCminsth

#ident  "ACI $Id: //depot/sw/src/dk/mdk/manlib/mInst.h#1 $, $Header: //depot/sw/src/dk/mdk/manlib/mInst.h#1 $"

// thermal unit
#define STX                             0x2
#define ETX                             0x3
#define ACK                             0x6
#define TEMP_CTRL_PREAMBLE              'L'
#define TEMP_CTRL_ADDRESS               1
#define TEMP_CTRL_MEAS_TEMP             0
#define TEMP_CTRL_GET_TEMP              100
#define TEMP_CTRL_SET_TEMP              200
#define TEMP_CTRL_REMOTE                400
#define TEMP_CTRL_LOCAL                 401


#ifndef COM_DEFS
#define COM_DEFS
// com port
#define READ_BUF_SIZE                   512
#define WRITE_BUF_SIZE                  512
#define COM_ERROR_GETHANDLE             1    
#define COM_ERROR_BUILDDCB              2
#define COM_ERROR_CONFIGDEVICE          4
#define COM_ERROR_CONFIGBUFFERS         8
#define COM_ERROR_SETDTR                16
#define COM_ERROR_CLEARDTR              32
#define COM_ERROR_PURGEBUFFERS          64
#define COM_ERROR_READ                  128 
#define COM_ERROR_WRITE                 256
#define COM_ERROR_MASK                  512
#define COM_ERROR_TIMEOUT               1024
#define COM_ERROR_INVALID_HANDLE        2048
#endif

// internal function definitions
HANDLE com_open(char *port, const int adr);
int com_close(const HANDLE);
char *com_read(const HANDLE);
unsigned long com_write(const HANDLE, char *wrt);
char *temp_ctrl_checksum(char *buf);
double *spa_802_11a_mask(const double f0, const double df, const double ref_level);
double *spa_802_11b_mask(const double f0, const double df, const double ref_level);
double *spa_phs_mask(const double f0, const double df, const double ref_level);
double *spa_get_trace(const int ud);
int spa_plot_mask(char *gp_script, char *gp_data_file, char *gp_output_file, char *gp_label);
int spa_compare_mask(double *trace, double *mask, const double f, const double df, const int verbose, const int output);
double spa_lookup_point(double *trace, const double f0, const double df, const double f);
double spa_calc_obwl(double *trace, const double tot);
double spa_calc_obwu(double *trace, const double tot);
double interp(const double y, const double y1, const double y2);
double *spa_802_11a_dsrc_mask(const double f0, const double df, 
									 const double ref_level, int bw);
double *spa_802_11n_ht40_mask(const double f0, const double df, const double ref_level);
double *spa_802_11n_ht20_mask(const double f0, const double df, const double ref_level);
double *spa_custom_mask(double f0, double df, double ref_level, A_INT16 customMask[4][2]);
#endif // __INCminsth

