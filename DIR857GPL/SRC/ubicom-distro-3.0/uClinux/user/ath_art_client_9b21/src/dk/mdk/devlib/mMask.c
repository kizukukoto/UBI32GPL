// To Do List :
//
//  1. verify setResetParams

#ifdef _WINDOWS
#include <windows.h>
#endif 
#include <stdio.h>
#ifdef WIN32
#include <conio.h>
#endif
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "wlantype.h"   /* typedefs for A_UINT16 etc.. */
#include "athreg.h"
#include "manlib.h"     /* The Manufacturing Library */
#include "mConfig.h"

#include "mMaskPhys.h"
#include "mMaskMath.h"

A_UINT32 goldDevNum;
A_UINT32 dutDevNum;
double total_gain_val = 0;

extern A_UINT32 CAP_DESC_PTR;
extern A_UINT32 CAP_DATA_PTR;	 

MANLIB_API void set_dev_nums(A_UINT32 gold_dev, A_UINT32 dut_dev) {
	goldDevNum = gold_dev;
	dutDevNum  = dut_dev;
}

// Routine to force the noise floor (NOTE: values latch after do_noisefloor!!)
MANLIB_API void force_minccapwr (A_UINT32 maxccapwr) {
	A_UINT32  do_noisefloor = 1 ;
	// Disable noise cal
	REGW(goldDevNum, 0x9860,REGR(goldDevNum, 0x9860)&~0x8002);

	// Force value of noise floor (maxCCApwr) and 
	REGW(goldDevNum, 0x9864, sign_trunc(maxccapwr*2,9));
    
	// Do noise cal to load minCCApwr register
	REGW(goldDevNum, 0x9860,REGR(goldDevNum, 0x9860)|(do_noisefloor<<1));

	while (do_noisefloor == 1) {
		do_noisefloor = (REGR(goldDevNum, 0x9860)|0x2) >> 1;
	}
}

// Routine to consume descriptors and report the most likely gains and rssi
MANLIB_API double detect_signal (A_UINT32 desc_cnt, A_UINT32 adc_des_size, A_UINT32 mode, A_UINT32 *gain) {
    A_UINT32 rx_desc_ptr ;
    A_UINT32 rx_data_ptr ;
    A_UINT32 buf_len = 0;
    A_UINT32 tmp_ptr = 0;
    A_UINT32 k = 0;
    double   rssi = 0;
    A_UINT32 rfgain_max = 64;
    A_UINT32 bbgain_max = 64;
    A_UINT32 rfgain_desired = 0;
    A_UINT32 bbgain_desired = 0;
    A_UINT32  rfgain_adj = adc_des_size - adc_desired_size();
	A_BOOL	  debug = 1;

printf("SNOOP: read rfgain_adj=%d\n", rfgain_adj);

    buf_len = pad_rx_buffer(HEAD_LEN+MDK_HEAD_LEN+MDK_DATA_LEN);

printf("SNOOP: buf_len=%d\n", buf_len);

    rx_desc_ptr = memAlloc(goldDevNum, desc_cnt * DESC_LEN);
    rx_data_ptr = memAlloc(goldDevNum, desc_cnt * buf_len);
     
printf("SNOOP: malloc done\n");

    for (k=0;k<desc_cnt;k++) {
        tmp_ptr = rx_desc_ptr + (k*DESC_LEN);
        mem_write(tmp_ptr, tmp_ptr+DESC_LEN);
        mem_write(tmp_ptr+0x4, rx_data_ptr+k*buf_len);
        mem_write(tmp_ptr+0x8, 0x0);
        mem_write(tmp_ptr+0xc, buf_len);
        mem_write(tmp_ptr+0x10, 0x0);
        mem_write(tmp_ptr+0x14, 0x0);
    }

printf("SNOOP: mem_write done \n");


    mem_write(tmp_ptr, 0x0);

    REGW(goldDevNum, 0x803c,0x20);                                   // mac::pcu::rx_filter::promisc
    REGW(goldDevNum, 0x8048,0x0);                                    // mac::pcu::diag_sw::rx_disable
        
    REGW(goldDevNum, 0x000c,rx_desc_ptr);                           // mac::dru::rxdp::linkptr
    REGW(goldDevNum, 0x0008,0x4);                                    // mac::dru::cr::rxe

//printf("SNOOP: REGW done \n");

    report_gain(rx_desc_ptr,desc_cnt, &(gain[0]));


//printf("SNOOP: report gain done \n");

    if (debug) { printf("Snoop: Current => RF: %d, BB: %d\n",gain[0],gain[1]); }

    rfgain_desired = gain[0] + rfgain_adj * 2;
    if (rfgain_desired > rfgain_max) {
        gain[0] = rfgain_max;
        bbgain_desired = gain[1] + (rfgain_desired-rfgain_max);
    } else {
        gain[0] = rfgain_desired;
        bbgain_desired = gain[1];
    }
    if (debug) { printf("Snoop: Desired => RF: %d, BB: %d\n",rfgain_desired,bbgain_desired); }

    if (bbgain_desired > bbgain_max) {
        gain[1] = bbgain_max;
    } else {
        gain[1] = bbgain_desired;
    }

	total_gain_val = total_gain(gain, mode);

printf("SNOOP: total gain=%f \n", total_gain_val);

    rssi = report_rssi(rx_desc_ptr,desc_cnt);

printf("SNOOP: rssi=%f \n", rssi );

	memFree(goldDevNum, rx_desc_ptr);
	memFree(goldDevNum, rx_data_ptr);


//printf("SNOOP: freed memory. done with detect_signal. returning rssi. \n");

    return (rssi);
}


