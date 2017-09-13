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


void process_fft_ht40_packet(HT40_FFT_PACKET *fft_40, struct ath_softc *sc, u_int16_t *max_mag_lower, u_int8_t* max_index_lower, u_int16_t *max_mag_upper, u_int8_t* max_index_upper, u_int16_t *max_mag, u_int8_t* max_index, int *narrowband_lower, int *narrowband_upper, int8_t *rssi_lower, int8_t *rssi_upper, int *bmap_lower, int *bmap_upper)
{
    HT40_BIN_MAG_DATA *bmag=NULL;
    MAX_MAG_INDEX_DATA *imag=NULL;
    u_int8_t maxval=0, maxindex=0, *bindata=NULL, maxindex_lower, bmapwt_upper, maxindex_upper, bmapwt_lower;
    u_int16_t maxmag_lower, maxmag_upper;
    int small_bitmap_lower=0, small_bitmap_upper=0, high_rssi_upper=0, high_rssi_lower=0;
    int8_t minimum_rssi=5;
    u_int8_t  calc_maxindex=0, calc_strongbins=0, calc_maxval=0;

    int one_over_wideband_min_large_bin_ratio=7, one_side_length = SPECTRAL_HT40_NUM_BINS;

    bmag = &(fft_40->lower_bins);
    bindata = bmag->bin_magnitude;
    maxval = return_max_value(bindata, SPECTRAL_HT40_NUM_BINS, &maxindex, sc, &calc_strongbins);
    calc_maxval = return_max_value(bindata, SPECTRAL_HT40_NUM_BINS, &calc_maxindex, sc, &calc_strongbins);

    imag = &(fft_40->lower_bins_max);
    process_mag_data(imag, &maxmag_lower, &bmapwt_lower, &maxindex_lower);
    print_max_mag_index_data(imag, sc);

    bmag = &(fft_40->upper_bins);
    bindata = bmag->bin_magnitude;
    //maxval = return_max_value(bindata, SPECTRAL_HT40_NUM_BINS, &maxindex, sc);
    imag = &(fft_40->upper_bins_max);
    process_mag_data(imag, &maxmag_upper, &bmapwt_upper, &maxindex_upper);
    print_max_mag_index_data(imag, sc);

    *max_mag_upper = maxmag_upper; *max_index_upper = maxindex_upper;
    *max_mag_lower = maxmag_lower; *max_index_lower = maxindex_lower;

    small_bitmap_lower = (bmapwt_lower < 2);
    small_bitmap_upper = (bmapwt_upper < 2);

    *bmap_lower = bmapwt_lower;
    *bmap_upper = bmapwt_upper;

   high_rssi_lower = (*rssi_lower > minimum_rssi);
   high_rssi_upper = (*rssi_upper > minimum_rssi);

    if (maxmag_upper > maxmag_lower) {
        *rssi_lower = minimum_rssi - 1;
        high_rssi_lower = 0;
    } else {
        *rssi_upper = minimum_rssi - 1;
        high_rssi_upper = 0;
    } 

    *narrowband_lower=0;       
    *narrowband_upper=0;       

    if(high_rssi_lower) {
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"%d bmap_lower=%d bmap_upper=%d\n",__LINE__, bmapwt_lower, bmapwt_upper);
        if((bmapwt_lower * one_over_wideband_min_large_bin_ratio) < (one_side_length - 1)){
            *narrowband_lower=1;       
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"nb_lower=%d nb_upper=%d\n",*narrowband_lower, *narrowband_upper);
        }
    }
    if(high_rssi_upper) {
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"%d bmap_lower=%d bmap_upper=%d\n",__LINE__, bmapwt_lower, bmapwt_upper);
        if((bmapwt_upper * one_over_wideband_min_large_bin_ratio) < (one_side_length - 1)){
            *narrowband_upper=1;       
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"nb_lower=%d nb_upper=%d\n",*narrowband_lower, *narrowband_upper);
        }
    }

    *max_mag = (maxmag_lower > maxmag_upper)? maxmag_lower:maxmag_upper; 
    *max_index = (maxmag_lower > maxmag_upper)? maxindex_lower:(maxindex_upper+64); 
    if ((sc->sc_spectral->classify_scan)) {
                SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"nb_lower=%d nb_upper=%d bmap_lower=%d bmap_upper=%d rssi_lower=%d rssi_upper=%d maxmag_lower=%d maxmag_upper=%d\n",*narrowband_lower, *narrowband_upper, bmapwt_lower, bmapwt_upper, *rssi_lower, *rssi_upper, maxmag_lower, maxmag_upper);
    }
}

void get_ht40_bmap_maxindex(struct ath_softc *sc, u_int8_t *fft_40_data, int datalen, u_int8_t *bmap_wt_lwr, u_int8_t *max_index_lwr, u_int8_t *bmap_wt_higher, u_int8_t *max_index_higher) 
{

    int offset = (SPECTRAL_HT40_TOTAL_DATA_LEN - datalen);
    u_int8_t temp_maxindex=0, temp_bmapwt=0;

    temp_maxindex = fft_40_data[SPECTRAL_HT40_LOWER_MAXINDEX_INDEX + offset];
    temp_maxindex = ((temp_maxindex >> 2) & 0x3F);
    temp_maxindex = fix_maxindex_inv_only(temp_maxindex);

    temp_bmapwt=fft_40_data[SPECTRAL_HT40_LOWER_BMAPWT_INDEX + offset];
    temp_bmapwt &= 0x3F;

    *bmap_wt_lwr = temp_bmapwt;
    *max_index_lwr = temp_maxindex;

    temp_maxindex = fft_40_data[SPECTRAL_HT40_HIGHER_MAXINDEX_INDEX + offset];
    temp_maxindex = ((temp_maxindex >> 2) & 0x3F);
    temp_maxindex = fix_maxindex_inv_only(temp_maxindex);

    temp_bmapwt=fft_40_data[SPECTRAL_HT40_HIGHER_BMAPWT_INDEX + offset];
    temp_bmapwt &= 0x3F;

    *bmap_wt_higher = temp_bmapwt;
    *max_index_higher = temp_maxindex;
}

#endif
