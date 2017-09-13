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
extern int nobeacons, nfrssi, maxholdintvl;

void ret_bin_thresh_sel(struct ath_softc *sc, u_int8_t *numerator, u_int8_t* denominator)
{
    struct ath_spectral *spectral=sc->sc_spectral;
    *numerator=0;
    *denominator=0;

    spectral_get_thresholds(sc, &(spectral->params));

    *numerator = (spectral->params.radar_bin_thresh_sel + 1); 
    *denominator=8;

    switch(spectral->params.radar_bin_thresh_sel) {

        case 0:*numerator=1;*denominator=8;break;
        case 1:*numerator=2;*denominator=8;break;
        case 2:*numerator=3;*denominator=8;break;
        case 3:*numerator=4;*denominator=8;break;
        case 4:*numerator=5;*denominator=8;break;
        case 5:*numerator=6;*denominator=8;break;
        case 6:*numerator=7;*denominator=8;break;
        default:break;
    }
    
}

u_int8_t return_max_value(u_int8_t* datap, u_int8_t numdata, u_int8_t *max_index, struct ath_softc *sc, u_int8_t *strong_bins)
{
    int i=0, temp=0, strong_bin_count=0, one_over_max_to_large_ratio=4; 
    u_int8_t maxval=*datap;
    u_int8_t numerator, denominator;
    int8_t real_maxindex=-28;

    *max_index = 0;
    *strong_bins = 0;

    SPECTRAL_DPRINTK(sc,ATH_DEBUG_SPECTRAL3, "datap=%p *datap=0x%x \n",datap, *datap);

    for (i=0; i< numdata;  i++) {
        if(maxval < *(datap + i)) {
            *max_index = i;
             maxval = *(datap + i);
             SPECTRAL_DPRINTK(sc,ATH_DEBUG_SPECTRAL3, "max_mag=0x%x max_index=%d\n",maxval, *max_index);
        }
    }

    real_maxindex += *max_index;
    SPECTRAL_DPRINTK(sc,ATH_DEBUG_SPECTRAL2, "real_maxindex=%d fix_mag_inv=%d\n",real_maxindex, fix_maxindex_inv_only(real_maxindex));
    ret_bin_thresh_sel(sc, &numerator, &denominator);
    one_over_max_to_large_ratio = (int) (denominator / numerator);

    for (i=0; i< numdata;  i++) {
        temp = *(datap + i);
        if ((temp * one_over_max_to_large_ratio) > maxval) {
            strong_bin_count++;
        }
    }
    *strong_bins = strong_bin_count;
    return maxval;
}


void process_mag_data(MAX_MAG_INDEX_DATA *imag, u_int16_t *mmag, u_int8_t *bmap_wt, u_int8_t *max_index)
{
    u_int16_t max_mag=0;
    u_int16_t temp=0;
    u_int8_t temp_maxindex=0, temp_bmap=0, temp_maxmag=0;

#ifdef OLD_MAGDATA_DEF
    *bmap_wt= imag->bmap_wt;  
    *max_index = imag->max_index_bits05;

    temp_maxindex = imag->max_index_bits05;
    temp_maxindex = (temp_maxindex >> 2);

    max_mag = imag->max_mag_bits01;  
    temp = imag->max_mag_bits29;
    max_mag = ((temp << 2) |  max_mag);

    temp = imag->max_mag_bits1110;
    max_mag = ((temp << 10) | max_mag);

#else

    //Get last 6 bits for bitmap_weight
    temp_bmap=(imag->all_bins1 & 0x3F);

    //Get first 2 bits for max_mag
    temp_maxmag=(imag->all_bins1 & 0xC0);
    temp_maxmag=(temp_maxmag >> 6);


    temp = imag->max_mag_bits29;
    max_mag = ((temp << 2) | temp_maxmag);


    temp_maxmag=(imag->all_bins2 & 0x03);
    temp = (temp_maxmag << 10);
    max_mag = temp | max_mag;

    //Get first 6 bits for bitmap_weight
    temp_maxindex=(imag->all_bins2 & 0xFC);
    temp_maxindex=(temp_maxindex >> 2);


    *bmap_wt= temp_bmap;  
    *max_index = temp_maxindex;

#endif
    *mmag = max_mag;
}


