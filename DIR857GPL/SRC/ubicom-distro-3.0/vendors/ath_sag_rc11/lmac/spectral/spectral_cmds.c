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

#if ATH_SUPPORT_SPECTRAL
#include "spectral.h"
#include "ah.h"
#include "ath_internal.h"


void stop_current_scan(struct ath_softc *sc)
{
    struct ath_spectral *spectral = sc->sc_spectral;

    if (spectral == NULL) {
		SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1, "%s: sc_spectral is NULL, HW may not be spectral capable\n",
			__func__);
		return;
    }

    ath_hal_stop_spectral_scan(sc->sc_ah);
#ifdef SPECTRAL_CLASSIFIER_IN_KERNEL
    OS_CANCEL_TIMER(&spectral->classify_timer);
#endif
#ifdef SPECTRAL_DEBUG_TIMER
    OS_CANCEL_TIMER(&spectral->debug_timer);
#endif
    if( spectral->classify_scan) {
        printk("%s - control chan detects=%d extension channel detects=%d above_dc_detects=%d below_dc_detects=%d\n", __func__,spectral->detects_control_channel, spectral->detects_extension_channel, spectral->detects_above_dc, spectral->detects_below_dc);
        spectral->detects_control_channel=0;
        spectral->detects_extension_channel=0;
        spectral->detects_above_dc=0;
        spectral->detects_below_dc=0;
        spectral->classify_scan = 0;
        print_detection(sc);
    }
    
    spectral->this_scan_phy_err = sc->sc_phy_stats[sc->sc_curmode].ast_rx_phyerr - spectral->cached_phy_err;
    spectral->send_single_packet = 0;

    spectral->ctl_eacs_avg_rssi=SPECTRAL_EACS_RSSI_THRESH;
    spectral->ext_eacs_avg_rssi=SPECTRAL_EACS_RSSI_THRESH;

    spectral->ctl_eacs_spectral_reports=0;
    spectral->ext_eacs_spectral_reports=0;
}
void start_spectral_scan(struct ath_softc *sc)
{
    HAL_SPECTRAL_PARAM spectral_params;
    struct ath_spectral *spectral = sc->sc_spectral;

    if (spectral == NULL) {
		SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1, "%s: sc_spectral is NULL, HW may not be spectral capable\n",
			__func__);
		return;
    }
    OS_MEMZERO(&spectral_params, sizeof (HAL_SPECTRAL_PARAM));

    spectral_params.ss_count = 128;
    spectral_params.ss_short_report = sc->sc_spectral->params.ss_short_report;
    spectral_params.ss_period = sc->sc_spectral->params.ss_period;
    spectral_params.ss_fft_period = sc->sc_spectral->params.ss_period;

    ath_hal_configure_spectral(sc->sc_ah, &spectral_params);
    spectral_get_thresholds(sc, &sc->sc_spectral->params);

    spectral->this_scan_phy_err = 0;
    spectral->send_single_packet = 1;
    spectral->spectral_sent_msg = 0;
    spectral->classify_scan = 0;
    spectral->num_spectral_data=0;
    spectral->eacs_this_scan_spectral_data=0;

    ath_hal_start_spectral_scan(sc->sc_ah);
}

void start_classify_scan(struct ath_softc *sc)
{
    HAL_SPECTRAL_PARAM spectral_params;
    struct ath_spectral *spectral = sc->sc_spectral;

    if (spectral == NULL) {
		SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1, "%s: sc_spectral is NULL, HW may not be spectral capable\n",
			__func__);
		return;
    }

    SPECTRAL_LOCK(spectral);          
    OS_MEMZERO(&spectral_params, sizeof (HAL_SPECTRAL_PARAM));
    spectral_params.ss_count = 128;
    spectral_params.ss_short_report = sc->sc_spectral->params.ss_short_report;
    spectral_params.ss_period = sc->sc_spectral->params.ss_period;
    spectral_params.ss_fft_period = sc->sc_spectral->params.ss_period;
    ath_hal_configure_spectral(sc->sc_ah, &spectral_params);
    spectral_get_thresholds(sc, &sc->sc_spectral->params);

    spectral->send_single_packet = 0;
    spectral->spectral_sent_msg = 0;
    spectral->classify_scan = 1;
    spectral->detects_control_channel=0;
    spectral->detects_extension_channel=0;
#ifdef SPECTRAL_CLASSIFIER_IN_KERNEL
    init_classifier(sc);
    classifier_initialize(&spectral->bd_lower, &spectral->bd_upper);
#endif
    spectral->this_scan_phy_err = 0;
    spectral->num_spectral_data=0;
    spectral->eacs_this_scan_spectral_data=0;

    SPECTRAL_UNLOCK(spectral);          
    ath_hal_start_spectral_scan(sc->sc_ah);
}
#endif
