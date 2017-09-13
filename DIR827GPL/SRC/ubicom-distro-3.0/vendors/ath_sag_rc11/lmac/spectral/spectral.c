/*
 * Copyright (c) 2002-2009 Atheros Communications, Inc.
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
#if ATH_SUPPORT_SPECTRAL
#include <osdep.h>
#include "ah.h"
#include "spectral.h"
#include "ath_internal.h"

int BTH_MIN_NUMBER_OF_FRAMES=8;
int spectral_debug_level=ATH_DEBUG_SPECTRAL;
int nobeacons=0;
int nfrssi=0;
int maxholdintvl=0;

void
spectral_clear_stats(struct ath_softc *sc)
{
    struct ath_spectral *spectral = sc->sc_spectral;
    if (spectral == NULL) 
            return;
    OS_MEMZERO(&spectral->ath_spectral_stats, sizeof (struct spectral_stats));
    spectral->ath_spectral_stats.last_reset_tstamp = ath_hal_gettsf64(sc->sc_ah);
}


int
spectral_attach(struct ath_softc *sc)
{

    struct ath_spectral *spectral = sc->sc_spectral;

    if (spectral != NULL) {
		SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL, "%s: sc_spectral was not NULL\n",
			__func__);
		return AH_FALSE;
	}
   if (ath_hal_getcapability(sc->sc_ah, HAL_CAP_PHYDIAG,
   		  HAL_CAP_RADAR, NULL) != HAL_OK) {
           SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL, "%s: HW not FFT capable\n",
    	    __func__);
           spectral = NULL;
           return AH_FALSE;
    }

    if (ath_hal_getcapability(sc->sc_ah, HAL_CAP_SPECTRAL_SCAN, 0, NULL) != HAL_OK) {
	        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL, "%s: HW not spectral capable\n",
			__func__);
           spectral = NULL;
            return AH_FALSE;
	}
    spectral = (struct ath_spectral *)OS_MALLOC(sc->sc_osdev, sizeof(struct ath_spectral), GFP_KERNEL);
    if (spectral == NULL) {
		SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL,
			"%s: ath_spectral allocation failed\n", __func__);
		return AH_FALSE;
    }

    OS_MEMZERO(spectral, sizeof (struct ath_spectral));
    SPECTRAL_LOCK_INIT(spectral);
    sc->sc_spectral = spectral;
    spectral_clear_stats(sc);

#ifdef SPECTRAL_DEBUG_TIMER
    OS_INIT_TIMER(sc->sc_osdev, &spectral->debug_timer, spectral_debug_timeout, sc);
#endif

#ifndef WIN32
    spectral_init_netlink(sc);
#endif
    SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"SPECTRAL maxhold set to %d nobeacons=%d nfrssi=%d\n", maxholdintvl, nobeacons, nfrssi);

#ifdef SPECTRAL_CLASSIFIER_IN_KERNEL
    OS_INIT_TIMER(sc->sc_osdev, &spectral->classify_timer, spectral_classify_scan, sc);
    init_classifier(sc);
#endif
    return AH_TRUE;
}

void
spectral_detach(struct ath_softc *sc)
{
	struct ath_spectral *spectral = sc->sc_spectral;

	if (spectral == NULL) {
	        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1, "%s: sc_spectral is NULL\n",
			__func__);
		return;
	}

#ifdef SPECTRAL_CLASSIFIER_IN_KERNEL
        OS_CANCEL_TIMER(&spectral->classify_timer);
#endif
#ifdef SPECTRAL_DEBUG_TIMER
        OS_CANCEL_TIMER(&spectral->debug_timer);
#endif
#ifndef WIN32
        spectral_destroy_netlink(sc); 
#endif
       SPECTRAL_LOCK_DESTROY(spectral);

       if (spectral) {
                OS_FREE(spectral);
                spectral = NULL;
        }
}
void print_chan_loading_details(struct ath_softc *sc)
{
	struct ath_spectral *spectral=sc->sc_spectral;

        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"ctl_chan_freq=%d\n",spectral->ctl_chan_frequency);
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"ctl_interf_count=%d\n",spectral->ctl_eacs_interf_count);
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"ctl_duty_cycle=%d\n",spectral->ctl_eacs_duty_cycle);
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"ctl_chan_loading=%d\n",spectral->ctl_chan_loading);
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"ctl_nf=%d\n",spectral->ctl_chan_noise_floor);

        if (spectral->sc_spectral_20_40_mode) {
            // HT40 mode
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"ext_chan_freq=%d\n",spectral->ext_chan_frequency);
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"ext_interf_count=%d\n",spectral->ext_eacs_interf_count);
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"ext_duty_cycle=%d\n",spectral->ext_eacs_duty_cycle);
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"ext_chan_loading=%d\n",spectral->ext_chan_loading);
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"ext_nf=%d\n",spectral->ext_chan_noise_floor);
        }
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1, "%s this_scan_spectral_data count = %d\n", __func__, spectral->eacs_this_scan_spectral_data);
}

void init_chan_loading(struct ath_softc *sc)
{
	struct ath_spectral *spectral=sc->sc_spectral;

        /* Check if channel change has happened in this reset */
        if(spectral->ctl_chan_frequency != sc->sc_curchan.channel) {
            print_chan_loading_details(sc);
            spectral->eacs_this_scan_spectral_data=0;
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1, "%s Resetting this_scan_spectral_data count = 0 for channel %d\n", __func__, sc->sc_curchan.channel);
        }

        if (spectral->sc_spectral_20_40_mode) {
            // HT40 mode
            spectral->ext_eacs_interf_count=0;
            spectral->ext_eacs_duty_cycle=0;
            spectral->ext_chan_loading=0;
            spectral->ext_chan_noise_floor=0;
            spectral->ext_chan_frequency=sc->sc_extchan.channel;
        }
        spectral->ctl_eacs_interf_count=0;
        spectral->ctl_eacs_duty_cycle=0;
        spectral->ctl_chan_loading=0;
        spectral->ctl_chan_noise_floor=0;
        spectral->ctl_chan_frequency=sc->sc_curchan.channel;

        spectral->ctl_eacs_rssi_thresh=SPECTRAL_EACS_RSSI_THRESH;
        spectral->ext_eacs_rssi_thresh=SPECTRAL_EACS_RSSI_THRESH;

        spectral->ctl_eacs_avg_rssi=SPECTRAL_EACS_RSSI_THRESH;
        spectral->ext_eacs_avg_rssi=SPECTRAL_EACS_RSSI_THRESH;

        spectral->ctl_eacs_spectral_reports=0;
        spectral->ext_eacs_spectral_reports=0;
}