// Routine to fix the gain at a given level
MANLIB_API void config_capture (A_UINT32 dut_dev, A_UCHAR *RX_ID, A_UCHAR *BSS_ID, A_UINT32 channel, A_UINT32 turbo, A_UINT32 *gain, A_UINT32 mode) {
	if (mode==2) { 
		mode = 1;	// 11b => 11g
	}
	setResetParams(goldDevNum, NULL, 0, 0, (A_UCHAR)mode, 0);
	resetDevice(dut_dev, RX_ID , BSS_ID, channel, turbo);
	
	REGW(goldDevNum, 0x9804,1);                                      // turbo=1 to make rx filter 40 MHz
	force_gain(gain[0], gain[1], gain[2], gain[3], gain[4]);
	printf("forced gain to : %d, %d, %d, %d, %d\n", gain[0], gain[1], gain[2], gain[3], gain[4]);
	
	enable_tstadc();
	arm_rx_buffer();
}


// Routine to trigger a pre-defined sweep across the desired channel
// and return the concatenated power spectral density estimate
MANLIB_API A_UINT32 trigger_sweep (A_UINT32 dut_dev, A_UINT32 channel, A_UINT32 mode, A_UINT32 averages, A_UINT32 path_loss, A_BOOL return_spectrum, double *psd) {
	A_UINT32 lvl_offset = path_loss - (A_UINT32) total_gain_val;  // total gain is computed in detect_signal earlier
	A_UINT32 num_fail_pts = 0;
	double *psdl40;
	double *psdl30;
	double *psdm;
	double *psdr30;
	double *psdr40;
//	A_UINT32 psd[ NUM_BASE_MASK_PTS];

	A_UINT32 ii, jj, size40;

	double   *WR_TURBO;
	double   *WI_TURBO;
	double   *WR_BASE;
	double   *WI_BASE;

	A_UINT32 CCK_L0, CCK_L1 ;
	A_UINT32 CCK_M0, CCK_M1;
	A_UINT32 CCK_R0, CCK_R1;
	A_UINT32 OFDM_L0, OFDM_L1;
	A_UINT32 OFDM_M0, OFDM_M1;
	A_UINT32 OFDM_R0, OFDM_R1;

	A_UINT32	a,b,c,d,e;

	if (mode == 2) {
		size40 = NUM_BASE_MASK_PTS;
	} else {
		size40 = NUM_TURBO_MASK_PTS;
	}

	psdl40 = (double *) malloc(size40 * sizeof(double));
	psdr40 = (double *) malloc(size40 * sizeof(double));
	psdl30 = (double *) malloc(NUM_BASE_MASK_PTS * sizeof(double));
	psdr30 = (double *) malloc(NUM_BASE_MASK_PTS * sizeof(double));
	psdm   = (double *) malloc(NUM_TURBO_MASK_PTS * sizeof(double));


	WR_BASE = (double *) malloc( NUM_BASE_MASK_PTS * sizeof(double));
	WI_BASE = (double *) malloc( NUM_BASE_MASK_PTS * sizeof(double));
	WR_TURBO = (double *) malloc( NUM_TURBO_MASK_PTS * sizeof(double));
	WI_TURBO = (double *) malloc( NUM_TURBO_MASK_PTS * sizeof(double));

	printf("snoop: trigger_sweep : done with mallocs \n");

	slice(NUM_TURBO_MASK_PTS,12.5,12.5, &OFDM_R0, &OFDM_R1);
	slice(NUM_BASE_MASK_PTS,25,0, &CCK_L0, &CCK_L1);
	slice(NUM_TURBO_MASK_PTS,25,25, &CCK_M0, &CCK_M1);
	slice(NUM_BASE_MASK_PTS,0,25, &CCK_R0, &CCK_R1);
	slice(NUM_TURBO_MASK_PTS,12.5,12.5, &OFDM_L0, &OFDM_L1);
	slice(NUM_TURBO_MASK_PTS,25,25, &OFDM_M0, &OFDM_M1);

	printf("snoop: trigger_sweep : ofdm r0 r1 m0 m1 l0 l1 = %d %d %d %d %d %d \n", OFDM_R0, OFDM_R1, OFDM_M0, OFDM_M1, OFDM_L0, OFDM_L1);
	printf("snoop: trigger_sweep : CCK r0 r1 m0 m1 l0 l1 = %d %d %d %d %d %d \n", CCK_R0, CCK_R1, CCK_M0, CCK_M1, CCK_L0, CCK_L1);

	wtable(NUM_TURBO_MASK_PTS, &(WR_TURBO[0]), &(WI_TURBO[0]));
	wtable(NUM_BASE_MASK_PTS, &(WR_BASE[0]), &(WI_BASE[0]));

	printf("snoop: trigger_sweep : done generating wtables \n");

	printf("snoop: trigger_sweep: 1st est spec : reg 18 = 0x%x\n", REGR(goldDevNum, 0x9848));

	estimate_spectrum(NUM_TURBO_MASK_PTS,WR_TURBO,WI_TURBO,averages,lvl_offset, &(psdm[0]) );
	read_gain(&(a),&(b),&(c),&(d),&(e));
	printf("snoop: trigger_sweep: read gain after 1st est spec : %d, %d, %d, %d, %d\n", a,b,c,d,e);

	printf("snoop: trigger_sweep : done with 1st estimate_spectrum \n");

    if (mode == 2) {
        REGW(goldDevNum, 0x9804,0);
        changeChannel(dut_dev,channel-40);
	estimate_spectrum(NUM_BASE_MASK_PTS,WR_BASE,WI_BASE,averages,lvl_offset, &(psdl40[0]) );
        changeChannel(dut_dev,channel-30);
	estimate_spectrum(NUM_BASE_MASK_PTS,WR_BASE,WI_BASE,averages,lvl_offset, &(psdl30[0]) );
        changeChannel(dut_dev,channel+30);
	estimate_spectrum(NUM_BASE_MASK_PTS,WR_BASE,WI_BASE,averages,lvl_offset, &(psdr30[0]) );
        changeChannel(dut_dev,channel+40);
	estimate_spectrum(NUM_BASE_MASK_PTS,WR_BASE,WI_BASE,averages,lvl_offset, &(psdr40[0]) );
        changeChannel(dut_dev,channel);
        REGW(goldDevNum, 0x9804,1);

	printf("snoop: trigger_sweep : done with all estimate_spectra for mode==2\n");
	
	jj = 0;	ii = 0;
	while (ii <= (CCK_L1 - CCK_L0)) { psd[jj+ii] = psdl40[CCK_L0+ii]; ii++; }

	jj += ii; ii = 0;
	while (ii <= (CCK_L1 - CCK_L0)) { psd[jj+ii] = psdl30[CCK_L0+ii]; ii++; }
	
	jj += ii; ii = 0;
	while (ii <= (CCK_M1 - CCK_M0)) { psd[jj+ii] = psdm[CCK_M0+ii]; ii++; }
		
	jj += ii; ii = 0;
	while (ii <= (CCK_R1 - CCK_R0)) { psd[jj+ii] = psdr30[CCK_R0+ii]; ii++; }

	jj += ii; ii = 0;
	while (ii <= (CCK_R1 - CCK_R0)) { psd[jj+ii] = psdr40[CCK_R0+ii]; ii++; }

    } else {
        changeChannel(dut_dev,channel-30);
printf("snoop: trigger_sweep: 2nd est spec : reg 18 = 0x%x\n", REGR(goldDevNum, 0x9848));
	estimate_spectrum(NUM_TURBO_MASK_PTS,WR_TURBO,WI_TURBO,averages,lvl_offset, &(psdl40[0]) );
	read_gain(&(a),&(b),&(c),&(d),&(e));
	printf("snoop: trigger_sweep: read gain after 2nd est spec : %d, %d, %d, %d, %d\n", a,b,c,d,e);
        changeChannel(dut_dev,channel+30);
printf("snoop: trigger_sweep: 3rd est spec : reg 18 = 0x%x\n", REGR(goldDevNum, 0x9848));
	estimate_spectrum(NUM_TURBO_MASK_PTS,WR_TURBO,WI_TURBO,averages,lvl_offset, &(psdr40[0]) );
	read_gain(&(a),&(b),&(c),&(d),&(e));
	printf("snoop: trigger_sweep: read gain after 3rd est spec : %d, %d, %d, %d, %d\n", a,b,c,d,e);
        changeChannel(dut_dev,channel);

	printf("snoop: trigger_sweep : done with all estimate_spectra for mode!=2\n");

	jj = 0;	ii = 0;
	while (ii <= (OFDM_L1 - OFDM_L0)) { psd[jj+ii] = psdl40[OFDM_L0+ii]; ii++; 
	//printf("snoop: trigger_sweep: psd[%d] = %f\n", (jj+ii), psd[jj+ii-1]);
	}
	
	jj += ii; ii = 0;
	while (ii <= (OFDM_M1 - OFDM_M0)) { psd[jj+ii] = psdm[OFDM_M0+ii]; ii++; 
	//printf("snoop: trigger_sweep: psd[%d] = %f\n", (jj+ii), psd[jj+ii-1]);
	}

	jj += ii; ii = 0;
	while (ii <= (OFDM_R1 - OFDM_R0)) { psd[jj+ii] = psdr40[OFDM_R0+ii]; ii++; 
	//printf("snoop: trigger_sweep: psd[%d] = %f\n", (jj+ii), psd[jj+ii-1]);
	}

    }

	free(psdl40); free(psdr40); free(psdl30); free(psdr30); free(psdm);
	memFree(goldDevNum, CAP_DATA_PTR);
	memFree(goldDevNum, CAP_DESC_PTR);

	printf("snoop: trigger_sweep : done \n");

	if (return_spectrum) {
		return(num_fail_pts);  //psd = NULL; // dummy op
	} else {
		return(num_fail_pts);
	}

//	return(num_fail_pts);
}
