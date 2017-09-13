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


#ifndef _SPECTRAL_H_
#define _SPECTRAL_H_

#include <osdep.h>
#include "sys/queue.h"

#ifdef WIN32
#include "spectral_types.h"
#endif
#include "spectral_classifier.h"
#include "spectralscan_classifier.h"
#include "spec_msg_proto.h"

#ifdef __NetBSD__
#include <net/if_media.h>
//#else
//#include <net80211/if_media.h>
#endif
//#include <net80211/ieee80211_var.h>

#include <ath_dev.h>
#include "ath_internal.h"
#include "if_athioctl.h"
//#include "if_athvar.h"
#include "ah.h"
#include "ah_desc.h"
#include "spectral_ioctl.h"
#include "spectral_data.h"

enum {
        ATH_DEBUG_SPECTRAL       = 0x00000100,   /* Minimal SPECTRAL debug */
        ATH_DEBUG_SPECTRAL1      = 0x00000200,   /* Normal SPECTRAL debug */
        ATH_DEBUG_SPECTRAL2      = 0x00000400,   /* Maximal SPECTRAL debug */
        ATH_DEBUG_SPECTRAL3      = 0x00000800,   /* matched filterID display */
};

#define MAX_SPECTRAL_PAYLOAD 1500
extern int BTH_MIN_NUMBER_OF_FRAMES;
extern int spectral_debug_level;

#define SPECTRAL_DPRINTK(sc, _m, _fmt, ...) do {             \
  if ((_m) <= spectral_debug_level) {               \
        printk(_fmt, __VA_ARGS__);  \
  } \
} while (0)


#define	N(a)	(sizeof(a)/sizeof(a[0]))

#define SPECTRAL_MIN(a,b) ((a)<(b)?(a):(b))
#define SPECTRAL_MAX(a,b) ((a)>(b)?(a):(b))
#define SPECTRAL_DIFF(a,b) (SPECTRAL_MAX(a,b) - SPECTRAL_MIN(a,b))
#define SPECTRAL_ABS_DIFF(a,b) (SPECTRAL_MAX(a,b) - SPECTRAL_MIN(a,b))

#define MAX_SPECTRAL_PAYLOAD 1500

#define SPECTRAL_TSMASK              0xFFFFFFFF      /* Mask for time stamp from descriptor */
#define SPECTRAL_TSSHIFT             32              /* Shift for time stamp from descriptor */
#define	SPECTRAL_TSF_WRAP		0xFFFFFFFFFFFFFFFFULL	/* 64 bit TSF wrap value */
#define	SPECTRAL_64BIT_TSFMASK	0x0000000000007FFFULL	/* TS mask for 64 bit value */


#define SPECTRAL_HT20_NUM_BINS                      56
#define SPECTRAL_HT20_FFT_LEN                       56
#define SPECTRAL_HT20_DC_INDEX                      (SPECTRAL_HT20_FFT_LEN / 2)

//Mon Oct 19 13:01:01 PDT 2009 - read BMAPWT and MAXINDEX for HT20 differently
#define SPECTRAL_HT20_BMAPWT_INDEX                  56
#define SPECTRAL_HT20_MAXINDEX_INDEX                58
    
#define SPECTRAL_HT40_NUM_BINS                      64
#define SPECTRAL_HT40_TOTAL_NUM_BINS                128
#define SPECTRAL_HT40_FFT_LEN                       128
#define SPECTRAL_HT40_DC_INDEX                      (SPECTRAL_HT40_FFT_LEN / 2)
#define SPECTRAL_HT20_DATA_LEN                      60
#define SPECTRAL_HT20_TOTAL_DATA_LEN                (SPECTRAL_HT20_DATA_LEN + 3)
#define SPECTRAL_HT40_DATA_LEN                      135
#define SPECTRAL_HT40_TOTAL_DATA_LEN                (SPECTRAL_HT40_DATA_LEN + 3)

#define SPECTRAL_HT40_LOWER_BMAPWT_INDEX            128
#define SPECTRAL_HT40_HIGHER_BMAPWT_INDEX           131
#define SPECTRAL_HT40_LOWER_MAXINDEX_INDEX          130
#define SPECTRAL_HT40_HIGHER_MAXINDEX_INDEX         133