void init_upper_lower_flags(struct ath_softc *sc)
{
	struct ath_spectral *spectral=sc->sc_spectral;

        if (spectral->sc_spectral_20_40_mode) {
            // HT40 mode
            if (sc->sc_extchan.channel < sc->sc_curchan.channel) { 
                spectral->lower_is_extension = 1;
                spectral->upper_is_control = 1;
                spectral->lower_is_control = 0;
                spectral->upper_is_extension = 0;
            } else {
                spectral->lower_is_extension = 0;
                spectral->upper_is_control = 0;
                spectral->lower_is_control = 1;
                spectral->upper_is_extension = 1;
            }
        } else {
                // HT20 mode, lower is always control
                spectral->lower_is_extension = 0;
                spectral->upper_is_control = 0;
                spectral->lower_is_control = 1;
                spectral->upper_is_extension = 0;
        }
	SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1, "%s lower_is_ext=%d lower_is_ctl=%d upper_is_ext=%d upper_is_ctl=%d",__func__, spectral->lower_is_extension, spectral->lower_is_control, spectral->upper_is_extension, spectral->upper_is_control);
    
}

int spectral_scan_enable(struct ath_softc *sc)
{
	struct ath_spectral *spectral=sc->sc_spectral;
	u_int32_t rfilt;
	struct ath_hal *ah = sc->sc_ah;
    HAL_SPECTRAL_PARAM spectral_params;
    HAL_CHANNEL *cmp_ch;

	if (spectral == NULL) {
		SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1, "%s: sc_spectral is NULL\n",
			__func__);
		return -EIO;
	}

    rfilt = ath_hal_getrxfilter(ah);
    rfilt &= ~HAL_RX_FILTER_PHYRADAR;
    ath_hal_setrxfilter(ah, rfilt);

    rfilt |= HAL_RX_FILTER_PHYRADAR;
    ath_hal_setrxfilter(ah, rfilt);


    if (!(ath_hal_is_spectral_active(sc->sc_ah))) {
           OS_MEMZERO(&spectral_params, sizeof (HAL_SPECTRAL_PARAM));

            // Initialize spectral parameters to defaults and configure hardware
	        spectral_params.ss_count = HAL_SPECTRAL_DEFAULT_SS_COUNT;
            spectral_params.ss_short_report = HAL_SPECTRAL_DEFAULT_SS_SHORT_REPORT; 
            spectral_params.ss_period = HAL_SPECTRAL_DEFAULT_SS_PERIOD;
            spectral_params.ss_fft_period = HAL_SPECTRAL_DEFAULT_SS_FFT_PERIOD;

            ath_hal_configure_spectral(sc->sc_ah, &spectral_params);
            ath_hal_start_spectral_scan(sc->sc_ah);

        	SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL, "Enabled spectral scan on channel %d\n",
     			sc->sc_curchan.channel);
    } else {
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL, "Spectral scan is already ACTIVE on channel %d\n",
     			sc->sc_curchan.channel);
    }

    spectral_get_thresholds(sc, &sc->sc_spectral->params);

    cmp_ch = (ath_hal_get_extension_channel(sc->sc_ah));    
    if (cmp_ch) {
        /* 20-40 mode*/
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1, "20-40 mode in channel %d\n",
 		sc->sc_curchan.channel);
        sc->sc_extchan = *cmp_ch;
        spectral->spectral_numbins = SPECTRAL_HT40_TOTAL_NUM_BINS;    
        spectral->spectral_fft_len = SPECTRAL_HT40_FFT_LEN;    
        spectral->spectral_data_len = SPECTRAL_HT40_TOTAL_DATA_LEN;    
        spectral->spectral_dc_index = SPECTRAL_HT40_DC_INDEX;
        spectral->spectral_max_index_offset = -1; //only valid in 20 mode

        spectral->spectral_lower_max_index_offset = spectral->spectral_fft_len + 2;
        spectral->spectral_upper_max_index_offset = spectral->spectral_fft_len + 5;
        spectral->sc_spectral_20_40_mode = 1;

        /* Initialize classifier params to be sent to user space classifier */
        if (sc->sc_extchan.channel < sc->sc_curchan.channel) { 
             spectral->classifier_params.lower_chan_in_mhz = sc->sc_extchan.channel;
             spectral->classifier_params.upper_chan_in_mhz = sc->sc_curchan.channel;
        } else {
             spectral->classifier_params.lower_chan_in_mhz = sc->sc_curchan.channel;
             spectral->classifier_params.upper_chan_in_mhz = sc->sc_extchan.channel;
        }
    } else {
            /* 20 only  mode*/
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1, "20 mode in channel %d\n",
 		sc->sc_curchan.channel);
        spectral->spectral_numbins = SPECTRAL_HT20_NUM_BINS;    
        spectral->spectral_dc_index = SPECTRAL_HT20_DC_INDEX;
        spectral->spectral_fft_len = SPECTRAL_HT20_FFT_LEN;    
        spectral->spectral_data_len = SPECTRAL_HT20_TOTAL_DATA_LEN;    
        spectral->spectral_lower_max_index_offset = -1;  // only valid in 20-40 mode
        spectral->spectral_upper_max_index_offset = -1; // only valid in 20-40 mode 
        spectral->spectral_max_index_offset = spectral->spectral_fft_len + 2;
        spectral->sc_spectral_20_40_mode = 0;

        /* Initialize classifier params to be sent to user space classifier */
        spectral->classifier_params.lower_chan_in_mhz = sc->sc_curchan.channel;
        spectral->classifier_params.upper_chan_in_mhz = 0;
    }
    spectral->send_single_packet = 0;
    spectral->classifier_params.spectral_20_40_mode = spectral->sc_spectral_20_40_mode;
    spectral->classifier_params.spectral_dc_index = spectral->spectral_dc_index;

    init_upper_lower_flags(sc);
    init_chan_loading(sc);