u_int32_t
spectral_round(int32_t val)
{
	u_int32_t ival,rem;

	if (val < 0)
		return 0;
	ival = val/100;
	rem = val-(ival*100);
	if (rem <50)
		return ival;
	else
		return(ival+1);
}
static inline u_int8_t
spectral_process_pulse_dur(struct ath_softc *sc, u_int8_t re_dur) 
{
	if (re_dur == 0) {
		return 1;
	} else {
		/* Convert 0.8us durations to TSF ticks (usecs) */
		return (u_int8_t)spectral_round((int32_t)(80*re_dur));
	}
}

int8_t fix_rssi_inv_only (struct ath_hal *ah, u_int8_t rssi_val)
{
    int8_t temp = rssi_val;
    int8_t inv  = 0;
    if (rssi_val > 127) {
        temp = rssi_val & 0x7f;
        inv = (~(temp) + 1) & 0x7f;
        temp = 0 - inv;
    }
    SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL3," %s rssival = %d temp=%d\n", __func__, rssi_val, temp);
    return temp;
}

int8_t adjust_rssi_with_nf_noconv_dbm (struct ath_hal *ah, int8_t rssi, int upper, int lower)
{
        return adjust_rssi_with_nf_dbm (ah, rssi, upper, lower, 0);
}
int8_t adjust_rssi_with_nf_conv_dbm (struct ath_hal *ah, int8_t rssi, int upper, int lower)
{
        return adjust_rssi_with_nf_dbm (ah, rssi, upper, lower, 1);
}
int8_t adjust_rssi_with_nf_dbm (struct ath_hal *ah, int8_t rssi, int upper, int lower, int convert_to_dbm)
{
    int16_t nf = -110;
    int8_t temp;
    if (upper) {
        nf = ath_hal_get_ctl_nf(ah);
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"NF calibrated [ctl] [chain 0] is %d\n", nf);
    }
    if (lower) {
        nf = ath_hal_get_ext_nf(ah);
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"NF calibrated [ext] [chain 0] is %d\n", nf);
    }
    temp = 110 + (nf) + (rssi);
    //if (temp < 0)
      //  temp *= -1;
    //Fri Oct 24 10:50:38 PDT 2008 - For classifier, no need to convert to dbm, so do not
    // subtract 93 from it
    if (convert_to_dbm)
        temp -= 96;
    SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"nf=%d temp=%d rssi = %d\n", nf, temp, rssi);
    return temp;
}

int8_t fix_rssi_for_classifier (struct ath_hal *ah, u_int8_t rssi_val, int upper, int lower)
{
    int8_t temp = rssi_val;
    int8_t inv  = 0;
    if (rssi_val > 127) {
        temp = rssi_val & 0x7f;
        inv = (~(temp) + 1) & 0x7f;
        temp = 0 - inv;
    }
    SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"%s rssival = %d temp=%d\n", __func__, rssi_val, temp);
    //Fri Oct 24 10:53:04 PDT 2008 - For classifier, convert_to_dbm=0
    temp = adjust_rssi_with_nf_noconv_dbm(ah, temp, upper, lower);
    return temp;
}

int8_t fix_rssi_inv_nf_dbm (struct ath_hal *ah, u_int8_t rssi_val, int upper, int lower)
{
    int8_t temp = rssi_val;
    int8_t inv  = 0;
    if (rssi_val > 127) {
        temp = rssi_val & 0x7f;
        inv = (~(temp) + 1) & 0x7f;
        temp = 0 - inv;
    }
    SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL3,"%s rssival = %d temp=%d\n", __func__, rssi_val, temp);
    //Fri Oct 24 10:53:04 PDT 2008 - To get a dbm value, pass in convert_to_dbm=1
    temp = adjust_rssi_with_nf_dbm(ah, temp, upper, lower, 1);
    return temp;
}