#define	SPECTRAL_MAX_EVENTS			1024		/* Max number of spectral events which can be q'd */

#define CLASSIFY_TIMEOUT_S             2
#define CLASSIFY_TIMEOUT_MS            (CLASSIFY_TIMEOUT_S * 1000)
#define DEBUG_TIMEOUT_S                1
#define DEBUG_TIMEOUT_MS               (DEBUG_TIMEOUT_S * 1000)

#define SPECTRAL_EACS_RSSI_THRESH      30 /* Use a RSSI threshold of 10dB(?) above the noise floor*/


#ifdef WIN32
#pragma pack(push, spectral, 1)
#define __ATTRIB_PACK
#else
#ifndef __ATTRIB_PACK
#define __ATTRIB_PACK __attribute__ ((packed))
#endif
#endif

typedef struct ht20_bin_mag_data {
    u_int8_t bin_magnitude[SPECTRAL_HT20_NUM_BINS];
} __ATTRIB_PACK HT20_BIN_MAG_DATA;

typedef struct ht40_bin_mag_data {
    u_int8_t bin_magnitude[SPECTRAL_HT40_NUM_BINS];
} __ATTRIB_PACK HT40_BIN_MAG_DATA;

#ifndef OLD_MAGDATA_DEF
typedef struct max_mag_index_data {
        u_int8_t all_bins1;
        u_int8_t max_mag_bits29;
        u_int8_t all_bins2;
}__ATTRIB_PACK MAX_MAG_INDEX_DATA;
#else
typedef struct max_mag_index_data {
        u_int8_t 
            max_mag_bits01:2,
            bmap_wt:6;
        u_int8_t max_mag_bits29;
        u_int8_t 
            max_index_bits05:6,
            max_mag_bits1110:2;
}__ATTRIB_PACK MAX_MAG_INDEX_DATA;
#endif
typedef struct ht20_fft_packet {
    HT20_BIN_MAG_DATA lower_bins;
    MAX_MAG_INDEX_DATA  lower_bins_max;
    u_int8_t       max_exp;
} __ATTRIB_PACK HT20_FFT_PACKET;

typedef struct ht40_fft_packet {
    HT40_BIN_MAG_DATA lower_bins;
    HT40_BIN_MAG_DATA upper_bins;
    MAX_MAG_INDEX_DATA  lower_bins_max;
    MAX_MAG_INDEX_DATA  upper_bins_max;
    u_int8_t       max_exp;
} __ATTRIB_PACK HT40_FFT_PACKET;

#ifdef WIN32
#pragma pack(pop, spectral)
#endif
#ifdef __ATTRIB_PACK
#undef __ATTRIB_PACK
#endif

struct spectral_pulseparams {
        u_int64_t       p_time;                 /* time for start of pulse in usecs*/
        u_int8_t        p_dur;                  /* Duration of pulse in usecs*/
        u_int8_t        p_rssi;                 /* Duration of pulse in usecs*/
};

struct spectral_event {
	u_int32_t	se_ts;		/* Original 15 bit recv timestamp */
	u_int64_t	se_full_ts;	/* 64-bit full timestamp from interrupt time */
	u_int8_t	se_rssi;	/* rssi of spectral event */
	u_int8_t	se_bwinfo;	/* rssi of spectral event */
	u_int8_t	se_dur;		/* duration of spectral pulse */
	u_int8_t	se_chanindex;	/* Channel of event */
	STAILQ_ENTRY(spectral_event)	se_list;	/* List of spectral events */
};




struct spectral_stats {
        u_int32_t       num_spectral_detects;      /* total num. of spectral detects */
        u_int32_t       total_phy_errors;
        u_int32_t       owl_phy_errors;
        u_int32_t       pri_phy_errors;
        u_int32_t       ext_phy_errors;
        u_int32_t       dc_phy_errors;
        u_int32_t       early_ext_phy_errors;
        u_int32_t       bwinfo_errors;
        u_int32_t       datalen_discards;
        u_int32_t       rssi_discards;
        u_int64_t       last_reset_tstamp;
};

#define HAL_SPECTRAL_DEFAULT_SS_COUNT     128
#define HAL_SPECTRAL_DEFAULT_SS_SHORT_REPORT AH_TRUE
#define HAL_SPECTRAL_DEFAULT_SS_PERIOD     18
#define HAL_SPECTRAL_DEFAULT_SS_FFT_PERIOD      8