#ifdef SPECTRAL_CLASSIFIER_IN_KERNEL
    init_classifier(sc);
#endif
	return 0;
}


int
spectral_control(ath_dev_t dev, u_int id,
                     void *indata, u_int32_t insize,
                     void *outdata, u_int32_t *outsize)
{
	struct ath_softc *sc = ATH_DEV_TO_SC(dev);
	int error = 0, temp_debug;
	HAL_SPECTRAL_PARAM sp_out;
	struct ath_spectral *spectral = sc->sc_spectral;
	struct spectral_ioctl_params *spectralparams;

	if (spectral == NULL) {
		error = -EINVAL;
                SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL, "%s SPECTRAL is null\n", __func__);
		goto bad;
	}

	switch (id) {

	case SPECTRAL_SET_CONFIG:
		if (insize < sizeof(HAL_SPECTRAL_PARAM) || !indata) {
			error = -EINVAL;
			break;
		}
		spectralparams = (struct spectral_ioctl_params *) indata;
		if (!spectral_set_thresholds(sc, SPECTRAL_PARAM_SCAN_COUNT, spectralparams->spectral_count))
			error = -EINVAL;
		if (!spectral_set_thresholds(sc, SPECTRAL_PARAM_FFT_PERIOD, spectralparams->spectral_fft_period))
			error = -EINVAL;
		if (!spectral_set_thresholds(sc, SPECTRAL_PARAM_SCAN_PERIOD, spectralparams->spectral_period))
			error = -EINVAL;
		if (!spectral_set_thresholds(sc, SPECTRAL_PARAM_SHORT_REPORT, (u_int32_t)spectralparams->spectral_short_report))
			error = -EINVAL;
		break;
	case SPECTRAL_GET_CONFIG:
		if (!outdata || !outsize || *outsize <sizeof(struct spectral_ioctl_params)) {
			error = -EINVAL;
			break;
		}
		*outsize = sizeof(struct spectral_ioctl_params);
		ath_hal_get_spectral_config(sc->sc_ah, &sp_out);
		spectralparams = (struct spectral_ioctl_params *) outdata;
		spectralparams->spectral_fft_period = sp_out.ss_fft_period;
		spectralparams->spectral_period = sp_out.ss_period;
		spectralparams->spectral_count = sp_out.ss_count;
		spectralparams->spectral_short_report = sp_out.ss_short_report;
#ifdef SPECTRAL_DEBUG
                printNoiseFloor(sc->sc_ah);
                print_phy_err_stats(sc);
                print_detection(sc);
#endif
                SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL,"this_scan_recv=%d total_recv=%d\n", spectral->num_spectral_data, spectral->total_spectral_data);
                break;
	case SPECTRAL_IS_ACTIVE:
		if (!outdata || !outsize || *outsize < sizeof(u_int32_t)) {
			error = -EINVAL;
			break;
		}
		*outsize = sizeof(u_int32_t);
		*((u_int32_t *)outdata) = (u_int32_t)(ath_hal_is_spectral_active(sc->sc_ah));
		break;
	case SPECTRAL_IS_ENABLED:
		if (!outdata || !outsize || *outsize < sizeof(u_int32_t)) {
			error = -EINVAL;
			break;
		}
		*outsize = sizeof(u_int32_t);
		*((u_int32_t *)outdata) = (u_int32_t)(ath_hal_is_spectral_enabled(sc->sc_ah));
		break;

    case SPECTRAL_SET_DEBUG_LEVEL:
		if (insize < sizeof(u_int32_t) || !indata) {
                        error = -EINVAL;
			break;
		}
		temp_debug = *(u_int32_t *)indata;
        spectral_debug_level = (ATH_DEBUG_SPECTRAL << temp_debug);
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL,"%s debug level now = 0x%x \n", __func__, spectral_debug_level);
        break;

    case SPECTRAL_ACTIVATE_SCAN:
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL,"%s - start scan command total_recv = %d\n", __func__, spectral->total_spectral_data);
        spectral->cached_phy_err = sc->sc_phy_stats[sc->sc_curmode].ast_rx_phyerr;
        spectral->scan_start_tstamp = ath_hal_gettsf64(sc->sc_ah);
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL,"Start scan at %llu\n", (long long unsigned int) spectral->scan_start_tstamp); 
        SPECTRAL_LOCK(spectral);          
        start_spectral_scan(sc);
        sc->sc_spectral_scan = 1;
        SPECTRAL_UNLOCK(spectral);          
        break;

    case SPECTRAL_CLASSIFY_SCAN:
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL,"%s - start classify scan command\n", __func__);
        spectral->scan_start_tstamp = ath_hal_gettsf64(sc->sc_ah);
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"Start classify scan at %llu\n", (long long unsigned int) spectral->scan_start_tstamp); 
        start_classify_scan(sc);
        break;
    case SPECTRAL_STOP_SCAN:
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL,"%s - stop scan command scan_recv=%d total_recv=%d\n", __func__, spectral->num_spectral_data, spectral->total_spectral_data);
        SPECTRAL_LOCK(spectral);       
        stop_current_scan(sc);
        sc->sc_spectral_scan = 0;
        SPECTRAL_UNLOCK(spectral);       
        break;
    case SPECTRAL_ACTIVATE_FULL_SCAN:
        SPECTRAL_LOCK(spectral);       
        start_spectral_scan(sc);
        sc->sc_spectral_full_scan = 1;
        SPECTRAL_UNLOCK(spectral);       
        break;

    case SPECTRAL_STOP_FULL_SCAN:
        SPECTRAL_LOCK(spectral);       
        stop_current_scan(sc);
        sc->sc_spectral_full_scan = 0;
        SPECTRAL_UNLOCK(spectral);       
        break;
	default:
		error = -EINVAL;
        break;
	}
