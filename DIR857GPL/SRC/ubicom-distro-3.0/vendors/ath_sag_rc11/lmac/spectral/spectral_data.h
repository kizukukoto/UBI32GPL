/*
 * Copyright (c) 2010, Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _SPECTRAL_DATA_H_
#define _SPECTRAL_DATA_H_

#include "spec_msg_proto.h"

#ifndef MAX_SPECTRAL_MSG_ELEMS
#define MAX_SPECTRAL_MSG_ELEMS 10
#endif

#ifndef NETLINK_ATHEROS
#define NETLINK_ATHEROS              (NETLINK_GENERIC + 1)
#endif

#ifdef WIN32
#pragma pack(push, spectral_data, 1)
#define __ATTRIB_PACK
#else
#ifndef __ATTRIB_PACK
#define __ATTRIB_PACK __attribute__ ((packed))
#endif
#endif

typedef struct spectral_classifier_params {
        int spectral_20_40_mode; /* Is AP in 20-40 mode? */
        int spectral_dc_index;
        int spectral_dc_in_mhz;
        int upper_chan_in_mhz;
        int lower_chan_in_mhz;
} __ATTRIB_PACK SPECTRAL_CLASSIFIER_PARAMS;


#define MAX_NUM_BINS 128

typedef struct spectral_data {
	int16_t		spectral_data_len;
	int16_t		spectral_rssi;
	int16_t		spectral_bwinfo;	
	int32_t		spectral_tstamp;	
	int16_t		spectral_max_index;	
	int16_t		spectral_max_mag;	
} __ATTRIB_PACK SPECTRAL_DATA;

struct spectral_scan_data {
    u_int16_t chanMag[128];
    u_int8_t  chanExp;
    int16_t   primRssi;
    int16_t   extRssi;
    u_int16_t dataLen;
    u_int32_t timeStamp;
    int16_t   filtRssi;
    u_int32_t numRssiAboveThres;
    int16_t   noiseFloor;
    u_int32_t center_freq;
};





typedef struct spectral_msg {

        int16_t      num_elems;
        SPECTRAL_DATA data_elems[MAX_SPECTRAL_MSG_ELEMS];
} SPECTRAL_MSG;

typedef struct spectral_samp_data {
	int16_t		spectral_data_len;
	int16_t		spectral_rssi;
        int8_t        spectral_combined_rssi;
        int8_t        spectral_upper_rssi;
        int8_t        spectral_lower_rssi;
        u_int8_t        spectral_max_scale;  
	int16_t		spectral_bwinfo;	
	int32_t		spectral_tstamp;	
	int16_t		spectral_max_index;	
	int16_t		spectral_max_mag;	
	u_int8_t	spectral_max_exp;	
	int32_t		spectral_last_tstamp;	
	int16_t		spectral_upper_max_index;	
	int16_t		spectral_lower_max_index;	
	u_int8_t	spectral_nb_upper;	
	u_int8_t	spectral_nb_lower;	
        SPECTRAL_CLASSIFIER_PARAMS classifier_params;
        u_int16_t       bin_pwr_count;
        u_int8_t        bin_pwr[MAX_NUM_BINS];
        struct INTERF_SRC_RSP interf_list;
} __ATTRIB_PACK SPECTRAL_SAMP_DATA;

typedef struct spectral_samp_msg {

        u_int16_t      freq;
        u_int16_t      freq_loading;
        u_int8_t        cw_interferer;
        SPECTRAL_SAMP_DATA samp_data;
} __ATTRIB_PACK SPECTRAL_SAMP_MSG;

#ifdef WIN32
#pragma pack(pop, spectral_data)
#endif
#ifdef __ATTRIB_PACK
#undef __ATTRIB_PACK
#endif

#endif