typedef spinlock_t spectralq_lock_t;

#define  SPECTRAL_LOCK_INIT(_ath_spectral)     spin_lock_init(&(_ath_spectral)->ath_spectral_lock)
#define  SPECTRAL_LOCK_DESTROY(_ath_spectral)  spin_lock_destroy(&(_ath_spectral)->ath_spectral_lock)
#define  SPECTRAL_LOCK(_ath_spectral)          spin_lock(&(_ath_spectral)->ath_spectral_lock)
#define  SPECTRAL_UNLOCK(_ath_spectral)        spin_unlock(&(_ath_spectral)->ath_spectral_lock)

struct ath_spectral {
	spectralq_lock_t        ath_spectral_lock;
    int16_t			spectral_curchan_radindex;	/* cur. channel spectral index */
	int16_t			spectral_extchan_radindex;	/* extension channel spectral index */
	u_int32_t		spectraldomain;	/* cur. SPECTRAL domain */
	u_int32_t		spectral_proc_phyerr;/* Flags for Phy Errs to process */

	HAL_SPECTRAL_PARAM	spectral_defaultparams; /* Default phy params per spectral state */
	struct spectral_stats        ath_spectral_stats;		/* SPECTRAL related stats */
        struct spectral_event        *events;        /* Events structure */
        unsigned int 
        sc_spectral_ext_chan_ok:1,	/* Can spectral be detected on the extension channel? */
        sc_spectral_combined_rssi_ok:1, /* Can use combined spectral RSSI? */
        sc_spectral_20_40_mode:1; /* Is AP in 20-40 mode? */

/* BEGIN - EACS related variables and counters */
        int ctl_chan_loading;
        int ctl_chan_frequency;
        int ctl_chan_noise_floor;

        int ext_chan_loading;
        int ext_chan_frequency;
        int ext_chan_noise_floor;

        int8_t ctl_eacs_rssi_thresh;
        int8_t ext_eacs_rssi_thresh;

        int8_t ctl_eacs_avg_rssi;
        int8_t ext_eacs_avg_rssi;

        int ctl_eacs_spectral_reports;
        int ext_eacs_spectral_reports;

        int  ctl_eacs_interf_count;
        int  ext_eacs_interf_count;

        int  ctl_eacs_duty_cycle;
        int  ext_eacs_duty_cycle;

        int eacs_this_scan_spectral_data;
/* END - EACS related variables and counters */

        int upper_is_control;
        int upper_is_extension;
        int lower_is_control;
        int lower_is_extension;

        u_int8_t		sc_spectraltest_ieeechan;	/* IEEE channel number to return to after a spectral mute test */
        struct sock             *spectral_sock;
        struct sk_buff          *spectral_skb;
        struct nlmsghdr         *spectral_nlh;
        u_int32_t               spectral_pid;

        int                     spectral_numbins;
        int                     spectral_fft_len;
        int                     spectral_data_len;
        int                     spectral_max_index_offset;
        int                     spectral_upper_max_index_offset;
        int                     spectral_lower_max_index_offset;
        int                     spectral_dc_index;
        int                     send_single_packet;
        int                     spectral_sent_msg;
        int                     classify_scan;
        os_timer_t   	        classify_timer;   
        os_timer_t   	        debug_timer;   

        SPECTRAL_BURST          current_burst;
        SPECTRAL_BURST          prev_burst;
	HAL_SPECTRAL_PARAM      params;     
        SPECTRAL_CLASSIFIER_PARAMS classifier_params;
        struct ss               bd_upper;
        struct ss               bd_lower;
        int                     last_capture_time;
        struct INTERF_SRC_RSP   lower_interf_list;
        struct INTERF_SRC_RSP   upper_interf_list;

        int num_spectral_data;
        int total_spectral_data;
        u_int64_t cached_phy_err;
        u_int64_t scan_start_tstamp;
        int this_scan_phy_err; 
        u_int32_t last_tstamp;
        u_int32_t first_tstamp;
        int max_rssi;


        int detects_control_channel;
        int detects_extension_channel;
        int detects_below_dc;
        int detects_above_dc;
        u_int32_t spectral_samp_count;
};