#ifdef WIN32
u_int8_t *ret_word_copied_fft_data(struct ath_softc *sc, struct ath_rx_status *rxs, struct ath_buf *bf)
{
#define ALLOC_FFT_DATA_SIZE 256

    struct ath_spectral *spectral=sc->sc_spectral;
    u_int32_t *word_ptr=NULL;
    u_int16_t datalen = rxs->rs_datalen;
    u_int8_t  *pfft_data=NULL;
    int j_max = (int)(datalen/4), i=0;

    if (datalen > spectral->spectral_data_len + 2) {
        return NULL;
    }
    pfft_data = (u_int8_t*)OS_MALLOC(sc->sc_osdev, ALLOC_FFT_DATA_SIZE, 0); 
    if (!pfft_data) {
        return NULL;
    }
        
    if ((datalen % 4) > 0) {
        j_max++;
    }

    OS_MEMZERO(pfft_data, ALLOC_FFT_DATA_SIZE);

    for (i = 0; i < j_max; i++) {
        word_ptr = (u_int32_t *)(((u_int8_t*)bf->bf_vdata + (4 * i)));
        pfft_data[i*4 + 0] = ((*word_ptr) >>  0) & 0xFF;
        pfft_data[i*4 + 1] = ((*word_ptr) >>  8) & 0xFF;
        pfft_data[i*4 + 2] = ((*word_ptr) >> 16) & 0xFF;
        pfft_data[i*4 + 3] = ((*word_ptr) >> 24) & 0xFF;
    }
    return pfft_data;
#undef ALLOC_FFT_DATA_SIZE
}
#endif