bad:
	return error;
}

int
spectral_set_thresholds(struct ath_softc *sc, const u_int32_t threshtype,
		   const u_int32_t value)
{
	struct ath_spectral *spectral=sc->sc_spectral;
	HAL_SPECTRAL_PARAM sp;

	if (spectral == NULL) {
		SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1, "%s: sc_spectral is NULL\n",
			__func__);
		return 0;
	}

	sp.ss_count = HAL_PHYERR_PARAM_NOVAL;
	sp.ss_short_report = AH_TRUE;
	sp.ss_period = HAL_PHYERR_PARAM_NOVAL;
	sp.ss_fft_period = HAL_PHYERR_PARAM_NOVAL;

	switch (threshtype) {
	case SPECTRAL_PARAM_FFT_PERIOD:
		sp.ss_fft_period = value;
		SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2, "%s: fft_period=%d\n",__func__, value);
		break;
	case SPECTRAL_PARAM_SCAN_PERIOD:
		sp.ss_period = value;
		SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2, "%s: scan_period=%d\n",__func__, value);
		break;
	case SPECTRAL_PARAM_SCAN_COUNT:
		sp.ss_count = value;
		SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2, "%s: scan_count=%d\n",__func__, value);
		break;
	case SPECTRAL_PARAM_SHORT_REPORT:
                if (value)
        		sp.ss_short_report = AH_TRUE;
                else
        		sp.ss_short_report = AH_FALSE;
		SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2, "%s: short_report=%d\n",__func__, sp.ss_short_report);
		break;
	}
	ath_hal_configure_spectral(sc->sc_ah, &sp);
        spectral_get_thresholds(sc, &sc->sc_spectral->params);
	return 1;
}
int
spectral_get_thresholds(struct ath_softc *sc, HAL_SPECTRAL_PARAM *param)
{
	ath_hal_get_spectral_config(sc->sc_ah, param);
	return 1;
}