struct samp_msg_params {
    struct ath_softc *sc;  
    int8_t rssi;
    int8_t lower_rssi; 
    int8_t upper_rssi;
    uint16_t bwinfo;
    uint16_t datalen; 
    uint32_t tstamp; 
    uint32_t last_tstamp; 
    uint16_t max_mag;
    uint16_t max_index;
    uint8_t max_exp; 
    int peak; 
    int8_t nb_lower;
    int8_t nb_upper;
    uint16_t max_lower_index;
    uint16_t max_upper_index;
    SPECTRAL_CLASSIFIER_PARAMS classifier_params;
    int pwr_count;
    u_int8_t **bin_pwr_data;
    u_int16_t freq;
    u_int16_t freq_loading;
    struct INTERF_SRC_RSP interf_list;
};


int         spectral_process_spectralevent(struct ath_softc *sc, HAL_CHANNEL *chan);
int         spectral_attach(struct ath_softc *sc);
void        spectral_detach(struct ath_softc *sc);
int         spectral_scan_enable(struct ath_softc *sc);
int         spectral_check_chirping(struct ath_softc *sc, struct ath_desc *ds,
                               int is_ctl, int is_ext, int *slope, int *is_dc);
void        ath_process_spectraldata(struct ath_softc *sc, struct ath_buf *bf, struct ath_rx_status *rxs,
                               u_int64_t fulltsf);
int         spectral_control(ath_dev_t dev, u_int id,
                            void *indata, u_int32_t insize,
                            void *outdata, u_int32_t *outsize);
int         spectral_get_thresholds(struct ath_softc *sc, HAL_SPECTRAL_PARAM *param);
void        spectral_clear_stats(struct ath_softc *sc);

/* NETLINK related declarations */
#if !defined(WIN32) && !__APPLE__
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
void spectral_nl_data_ready(struct sock *sk, int len);
#else
void spectral_nl_data_ready(struct sk_buff *skb);
#endif /* VERSION CHECK */
#endif /* WIN32 defined*/
int spectral_init_netlink(struct ath_softc *sc);
void spectral_unicast_msg(struct ath_softc *sc);
void spectral_bcast_msg(struct ath_softc *sc);
int spectral_destroy_netlink(struct ath_softc *sc);
void spectral_prep_skb(struct ath_softc *sc);
int spectral_destroy_netlink(struct ath_softc *sc);

#ifdef SPECTRAL_CLASSIFIER_IN_KERNEL
void init_classifier(struct ath_softc *sc);
void classifier_initialize(struct ss *spectral_lower, struct ss *spectral_upper);
void classifier(struct ss *bd, int timestamp, int last_capture_time, int rssi, int narrowband, int peak_index);
OS_TIMER_FUNC(spectral_classify_scan);
#endif

void enable_beacons(struct ath_softc *sc);
void disable_beacons(struct ath_softc *sc);

OS_TIMER_FUNC(spectral_debug_timeout);

void print_classifier_counts(struct ath_softc *sc, struct ss *bd, const char *print_str);
void print_detection(struct ath_softc *sc);
void print_phy_err_stats(struct ath_softc *sc);
void print_fft_ht40_bytes(HT40_FFT_PACKET *fft_40, struct ath_softc *sc);
void reg_dump(struct ath_softc *sc);
void print_config_regs(struct ath_softc *sc);
void printNoiseFloor(struct ath_hal *ah);
void print_ht40_bin_mag_data(HT40_BIN_MAG_DATA *bmag, struct ath_softc *sc);
void print_max_mag_index_data(MAX_MAG_INDEX_DATA *imag, struct ath_softc *sc);
void print_fft_ht40_packet(HT40_FFT_PACKET *fft_40, struct ath_softc *sc);
void print_hex_fft_ht20_packet(HT20_FFT_PACKET *fft_20, struct ath_softc *sc);

void fake_ht40_data_packet(HT40_FFT_PACKET *ht40pkt, struct ath_softc *sc);
void send_fake_ht40_data(struct ath_softc *sc);
//u_int8_t return_max_value(u_int8_t* datap, u_int8_t numdata, u_int8_t *max_index, struct ath_softc *sc);
u_int8_t return_max_value(u_int8_t* datap, u_int8_t numdata, u_int8_t *max_index, struct ath_softc *sc, u_int8_t *strong_bins);
void process_mag_data(MAX_MAG_INDEX_DATA *imag, u_int16_t *mmag, u_int8_t *bmap_wt, u_int8_t *max_index);