void
ath_process_spectraldata(struct ath_softc *sc, struct ath_buf *bf, struct ath_rx_status *rxs, u_int64_t fulltsf)
{
#define EXT_CH_RADAR_FOUND 0x02
#define PRI_CH_RADAR_FOUND 0x01
#define EXT_CH_RADAR_EARLY_FOUND 0x04
#define SPECTRAL_SCAN_DATA 0x10
        struct ath_spectral *spectral=sc->sc_spectral;
        struct samp_msg_params params;
	u_int8_t rssi, control_rssi=0, extension_rssi=0, combined_rssi=0;
        u_int8_t  max_exp=0, max_scale=0;
	u_int8_t dc_index, lower_dc=0, upper_dc=0;
	u_int8_t ext_rssi=0;
	int8_t inv_control_rssi=0, inv_combined_rssi=0, inv_extension_rssi=0, temp_nf;
        u_int8_t pulse_bw_info=0, pulse_length_ext=0, pulse_length_pri=0;
	u_int32_t tstamp=0;
        u_int16_t datalen, max_mag=0, newdatalen=0, already_copied=0;
        u_int16_t maxmag_upper=0, maxmag_lower;
        u_int8_t maxindex_upper, maxindex_lower;
        u_int8_t bin_pwr_data[130]={0}, max_index=0;
        int bin_pwr_count=0;
        u_int32_t *last_word_ptr, *secondlast_word_ptr;
        u_int8_t *byte_ptr, last_byte_0, last_byte_1, last_byte_2, last_byte_3, *fft_data_end_ptr; 
        u_int8_t secondlast_byte_0, secondlast_byte_1, secondlast_byte_2, secondlast_byte_3; 
        HT20_FFT_PACKET fft_20;
        HT40_FFT_PACKET fft_40;
        u_int8_t *fft_data_ptr, *fft_src_ptr, *data_ptr;
        int bmap_lower=0, bmap_upper=0;

        int8_t cmp_rssi=-110;
        int8_t rssi_up=0, rssi_low=0;
        static u_int64_t last_tsf=0, send_tstamp=0;
        int8_t nfc_control_rssi=0, nfc_extension_rssi=0;

        int peak_freq=0,i, nb_lower=0, nb_upper=0;
#ifdef WIN32
        u_int8_t *word_fft_data_ptr=NULL;
#endif
	if (((rxs->rs_phyerr != HAL_PHYERR_RADAR)) &&
	    ((rxs->rs_phyerr != HAL_PHYERR_FALSE_RADAR_EXT)) &&
	    ((rxs->rs_phyerr != HAL_PHYERR_SPECTRAL))) {
		SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL3, "%s: rs_phyer=0x%x not a radar error\n",__func__, rxs->rs_phyerr);
        	return;
        }

	if (spectral == NULL) {
		SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL, "%s: sc_spectral is NULL\n",__func__);
		return;
	}

        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL3, "%s total_phy_errors=%d\n", __func__, spectral->ath_spectral_stats.total_phy_errors);
        spectral->ath_spectral_stats.total_phy_errors++;
        datalen = rxs->rs_datalen;
        tstamp = (rxs->rs_tstamp & SPECTRAL_TSMASK);
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL3, "%s datalen=%d\n", __func__, datalen);

        /* WAR: Never trust combined RSSI on radar pulses for <=
         * OWL2.0. For short pulses only the chain 0 rssi is present
         * and remaining descriptor data is all 0x80, for longer
         * pulses the descriptor is present, but the combined value is
         * inaccurate. This HW capability is queried in spectral_attach and stored in
         * the sc_spectral_combined_rssi_ok flag.*/

        if (sc->sc_spectral->sc_spectral_combined_rssi_ok) {
            rssi = (u_int8_t) rxs->rs_rssi;
        } else {
            rssi = (u_int8_t) rxs->rs_rssi_ctl0;
        }
        
        combined_rssi = rssi;
        control_rssi = (u_int8_t) rxs->rs_rssi_ctl0;
        extension_rssi = (u_int8_t) rxs->rs_rssi_ext0;


        control_rssi = (u_int8_t) rxs->rs_rssi_ctl0;
        extension_rssi = (u_int8_t) rxs->rs_rssi_ext0;
    
        ext_rssi = (u_int8_t) rxs->rs_rssi_ext0;

        /* If the combined RSSI is less than a particular threshold, this pulse is of no
         interest to the classifier, so discard it here itself */
        inv_combined_rssi = fix_rssi_inv_only(sc->sc_ah, combined_rssi);
        if (inv_combined_rssi < 5) {
            return;
        }

        last_word_ptr = (u_int32_t *)(((u_int8_t*)bf->bf_vdata) + datalen - (datalen%4));

        secondlast_word_ptr = last_word_ptr-1;

        byte_ptr = (u_int8_t*)last_word_ptr; 
        last_byte_0=(*(byte_ptr) & 0xff); 
        last_byte_1=(*(byte_ptr+1) & 0xff); 
        last_byte_2=(*(byte_ptr+2) & 0xff); 
        last_byte_3=(*(byte_ptr+3) & 0xff); 

        byte_ptr = (u_int8_t*)secondlast_word_ptr; 
        secondlast_byte_0=(*(byte_ptr) & 0xff); 
        secondlast_byte_1=(*(byte_ptr+1) & 0xff); 
        secondlast_byte_2=(*(byte_ptr+2) & 0xff); 
        secondlast_byte_3=(*(byte_ptr+3) & 0xff); 

        if (!datalen) {
            spectral->ath_spectral_stats.datalen_discards++;
            return;
        }

        if (datalen < spectral->spectral_data_len -1) {
            spectral->ath_spectral_stats.datalen_discards++;
            return;
        }
 
        switch((datalen & 0x3)) {
        case 0:
            pulse_bw_info = secondlast_byte_3;
            pulse_length_ext = secondlast_byte_2;
            pulse_length_pri = secondlast_byte_1;
            byte_ptr = (u_int8_t*)secondlast_word_ptr; 
            fft_data_end_ptr=(byte_ptr); 
            break;
        case 1:
            pulse_bw_info = last_byte_0;
            pulse_length_ext = secondlast_byte_3;
            pulse_length_pri = secondlast_byte_2;
            byte_ptr = (u_int8_t*)secondlast_word_ptr; 
            fft_data_end_ptr=(byte_ptr+2); 
            break;
        case 2:
            pulse_bw_info = last_byte_1;
            pulse_length_ext = last_byte_0;
            pulse_length_pri = secondlast_byte_3;
            byte_ptr = (u_int8_t*)secondlast_word_ptr; 
            fft_data_end_ptr=(byte_ptr+3); 
           break;
        case 3:
            pulse_bw_info = last_byte_2;
            pulse_length_ext = last_byte_1;
            pulse_length_pri = last_byte_0;
            byte_ptr = (u_int8_t*)last_word_ptr; 
            fft_data_end_ptr=(byte_ptr); 
            break;
        default:
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL, "datalen mod4=%d spectral_data_len=%d\n", (datalen%4), spectral->spectral_data_len);
        }

        /* Only the last 3 bits of the BW info are relevant, they indicate
        which channel the radar was detected in.*/
        pulse_bw_info &= 0x17;

         if (pulse_bw_info & SPECTRAL_SCAN_DATA) {

            if (datalen > spectral->spectral_data_len + 2) {
                    SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2, "INVALID SPECTRAL SCAN datalen=%d datalen_mod_4=%d\n", datalen, (datalen%4));
                    byte_ptr = (u_int8_t*)bf->bf_vdata;
                    SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2, 
                    "INVALID bwinfo=0x%x byte_ptr[last]=0x%x byte_ptr[last-1]=0x%x byte_ptr[last-2]=0x%x bytes_ptr[last-3]=0x%x\n", pulse_bw_info, 
                *(byte_ptr + datalen),
                *(byte_ptr + (datalen - 1)),
                *(byte_ptr + (datalen - 2)),
                *(byte_ptr + (datalen - 3)));
                    return;
            }

            if (spectral->num_spectral_data == 0) {
                    spectral->first_tstamp = tstamp;
                    spectral->max_rssi = -110;
                    SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"First FFT data tstamp = %u rssi=%d\n", tstamp, fix_rssi_inv_only(sc->sc_ah, combined_rssi));
            }

            spectral->num_spectral_data++;    
            OS_MEMZERO(&fft_40, sizeof (fft_40));
            OS_MEMZERO(&fft_20, sizeof (fft_20));

            if (spectral->sc_spectral_20_40_mode) {
                    fft_data_ptr = (u_int8_t*)&fft_40.lower_bins.bin_magnitude[0];
            } else {
                    fft_data_ptr = (u_int8_t*)&fft_20.lower_bins.bin_magnitude[0];
            }