void spectral_send_eacs_msg(struct ath_softc *sc)
{
#ifndef WIN32
    SPECTRAL_SAMP_MSG *msg = NULL;
    struct ath_spectral *spectral = sc->sc_spectral;
    spectral_prep_skb(sc);
    if (spectral->spectral_skb != NULL) {
         spectral->spectral_nlh = (struct nlmsghdr*)spectral->spectral_skb->data;
         msg = (SPECTRAL_SAMP_MSG*) NLMSG_DATA(spectral->spectral_nlh);
         msg->cw_interferer = 1;
         print_samp_msg (msg, sc);
         SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"nlmsg_len=%d skb->len=%d\n", spectral->spectral_nlh->nlmsg_len, spectral->spectral_skb->len);
         SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"%s", ".");
         spectral_bcast_msg(sc);
    }
#endif

}
#define MIN_SPECTRAL_DATA_SIZE (10)
int 
is_spectral_phyerr(struct ath_softc *sc, 
                      struct ath_buf *bf, 
                      struct ath_rx_status *rxs) 
{

    u_int8_t pulse_bw_info = 0;
    u_int8_t *byte_ptr;
    u_int8_t last_byte_0;
    u_int8_t last_byte_1;
    u_int8_t last_byte_2;
    u_int8_t last_byte_3;
    u_int8_t secondlast_byte_0;
    u_int8_t secondlast_byte_1;
    u_int8_t secondlast_byte_2;
    u_int8_t secondlast_byte_3;
    u_int8_t *dataPtr;
    u_int16_t datalen;


