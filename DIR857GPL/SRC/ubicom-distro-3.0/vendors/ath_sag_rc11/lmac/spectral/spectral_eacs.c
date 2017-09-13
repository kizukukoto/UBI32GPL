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

/* Get noise floor compensated control channel RSSI */
int8_t get_nfc_ctl_rssi(struct ath_hal *ah, int8_t rssi, int8_t *ctl_nf)
{
    int16_t nf = -110;
    int8_t temp;
    *ctl_nf = -110;

    nf = ath_hal_get_ctl_nf(ah);
    SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL3,"NF calibrated [ctl] [chain 0] is %d\n", nf);
    temp = 110 + (nf) + (rssi);
    SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"CTL NFC nf=%d temp=%d rssi = %d\n", nf, temp, rssi);
    *ctl_nf = nf;
    return temp;
}

/* Get noise floor compensated extension channel RSSI */
int8_t get_nfc_ext_rssi(struct ath_hal *ah, int8_t rssi, int8_t *ext_nf)
{
    int16_t nf = -110;
    int8_t temp;
    *ext_nf = -110;
    nf = ath_hal_get_ext_nf(ah);
    SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL3,"NF calibrated [ext] [chain 0] is %d\n", nf);
    temp = 110 + (nf) + (rssi);
    SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"EXT NFC nf=%d temp=%d rssi = %d\n", nf, temp, rssi);
    *ext_nf = nf;
    return temp;
}

void update_eacs_avg_rssi(struct ath_softc *sc, int8_t nfc_ctl_rssi, int8_t nfc_ext_rssi)
{
    struct ath_spectral *spectral=sc->sc_spectral;
    int temp=0;

    if(spectral->sc_spectral_20_40_mode) {
            // HT40 mode
            if ((nfc_ext_rssi - spectral->ext_eacs_avg_rssi) > 10) {
                SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"nfc_ext_rssi=%d ext_avg_rssi=%d \n", nfc_ext_rssi ,spectral->ext_eacs_avg_rssi);
            }

            temp = (spectral->ext_eacs_avg_rssi * (spectral->ext_eacs_spectral_reports));
            temp += nfc_ext_rssi;
            spectral->ext_eacs_spectral_reports++;
            spectral->ext_eacs_avg_rssi = (temp / spectral->ext_eacs_spectral_reports);
        }

            if((nfc_ctl_rssi - spectral->ctl_eacs_avg_rssi) > 10){
                SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"nfc_ctl_rssi=%d ctl_avg_rssi=%d \n", nfc_ctl_rssi ,spectral->ctl_eacs_avg_rssi);
            }

            temp = (spectral->ctl_eacs_avg_rssi * (spectral->ctl_eacs_spectral_reports));
            temp += nfc_ctl_rssi;
            spectral->ctl_eacs_spectral_reports++;
            spectral->ctl_eacs_avg_rssi = (temp / spectral->ctl_eacs_spectral_reports);
}

void update_eacs_counters(struct ath_softc *sc, int8_t nfc_ctl_rssi, int8_t nfc_ext_rssi)
{
	struct ath_spectral *spectral=sc->sc_spectral;

        update_eacs_thresholds(sc);
        update_eacs_avg_rssi(sc, nfc_ctl_rssi, nfc_ext_rssi);

        if (spectral->sc_spectral_20_40_mode) {
            // HT40 mode
            if (nfc_ext_rssi > spectral->ext_eacs_rssi_thresh){
                spectral->ext_eacs_interf_count++;    
            }
            spectral->ext_eacs_duty_cycle=((spectral->ext_eacs_interf_count * 100)/spectral->eacs_this_scan_spectral_data);
        }

        if (nfc_ctl_rssi > spectral->ctl_eacs_rssi_thresh){
            spectral->ctl_eacs_interf_count++;    
        }
        spectral->ctl_eacs_duty_cycle=((spectral->ctl_eacs_interf_count * 100)/spectral->eacs_this_scan_spectral_data);
        SPECTRAL_DPRINTK(sc,ATH_DEBUG_SPECTRAL1, "ctl_interf_count=%d ext_interf_count=%d this_scan_spectral_data=%d\n",
            spectral->ctl_eacs_interf_count, 
            spectral->ext_eacs_interf_count, 
            spectral->eacs_this_scan_spectral_data);
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"duty cycle ctl=%d ext=%d nfc_ctl_rssi=%d ctl_eacs_rssi_thresh=%d nfc_ext_rssi=%d ctl_eacs_rss_thresh=%d \n", spectral->ctl_eacs_duty_cycle, spectral->ext_eacs_duty_cycle, nfc_ctl_rssi, spectral->ctl_eacs_rssi_thresh, nfc_ext_rssi ,spectral->ext_eacs_rssi_thresh);
}

int get_eacs_control_duty_cycle(struct ath_softc *sc)
{
	struct ath_spectral *spectral=sc->sc_spectral;
        if (spectral == NULL) {
		SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1, "%s: sc_spectral is NULL, HW may not be spectral capable\n",
			__func__);
		return 0;
        }
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL,"%s control chan = %d extension chan = %d\n", __func__, sc->sc_curchan.channel, sc->sc_extchan.channel); 
        SPECTRAL_DPRINTK(sc,ATH_DEBUG_SPECTRAL, "ctl_interf_count=%d ext_interf_count=%d this_scan_spectral_data=%d\n",
            spectral->ctl_eacs_interf_count, 
            spectral->ext_eacs_interf_count, 
            spectral->eacs_this_scan_spectral_data);
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL,"%s duty cycle ctl=%d ext=%d\n", __func__, spectral->ctl_eacs_duty_cycle, spectral->ext_eacs_duty_cycle);
        return spectral->ctl_eacs_duty_cycle;
}

int get_eacs_extension_duty_cycle(struct ath_softc *sc)
{
	struct ath_spectral *spectral=sc->sc_spectral;
        if (spectral == NULL) {
		SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1, "%s: sc_spectral is NULL, HW may not be spectral capable\n",
			__func__);
		return 0;
        }
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL,"%s control chan = %d extension chan = %d\n", __func__, sc->sc_curchan.channel, sc->sc_extchan.channel); 
        SPECTRAL_DPRINTK(sc,ATH_DEBUG_SPECTRAL1, "ctl_interf_count=%d ext_interf_count=%d this_scan_spectral_data=%d\n",
            spectral->ctl_eacs_interf_count, 
            spectral->ext_eacs_interf_count, 
            spectral->eacs_this_scan_spectral_data);
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL,"%s duty cycle ctl=%d ext=%d\n", __func__, spectral->ctl_eacs_duty_cycle, spectral->ext_eacs_duty_cycle);
        return spectral->ext_eacs_duty_cycle;
}

void update_eacs_thresholds(struct ath_softc *sc)
{
    struct ath_spectral *spectral=sc->sc_spectral;
    struct ath_hal *ah=sc->sc_ah;
    int16_t nf = -110;

    nf = ath_hal_get_ctl_nf(ah);
    SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"NF calibrated [ctl] [chain 0] is %d\n", nf);

    spectral->ctl_eacs_rssi_thresh = nf + 10;

    if (spectral->sc_spectral_20_40_mode) {
            nf = ath_hal_get_ext_nf(ah);
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"NF calibrated [ext] [chain 0] is %d\n", nf);
        spectral->ext_eacs_rssi_thresh = nf + 10;
    }
    spectral->ctl_eacs_rssi_thresh = 32;
    spectral->ext_eacs_rssi_thresh = 32;
}
#endif