#ifdef WIN32
            word_fft_data_ptr=ret_word_copied_fft_data(sc, rxs, bf);
#endif
            byte_ptr = fft_data_ptr;
            if (datalen == spectral->spectral_data_len) { 
            
              if (spectral->sc_spectral_20_40_mode) {
                    // HT40 packet, correct length
#ifndef WIN32
                    OS_MEMCPY(&fft_40, (u_int8_t*)(bf->bf_vdata), datalen);
#else
                    OS_MEMCPY(&fft_40, word_fft_data_ptr, datalen);
#endif
                } else {
                    // HT20 packet, correct length
#ifndef WIN32
                    OS_MEMCPY(&fft_20, (u_int8_t*)(bf->bf_vdata), datalen);
#else
                    OS_MEMCPY(&fft_20, word_fft_data_ptr, datalen);
#endif
                }

            }//endif (datalen == spectral->spectral_data_len)

            /* This happens when there is a missing byte because CCK is enabled. This may happen with or without the second bug of the MAC inserting 2 bytes.*/
            if ((datalen == (spectral->spectral_data_len - 1)) || 
               (datalen == (spectral->spectral_data_len + 1))) {

               SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2, "%s %d missing 1 byte datalen=%d expected=%d\n", __func__, __LINE__, datalen, spectral->spectral_data_len);

              already_copied ++;

              if (spectral->sc_spectral_20_40_mode) {
                    // HT40 packet, missing 1 byte
#ifndef WIN32
                    // Use the beginning byte as byte 0 and byte 1 
                    fft_40.lower_bins.bin_magnitude[0]=*(u_int8_t*)(bf->bf_vdata);
                    // Now copy over the rest
                    fft_data_ptr = (u_int8_t*)&fft_40.lower_bins.bin_magnitude[1];
                    OS_MEMCPY(fft_data_ptr, (u_int8_t*)(bf->bf_vdata), datalen);
#else
                    // Use the beginning byte as byte 0 and byte 1 
                    fft_40.lower_bins.bin_magnitude[0]=*word_fft_data_ptr;
                    // Now copy over the rest
                    fft_data_ptr = (u_int8_t*)&fft_40.lower_bins.bin_magnitude[1];
                    OS_MEMCPY(fft_data_ptr, word_fft_data_ptr, datalen);
#endif
                } else {
                    // HT20 packet, missing 1 byte
#ifndef WIN32
                    // Use the beginning byte as byte 0 and byte 1 
                    fft_20.lower_bins.bin_magnitude[0]=*(u_int8_t*)(bf->bf_vdata);
                    // Now copy over the rest
                    fft_data_ptr = (u_int8_t*)&fft_20.lower_bins.bin_magnitude[1];
                    OS_MEMCPY(fft_data_ptr, (u_int8_t*)(bf->bf_vdata), datalen);
#else
                    // Use the beginning byte as byte 0 and byte 1 
                    fft_20.lower_bins.bin_magnitude[0]=*(word_fft_data_ptr);
                    // Now copy over the rest
                    fft_data_ptr = (u_int8_t*)&fft_20.lower_bins.bin_magnitude[1];
                    OS_MEMCPY(fft_data_ptr, word_fft_data_ptr, datalen);
#endif
                }
            }

            if ((datalen == (spectral->spectral_data_len + 1)) || 
               (datalen == (spectral->spectral_data_len + 2))) {

               SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2, "%s %d extra bytes datalen=%d expected=%d\n", __func__, __LINE__, datalen, spectral->spectral_data_len);

              if (spectral->sc_spectral_20_40_mode) {// HT40 packet, MAC added 2 extra bytes
#ifndef WIN32
                    fft_src_ptr = (u_int8_t*)(bf->bf_vdata);
#else
                    fft_src_ptr = word_fft_data_ptr;
#endif

                    fft_data_ptr = (u_int8_t*)&fft_40.lower_bins.bin_magnitude[already_copied];

                    for(i=0, newdatalen=0; newdatalen < (SPECTRAL_HT40_DATA_LEN - already_copied); i++) {
                        if (i == 30 || i == 32) {
                            continue;
                        }

                        *(fft_data_ptr+newdatalen)=*(fft_src_ptr+i);
                        newdatalen++;
                    }
                } else { //HT20 packet, MAC added 2 extra bytes
#ifndef WIN32
                    fft_src_ptr = (u_int8_t*)(bf->bf_vdata);
#else
                    fft_src_ptr = word_fft_data_ptr;
#endif
                    fft_data_ptr = (u_int8_t*)&fft_20.lower_bins.bin_magnitude[already_copied];

                    for(i=0, newdatalen=0; newdatalen < (SPECTRAL_HT20_DATA_LEN - already_copied); i++) {
                        if (i == 30 || i == 32)
                            continue;

                        *(fft_data_ptr+newdatalen)=*(fft_src_ptr+i);
                        newdatalen++;
                    }
                }
            }