void process_fft_ht20_packet(HT20_FFT_PACKET *fft_20, struct ath_softc *sc, u_int16_t *max_mag, u_int8_t* max_index, int *narrowband, int8_t *rssi, int *bmap);

void process_fft_ht40_packet(HT40_FFT_PACKET *fft_40, struct ath_softc *sc, u_int16_t *max_mag_lower, u_int8_t* max_index_lower, u_int16_t *max_mag_upper, u_int8_t* max_index_upper, u_int16_t *max_mag, u_int8_t* max_index, int *narrowband_lower, int *narrowband_upper, int8_t *rssi_lower, int8_t*rssi_upper, int *bmap_lower, int *bmap_upper);

/* SAMP declarations - SPECTRAL ANALYSIS MESSAGING PROTOCOL */
void spectral_add_interf_samp_msg(struct samp_msg_params *params, struct ath_softc *sc);
void spectral_create_samp_msg(struct samp_msg_params *params);
void spectral_send_eacs_msg(struct ath_softc *sc);
void spectral_create_msg(struct ath_softc *sc, uint16_t rssi, uint16_t bwinfo, uint16_t datalen, uint32_t tstamp, uint16_t max_mag, uint16_t max_index, int peak);
void print_samp_msg (SPECTRAL_SAMP_MSG *samp, struct ath_softc *sc);

/* Spectral commands that can be issued from the commandline using spectraltool */
void start_spectral_scan(struct ath_softc *sc);
void start_classify_scan(struct ath_softc *sc);
void stop_current_scan(struct ath_softc *sc);
int spectral_set_thresholds(struct ath_softc *sc, const u_int32_t threshtype,const u_int32_t value);
u_int32_t spectral_round(int32_t val);

#if 0
int8_t adjust_rssi (struct ath_hal *ah, int8_t rssi, int upper, int lower, int convert_to_dbm);
int8_t fix_rssi_for_classifier (struct ath_hal *ah, u_int8_t rssi_val, int upper, int lower);
int8_t fix_combined_rssi (struct ath_hal *ah, u_int8_t rssi_val);
int8_t fix_rssi (struct ath_hal *ah, u_int8_t rssi_val, int upper, int lower);

#else
int8_t fix_maxindex_inv_only (u_int8_t val);
int8_t adjust_rssi_with_nf_noconv_dbm (struct ath_hal *ah, int8_t rssi, int upper, int lower);
int8_t adjust_rssi_with_nf_conv_dbm (struct ath_hal *ah, int8_t rssi, int upper, int lower);
int8_t adjust_rssi_with_nf_dbm (struct ath_hal *ah, int8_t rssi, int upper, int lower, int convert_to_dbm); 
int8_t fix_rssi_for_classifier (struct ath_hal *ah, u_int8_t rssi_val, int upper, int lower);
int8_t fix_rssi_inv_only (struct ath_hal *ah, u_int8_t rssi_val);
int8_t fix_rssi (struct ath_hal *ah, u_int8_t rssi_val, int upper, int lower);
#endif

extern int is_spectral_phyerr(struct ath_softc *sc, struct ath_buf *buf, 
                              struct ath_rx_status* rxs);

/* BEGIN EACS related declarations */
void spectral_read_mac_counters(struct ath_softc *sc);
void init_chan_loading(struct ath_softc *sc);
int8_t get_nfc_ctl_rssi(struct ath_hal *ah, int8_t rssi, int8_t *ctl_nf);
int8_t get_nfc_ext_rssi(struct ath_hal *ah, int8_t rssi, int8_t *ext_nf );
void update_eacs_counters(struct ath_softc *sc, int8_t nfc_ctl_rssi, int8_t nfc_ext_rssi);
int get_eacs_extension_duty_cycle(struct ath_softc *sc);
int get_eacs_control_duty_cycle(struct ath_softc *sc);
void update_eacs_thresholds(struct ath_softc *sc);

/* END EACS related declarations */

#endif  /* _SPECTRAL_H_ */