    u_int32_t *last_word_ptr;
    u_int32_t *secondlast_word_ptr;


    if ((!(rxs->rs_phyerr == HAL_PHYERR_RADAR)) &&
           (!(rxs->rs_phyerr == HAL_PHYERR_FALSE_RADAR_EXT)) && 
           (!(rxs->rs_phyerr == HAL_PHYERR_SPECTRAL))) {
        return AH_FALSE;
    }

    if (!rxs->rs_datalen) {
        return AH_FALSE;
    }

    if (!sc->sc_spectral) {
        return AH_FALSE;
    }

    // If spectral scan is not started, just drop this packet.
    if (!(sc->sc_spectral_scan || sc->sc_spectral_full_scan)) {
        return AH_FALSE;
    }

    //FIXME. When datalen is less than 2 words, following process will underrun.
    if ( rxs->rs_datalen < MIN_SPECTRAL_DATA_SIZE ) {
        return AH_FALSE;
    }

    datalen = rxs->rs_datalen;
    dataPtr = bf->bf_vdata;
    last_word_ptr = (u_int32_t *)(dataPtr + datalen - (datalen % 4));
    secondlast_word_ptr = last_word_ptr - 1;

    byte_ptr = (u_int8_t *)last_word_ptr;
    last_byte_0 = (*(byte_ptr) & 0xff);
    last_byte_1 = (*(byte_ptr + 1) & 0xff);
    last_byte_2 = (*(byte_ptr + 2) & 0xff);
    last_byte_3 = (*(byte_ptr + 3) & 0xff);