#ifdef WIN32
              OS_FREE(word_fft_data_ptr);
              word_fft_data_ptr=NULL;
#endif
              spectral->total_spectral_data++;

              dc_index=spectral->spectral_dc_index;

              if (spectral->sc_spectral_20_40_mode) {
                    max_exp = (fft_40.max_exp & 0x07);
                    byte_ptr = (u_int8_t*)&fft_40.lower_bins.bin_magnitude[0];

                    /* Correct the DC bin value */
                    lower_dc = *(byte_ptr+dc_index-1);
                    upper_dc = *(byte_ptr+dc_index+1);
                    *(byte_ptr+dc_index)=((upper_dc + lower_dc)/2);

                } else {
                    max_exp = (fft_20.max_exp & 0x07);
                    byte_ptr = (u_int8_t*)&fft_20.lower_bins.bin_magnitude[0];

                    /* Correct the DC bin value */
                    lower_dc = *(byte_ptr+dc_index-1);
                    upper_dc = *(byte_ptr+dc_index+1);
                    *(byte_ptr+dc_index)=((upper_dc + lower_dc)/2);

                }

            if (sc->sc_ent_spectral_mask) {
                *(byte_ptr+dc_index) &=
                ~((1 << sc->sc_ent_spectral_mask) - 1);
            }

            if (max_exp < 1)
                max_scale = 1;
            else 
                max_scale = (2) << (max_exp - 1);

            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2, "max_scale=%d max_exp=%d \n", max_scale, max_exp);

            bin_pwr_count = spectral->spectral_numbins;

            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL3, "pwr_count=%d bin_pwr_data=%p byte_ptr=%p \n", bin_pwr_count, bin_pwr_data, byte_ptr);

            if ((last_tsf  > fulltsf) && (!spectral->classify_scan)) {
               SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL,"Reset last=0x%llu now=0x%llu\n", (long long unsigned int) last_tsf, (long long unsigned int) fulltsf);
               spectral->send_single_packet = 1;
               last_tsf = fulltsf;
            }

            inv_combined_rssi = fix_rssi_inv_only(sc->sc_ah, combined_rssi);
            inv_control_rssi = fix_rssi_inv_only(sc->sc_ah, control_rssi);
            inv_extension_rssi = fix_rssi_inv_only(sc->sc_ah, extension_rssi);