    byte_ptr = (u_int8_t*)secondlast_word_ptr;
    secondlast_byte_0 = (*(byte_ptr) & 0xff);
    secondlast_byte_1 = (*(byte_ptr + 1) & 0xff);
    secondlast_byte_2 = (*(byte_ptr + 2) & 0xff);
    secondlast_byte_3 = (*(byte_ptr + 3) & 0xff);

    /* extract the bwinfo */
    switch ((datalen & 0x3))  {
        case 0:
            pulse_bw_info = secondlast_byte_3;
            break;
        case 1:
            pulse_bw_info = last_byte_0;
            break;
        case 2:
            pulse_bw_info = last_byte_1;
            break;
        case 3:
            pulse_bw_info = last_byte_2;
            break;
    }

    /* the 5lsbs contain information regarding the source of data */
    pulse_bw_info &= 0x10; //SPECTRAL_SCAN_BITMASK;
    return (pulse_bw_info ? AH_TRUE:AH_FALSE);
}



#ifndef __NetBSD__
#ifdef __linux__
/*
 * Linux Module glue.
 */
static char *dev_info = "ath_spectral";

MODULE_AUTHOR("Atheros Communications, Inc.");
MODULE_DESCRIPTION("SPECTRAL Support for Atheros 802.11 wireless LAN cards.");
MODULE_SUPPORTED_DEVICE("Atheros WLAN cards");
#ifdef MODULE_LICENSE
MODULE_LICENSE("Proprietary");
#endif

static int __init
init_ath_spectral(void)
{
	printk (KERN_INFO "%s: Version 2.0.0\n"
		"Copyright (c) 2005-2009 Atheros Communications, Inc. "
		"All Rights Reserved\n",dev_info);
        printk(KERN_INFO "SPECTRAL module built on %s %s\n", __DATE__, __TIME__);
	return 0;
}
module_init(init_ath_spectral);

static void __exit
exit_ath_spectral(void)
{
	printk (KERN_INFO "%s: driver unloaded\n", dev_info);
}
module_exit(exit_ath_spectral);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,52))
MODULE_PARM(nfrssi, "i");
MODULE_PARM(nobeacons, "i");
MODULE_PARM(maxholdintvl, "i");
MODULE_PARM_DESC(nfrssi, "Adjust for noise floor or not");
MODULE_PARM_DESC(nobeacons, "Should the AP beacon during spectral scan?");
MODULE_PARM_DESC(maxholdintvl, "Should max hold be used and what is the max hold interval?");
#else
#include <linux/moduleparam.h>
module_param(nfrssi, int, 0600);
module_param(nobeacons, int, 0600);
module_param(maxholdintvl, int, 0600);
#endif

#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

EXPORT_SYMBOL(spectral_attach);
EXPORT_SYMBOL(spectral_detach);
EXPORT_SYMBOL(spectral_scan_enable);
EXPORT_SYMBOL(ath_process_spectraldata);
EXPORT_SYMBOL(spectral_control);
EXPORT_SYMBOL(spectral_set_thresholds);
EXPORT_SYMBOL(is_spectral_phyerr);
EXPORT_SYMBOL(start_spectral_scan);
EXPORT_SYMBOL(stop_current_scan);
EXPORT_SYMBOL(get_eacs_extension_duty_cycle);
EXPORT_SYMBOL(get_eacs_control_duty_cycle);
EXPORT_SYMBOL(spectral_send_eacs_msg);

#endif /* __linux__ */
#endif /* __netbsd__ */

#undef SPECTRAL_DPRINTK 
#undef SPECTRAL_MIN
#undef SPECTRAL_MAX
#undef SPECTRAL_DIFF

#endif