#ifdef INV_RSSI_ONLY
            {
                if (spectral->upper_is_control)
                    rssi_up = inv_control_rssi;
                else
                    rssi_up = inv_extension_rssi;

                if (spectral->lower_is_control)
                    rssi_low = inv_control_rssi;
                else
                    rssi_low = inv_extension_rssi;
            }
#else 
            { 
                if (spectral->upper_is_control)
                    rssi_up = control_rssi;
                else
                    rssi_up = extension_rssi;

                if (spectral->lower_is_control)
                    rssi_low = control_rssi;
                else
                    rssi_low = extension_rssi;

                nfc_control_rssi = get_nfc_ctl_rssi(sc->sc_ah, inv_control_rssi, &temp_nf);
                spectral->ctl_chan_noise_floor = temp_nf;
                spectral->eacs_this_scan_spectral_data++;
                rssi_low = fix_rssi_for_classifier(sc->sc_ah, rssi_low, spectral->lower_is_control, spectral->lower_is_extension);

                if (spectral->sc_spectral_20_40_mode) {
                    rssi_up = fix_rssi_for_classifier(sc->sc_ah, rssi_up,spectral->upper_is_control,spectral->upper_is_extension);
                    nfc_extension_rssi = get_nfc_ext_rssi(sc->sc_ah, inv_extension_rssi, &temp_nf);
                    spectral->ext_chan_noise_floor = temp_nf;
                }
            }
            update_eacs_counters(sc, nfc_control_rssi, nfc_extension_rssi);
#endif // else ifndef INV_RSSI_ONLY

            if (spectral->sc_spectral_20_40_mode) {
                print_fft_ht40_packet(&fft_40, sc);
                process_fft_ht40_packet(&fft_40, sc, &maxmag_lower, &maxindex_lower, &maxmag_upper, &maxindex_upper, &max_mag,  &max_index, &nb_lower, &nb_upper, &rssi_low, &rssi_up, &bmap_lower, &bmap_upper);
            } else {
                print_hex_fft_ht20_packet(&fft_20, sc);
                process_fft_ht20_packet(&fft_20, sc, &maxmag_lower, &maxindex_lower, &nb_lower, &rssi_low, &bmap_lower);
                rssi_up = 0;
                extension_rssi=0;
                inv_extension_rssi=0;
                nb_upper=0;
                maxindex_upper=0;
                maxmag_upper=0;
            }

            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2, "nb_lower=%d combined=%d lower=%d inv_combined=%d inv_control=%d rssi_low=%d\n",nb_lower, combined_rssi, control_rssi, inv_combined_rssi, inv_control_rssi, rssi_low); 
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1, "%d %d %d %d %d %d %d %d\n",tstamp, rssi_low, nb_lower, maxindex_lower, rssi_up, nb_upper, maxindex_upper, inv_combined_rssi); 

            params.sc=sc;
            params.rssi=inv_combined_rssi; 
            params.lower_rssi=rssi_low; 
            params.upper_rssi=rssi_up; 

            params.bwinfo=pulse_bw_info; 
            params.tstamp=tstamp;
            params.max_mag=max_mag; 
            params.max_index=max_index;
            params.max_exp=max_scale;
            params.peak=peak_freq; 
            params.pwr_count=bin_pwr_count; 
            params.bin_pwr_data=&byte_ptr; 
            params.freq=sc->sc_curchan.channel;
            params.freq_loading=(spectral->ctl_eacs_duty_cycle > spectral->ext_eacs_duty_cycle ? spectral->ctl_eacs_duty_cycle: spectral->ext_eacs_duty_cycle);
 
            params.interf_list.count=0; 
            
            params.max_lower_index = maxindex_lower;
            params.max_upper_index = maxindex_upper;
            params.nb_lower = nb_lower;
            params.nb_upper = nb_upper;
            params.last_tstamp = spectral->last_tstamp;

            OS_MEMCPY(&params.classifier_params, &spectral->classifier_params, sizeof(SPECTRAL_CLASSIFIER_PARAMS));

            if((params.nb_lower==1) || (params.nb_upper==1)) {
                SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2, " %s %d nb_lower=%d nb_upper=%d params.tstamp=0x%x, params.max_mag=%d params.max_index=%d, params.peak=%d\n",__func__, __LINE__,  params.nb_lower, params.nb_upper, params.tstamp, params.max_mag, params.max_index, params.peak); 
            }

            cmp_rssi = fix_rssi_inv_only (sc->sc_ah, combined_rssi);
            spectral->send_single_packet = 0;

#ifdef SPECTRAL_DEBUG_TIMER
            OS_CANCEL_TIMER(&spectral->debug_timer);
#endif
            spectral->spectral_sent_msg++;

            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL3,"\n%s %d datalen=%d\n",__func__, __LINE__, datalen ); 
            params.datalen = datalen;
            if (spectral->sc_spectral_20_40_mode) {
                data_ptr = (u_int8_t*)&fft_40.lower_bins.bin_magnitude;
            } else {
                data_ptr = (u_int8_t*)&fft_20.lower_bins.bin_magnitude;
            }

            byte_ptr = (u_int8_t*)data_ptr;
            params.bin_pwr_data = &byte_ptr;

            send_tstamp =  ath_hal_gettsf64(sc->sc_ah);

            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2, "Sending FFT data recvd at %llu desc tstamp = %u current tstamp=%u\n", (long long unsigned int)send_tstamp, params.tstamp, tstamp);
            spectral_create_samp_msg(&params);

#ifdef SPECTRAL_CLASSIFIER_IN_KERNEL
            if (sc->sc_spectral->classify_scan && maxindex_lower != (SPECTRAL_HT20_DC_INDEX + 1)) {
                   classifier(&spectral->bd_lower, tstamp, spectral->last_tstamp, rssi_low, nb_lower, maxindex_lower);
                if (spectral->sc_spectral_20_40_mode && maxindex_upper != (SPECTRAL_HT40_DC_INDEX)) {
                    classifier(&spectral->bd_upper, tstamp, spectral->last_tstamp, rssi_up, nb_upper, maxindex_upper);
            }

                print_detection(sc);
            }
#endif
            spectral->last_tstamp=tstamp;

            return;

        // end if (pulse_bw_info & SPECTRAL_SCAN_DATA) 
        } else {
      
    /* Add a HAL capability that tests if chip is capable of spectral scan. 
       Probably just a check if its a Merlin and above. */
	SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2, "NON SPECTRAL ERROR datalen=%d pulse_bw_info=0x%x pulse_length_ext=%u pulse_length_pri=%u rssi=%u ext_rssi=%u phyerr=0x%x\n", datalen, pulse_bw_info, pulse_length_ext, pulse_length_pri, rssi, ext_rssi, rxs->rs_phyerr);
            spectral->ath_spectral_stats.owl_phy_errors++;
    
        }

#undef EXT_CH_RADAR_FOUND
#undef PRI_CH_RADAR_FOUND
#undef EXT_CH_RADAR_EARLY_FOUND
#undef SPECTRAL_SCAN_DATA
}
#endif
