// ported from trimmed oahu.pm in perl     -- P.Dua 11/02

// To Do list :
//      set goldDevNum, dutDevNum way upfront
//	fix mem_read calls
//	fix mem_write calls
#ifdef _WINDOWS
#include <windows.h>
#endif 
#include <stdio.h>
#ifdef WIN32 
#include <memory.h>
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
#ifdef ANWI
#include "mld_anwi.h"
#endif
#ifdef LINUX
#include "mld_linux.h"
#endif


#include "mMaskPhys.h"
#include "mMaskMath.h"

A_UINT32 CAP_DESC_PTR;
A_UINT32 CAP_DATA_PTR;	 


extern A_UINT32 goldDevNum;         // Needs to be set up at the begining. obviates having to pass around devNum to every call
extern A_UINT32 dutDevNum;          // Needs to be set up at the begining. probably never needed - DUT just transmits. 


// Routine to determine the start and stop indices
// of an n-point array based on the +/- % values
void slice (A_UINT32 n, double plus_pct, double minus_pct, 	A_UINT32 *beg, A_UINT32 *end) { 

	minus_pct = (minus_pct > 50) ? 50 : minus_pct;
	plus_pct = (plus_pct > 50) ? 50 : plus_pct;
	*beg = (n/2) - (A_UINT32)(n*minus_pct/100);
	*end = (n/2) + (A_UINT32)(n*plus_pct/100) - 1;
}

// Routine to reverse the least significant bits
A_INT32 bit_rev (A_INT32 val, A_INT32 bits) { 
	A_INT32 k,retval=0;
	
	for (k=0; k<bits; k++) {
		retval <<= 1;
		retval |= (val >> k) & 1;
	}
	return(retval);
}

// Routine to create an n-bit mask
A_UINT32 mask (A_UINT32 n) { 
    return((((1 << (n-1)) - 1) << 1) + 1);
}

// Routine to truncate a signed 32-bit to signed n-bit word
A_INT32 sign_ext (A_INT32 field, A_UINT32 wid) { 
    if ((field >> (wid-1)) == 1) {
        return(-1*((field ^ mask(wid)) + 1));
    }
    return(field);
}

// Routine to truncate a signed 32-bit to signed n-bit word
A_INT32 sign_trunc (A_INT32 signed_val, A_UINT32 wid) { 
    A_UINT32 neg      = (signed_val < 0) ? 1 : 0;
    A_UINT32 mask_wid     = mask(wid);
    A_UINT32 mantissa = mask(wid-1);

    A_UINT32 mag = neg ? -signed_val : signed_val;
    if (neg) { mantissa++; }
    if (mag > mantissa) { 
        printf( "\nIn function sign_trunc, Magnitude mag (%d) is larger than mantissa (%d)!\n", mag, mantissa);
	exit(0);
    } else {
        return(neg ? (mag ^ mask_wid)+1 : mag);
    }
}

// Routine to extract a field
A_UINT32 field_read (A_UINT32 base, A_UINT32 reg, A_UINT32 ofs, A_UINT32 bits) { 
	A_UINT32 addr = base+(reg<<2);
	A_UINT32 mask_bits = mask(bits);
	return ((REGR(goldDevNum, addr) >> ofs) & mask_bits);
}

// Routine to modify a field
void field_write (A_UINT32 base, A_UINT32 reg, A_UINT32 ofs, A_UINT32 bits, A_UINT32 unsignd) { 
	A_UINT32 addr = base+(reg<<2);
	A_UINT32 mask_bits = mask(bits);
	if (unsignd > mask_bits) { 
		printf( "\nIn function field_write, value for the field %d is larger than the mask (%d)!\n", unsignd, mask_bits); 
		exit(0);
	} else {
		REGW(goldDevNum, addr,(REGR(goldDevNum, addr) & ~(mask_bits<<ofs) | (unsignd<<ofs)));
	}
}

// Routine to read expected attenuation of tx-rx switch
A_UINT32 txrxatten () { 
    return (REGR(goldDevNum, 0x9848) >> 12) & 0x3f;
}

// Routine to read expected attenuation of beanie lna switch
A_UINT32 blnaatten_db_2ghz () { 
    return (REGR(goldDevNum, 0xa20c) >> 5) & 0x1f;
}

// Routine to read expected attenuation of beanie-sombrero switch
A_UINT32 bswatten_db_2ghz () { 
    return REGR(goldDevNum, 0xa20c) & 0x1f;
}

// Routine to configure most of the agc timing and level params
void adjust_agc (A_UINT32 switch_settling, A_UINT32 agc_settling, double coarse_high,  
		 double coarsepwr_const, double relpwr, A_UINT32 min_num_gain_change,
		 double total_desired, double pga_desired_size, double adc_desired_size) {

    A_UINT32 wrdata;
    
    wrdata = REGR(goldDevNum, 0x9844) & ~((0x7f<<7) | 0x7f);
    wrdata |= (switch_settling<<15) | agc_settling;
    REGW(goldDevNum, 0x9844, wrdata);
    
    wrdata = REGR(goldDevNum, 0x985c) & ~((0x7f<<15) | 0x7f);
    wrdata |= (sign_trunc((A_UINT32)(coarse_high*2),7)<<15) |  (sign_trunc((A_UINT32)(coarsepwr_const*2),7));
    REGW(goldDevNum, 0x985c, wrdata);
    
    wrdata = REGR(goldDevNum, 0x9858) & ~(0x3f<<6);
    wrdata |= (sign_trunc((A_UINT32)(relpwr*2),6)<<6);
    REGW(goldDevNum, 0x9858, wrdata);
    
    wrdata = REGR(goldDevNum, 0x9860) & ~(0x7<<3);
    wrdata |= (min_num_gain_change<<3);
    REGW(goldDevNum, 0x9860, wrdata);
    
    wrdata = REGR(goldDevNum, 0x9850) & ~((0xff<<20) | (0xff<<8) | 0xff);
    wrdata |= ((sign_trunc((A_UINT32)(total_desired*2),8)<<20) |
	       (sign_trunc((A_UINT32)(pga_desired_size*2),8)<<8) |
	       (sign_trunc((A_UINT32)(adc_desired_size*2),8)));
    REGW(goldDevNum, 0x9850, wrdata);
}

// Routine to force gain tables and switches (NOTE: values latch on rx_frame!!) 
void force_gain (A_UINT32 rfgain, A_UINT32 bbgain, A_UINT32 blnaatten_db_2ghz, 
		 A_UINT32 bswatten_db_2ghz, A_UINT32 txrxatten) {
    
    A_UINT32 blna_switch_2ghz = (blnaatten_db_2ghz!=0)?0:1;
    A_UINT32 bsw_switch_2ghz = (bswatten_db_2ghz!=0)?1:0;
    A_UINT32 rxtx_flag = (txrxatten!=0)?1:0;
    A_UINT32 wrdata;
    
    wrdata = REGR(goldDevNum, 0x9848) | (1<<23); // enable force rfgain, bbgain, rxtx_flag etc...
    REGW(goldDevNum, 0x9848,wrdata);
    wrdata = REGR(goldDevNum, 0x984c) & ~((0x7f<<25) | (0x7f<<18) | (0x1<<17));
    wrdata |= ((rfgain<<25) | (bbgain<<18) | (rxtx_flag<<17));
    REGW(goldDevNum, 0x984c,wrdata);
    wrdata = REGR(goldDevNum, 0xa20c) & ~((0x1<<25) | (0x1<<24));
    wrdata |= ((blna_switch_2ghz<<25) | (bsw_switch_2ghz<<24));
    REGW(goldDevNum, 0xa20c,wrdata);

	printf("snoop: force_gain: reg 18 = 0x%x\n", REGR(goldDevNum, 0x9848));
    
}

// Routine to force gain tables and switches
void read_gain (A_UINT32 *rfgain,A_UINT32 *bbgain,A_UINT32 *blnaatten_db_2ghz, 
		A_UINT32 *bswatten_db_2ghz,A_UINT32 *txrxatten) {

    A_UINT32 blna_switch_2ghz,bsw_switch_2ghz,rxtx_flag;
    A_UINT32 rddata;

    rddata = REGR(goldDevNum, 0x984c);
    *rfgain = ((rddata >> 25) & 0x7f);  
    *bbgain = ((rddata >> 18) & 0x7f);  
    rxtx_flag = ((rddata >> 17) & 0x1);
    if (rxtx_flag!=0) {
	rddata = REGR(goldDevNum, 0x9848);
	*txrxatten = ((rddata >> 12) & 0x3f);
    } else {
      *txrxatten = 0;
    }

    rddata = REGR(goldDevNum, 0xa20c);
    blna_switch_2ghz = ((rddata >> 25) & 0x1);
    bsw_switch_2ghz = ((rddata >> 24) & 0x1);
    if (blna_switch_2ghz==0) {
	*blnaatten_db_2ghz = ((rddata >> 5) & 0x1f);
    } else {
      *blnaatten_db_2ghz = 0;
    }

    if (bsw_switch_2ghz!=0) {
	*bswatten_db_2ghz = (rddata & 0x1f);
    } else {
      *bswatten_db_2ghz = 0;
    }
}

// Routine to calculate the total gain - takes a gain array and mode (dk::mode 11a/b/g) as input 
double total_gain (A_UINT32 *gain, A_UINT32 mode) {
    if (mode == 0) {
        return (double)(gain[0]+ gain[1])/2.0 - gain[4];
    } else {
        return (double)( gain[0]+ gain[1])/2.0 - gain[2]- gain[3]- gain[4];
    }
}

// Routine to pad the rx buffer length to an integer multiple of 4
A_UINT32 pad_rx_buffer (A_UINT32 buf_len) { 
    return ((A_UINT32)((buf_len+3)/4))*4;
}

// Routine to report the most likely gain settings during descriptor consumption
void report_gain (A_UINT32 rx_desc_ptr, A_UINT32 desc_cnt, A_UINT32 *gain) { 
    A_UINT32 desc5 = rx_desc_ptr+(desc_cnt-1)*DESC_LEN+0x14 ;
    A_UINT32 k = 0;
    A_UINT32 start_time = milliTime();
    A_UINT32 *rfgain;
    A_UINT32 *bbgain;
    A_UINT32 *blna;
    A_UINT32 *xatten1;
    A_UINT32 *xatten2;

    A_BOOL   TIMED_OUT = FALSE;
    char  name[25];

//printf("snoop:report_gain: entered report_gain\n");

	rfgain = (A_UINT32 *) malloc(20*sizeof(A_UINT32));
	bbgain = (A_UINT32 *) malloc(20*sizeof(A_UINT32));
	blna = (A_UINT32 *) malloc(20*sizeof(A_UINT32));
	xatten1 = (A_UINT32 *) malloc(20*sizeof(A_UINT32));
	xatten2 = (A_UINT32 *) malloc(20*sizeof(A_UINT32));

//printf("snoop:report_gain: done with mallocs\n");

    while (((mem_read(desc5) & 0x1) != 0x1) && (!TIMED_OUT)) {
printf("snoop:report_gain: iteration %d : read_gain res", k);
	read_gain(&(rfgain[k]),&(bbgain[k]),&(blna[k]),&(xatten1[k]),&(xatten2[k]));
printf("ults are: %d, %d, %d, %d, %d\n", rfgain[k],bbgain[k],blna[k],xatten1[k],xatten2[k]);
        k++; 
	mSleep(10);

        if (milliTime() >= start_time+10000) { 
	    printf( "Timed out in report_gainf after 10s!\n"); 
	    TIMED_OUT = TRUE;
	}

	if (k>16) {
	    TIMED_OUT = TRUE;
	}
    }

printf("snoop:report_gain: out of the loop with k = %d\n", k);

    mem_write(desc5, 0x0);

    strcpy(name, "rfgain");
    gain[0] = mode(&(rfgain[0]), k, name);
printf("snoop:report_gain: rfgain mode = %d\n", gain[0]);
    strcpy(name, "bbgain");
    gain[1] = mode(&(bbgain[0]), k, name);
printf("snoop:report_gain: bbgain mode = %d\n", gain[1]);
    strcpy(name, "blna");
    gain[2] = mode(&(blna[0]), k, name);
printf("snoop:report_gain: blna mode = %d\n", gain[2]);
    strcpy(name, "xatten1");
    gain[3] = mode(&(xatten1[0]), k, name);
printf("snoop:report_gain: xatten1 mode = %d\n", gain[3]);
    strcpy(name, "xatten2");
    gain[4] = mode(&(xatten2[0]), k, name);
printf("snoop:report_gain: xatten2 mode = %d\n", gain[4]);


 free(rfgain); free(bbgain); free(blna); free(xatten1); free(xatten2);
}

// Routine to report the status information after descriptor consumption
double report_status (A_UINT32 rx_desc_ptr, A_UINT32 desc_cnt) { 

    A_UINT32 j = 0;
    A_UINT32 k = 0;
    A_UINT32 *retval;
	A_UINT32 return_val;
    A_UINT32 datalen = 0;
    A_UINT32 tmp_ptr = 0;
    A_UINT32 desc5 = 0;
    A_UINT32 done = 0;
    A_UINT32 frrxok = 0;
    A_UINT32 crcerr = 0;
    A_UINT32 fifooverrun = 0;
    A_UINT32 decryptcrcerr = 0;
    A_UINT32 phyerr = 0;
    A_UINT32 keyidxvalid = 0;
    A_UINT32 keycachemiss = 0;
    A_UINT32 desc4 = 0;
    A_UINT32 more = 0;
    A_UINT32 rssi = 0;
    double rssi_out_resolution = read_rssi_out();

    //    FILE *STATUS;
    char name[25];

	retval = (A_UINT32 *) malloc(100*sizeof(A_UINT32));

//    if( (STATUS = fopen( "status.nfo", "w")) == NULL ) {
//	  printf("Failed to open status.nfo - using Defaults\n");
//	  exit(0);
//    }

//    fprintf( STATUS, "RSSI\tDONE\tDATALEN\tMORE\tFRRXOK\tCRCERR\tFIFO\tDECRYPT\tPHYERR\tKEYIDX\tKEYCACHE\n");
//    fprintf( STATUS, "\t\t\t\t\t\tOVERRUN\tCRCERR\t\tVALID\tMISS\n");
    for (j=0;j<desc_cnt; j++) {
        tmp_ptr      = rx_desc_ptr + (j*DESC_LEN);   
        desc5         = mem_read(tmp_ptr + 0x14);
        done          = (desc5 & 0x1);
        frrxok        = ((desc5 & 0x2) != 0x0) ? 1 : 0;
        crcerr        = ((desc5 & 0x4) != 0x0) ? 1 : 0;
        fifooverrun   = ((desc5 & 0x8) != 0x0) ? 1 : 0;
        decryptcrcerr = ((desc5 & 0x10) != 0x0) ? 1 : 0;
        phyerr        = ((desc5 & 0x60) != 0x0) ? 1 : 0;
        keyidxvalid   = ((desc5 & 0x80) != 0x0) ? 1 : 0;
        keycachemiss  = ((desc5 & 0x10000000) != 0x0) ? 1 : 0;
        desc4         = mem_read(tmp_ptr + 0x10);
        datalen       = desc4 & 0xfff;
        more          = (desc4 >> 12) & 0x1;
        rssi          = sign_ext((desc4 >> 19) & 0xff,8);
        if (done & frrxok) {
            retval[k++] = rssi ;
        }
	//        fprintf( STATUS, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",rssi,done,datalen,more,frrxok,
	//	 crcerr,fifooverrun,decryptcrcerr,phyerr,keyidxvalid,keycachemiss);

    }
    //    fclose(STATUS) ;

    strcpy(name, "rssi");
	return_val = mode(&(retval[0]), k, name);
	free(retval);
    return(rssi_out_resolution*return_val);
}

// Routine to report the rssi after descriptor consumption
double report_rssi (A_UINT32 rx_desc_ptr, A_UINT32 desc_cnt) { 
    A_UINT32 j = 0;
    A_UINT32 k = 0;
    A_UINT32 tmp_ptr = 0;
    A_UINT32 desc4 = 0;
    A_UINT32 desc5 = 0;
    A_UINT32 rssi = 0;
    double rssi_out_resolution = read_rssi_out();
    double final_ret_val;
    A_UINT32 *retval;
	A_UINT32 return_val;

    char  name[25];

	retval = (A_UINT32 *) malloc(100*sizeof(A_UINT32));

    for (j=0;j<desc_cnt; j++) {
        tmp_ptr = rx_desc_ptr + (j*DESC_LEN);
        desc4 = mem_read(tmp_ptr + 0x10);
        desc5 = mem_read(tmp_ptr + 0x14);
        if ((desc5 & 0x3) == 0x3) {
            rssi = sign_ext((desc4 >> 20) & 0xff,8);	
printf("snoop: report_rssi: rssi[%d] = %d  (res = %f)\n",k, rssi, rssi_out_resolution);	
            retval[k++] = rssi;
        }
    }
    strcpy(name, "rssi");
	return_val =  mode(&(retval[0]),k,name);
	final_ret_val = (double) (return_val*rssi_out_resolution);
	free(retval);
    return(final_ret_val);
}

// Routine to read back setting for adc_desired_size
A_INT32 adc_desired_size () { 
    return sign_ext((REGR(goldDevNum, 0x9850) & 0xff),8)/2;
}



// Routine to setup the receive capture buffer and enable the tstadc
void enable_tstadc () { 
    CAP_DESC_PTR = memAlloc(goldDevNum, DESC_LEN);
    CAP_DATA_PTR = memAlloc(goldDevNum, CAPTURE_LEN);
    mem_write(CAP_DESC_PTR,      0x0);
    mem_write(CAP_DESC_PTR+0x4,  CAP_DATA_PTR);
    mem_write(CAP_DESC_PTR+0x8,  0x0);
    mem_write(CAP_DESC_PTR+0xc,  0xfff);
    mem_write(CAP_DESC_PTR+0x10, 0x0);
    mem_write(CAP_DESC_PTR+0x14, 0x0);
    REGW(goldDevNum, 0x803c,0x20);                                   // mac::pcu::rx_filter::promiscuous
    REGW(goldDevNum, 0x8048,0x0);                                    // mac::pcu::diag_sw::rx_disable
    REGW(goldDevNum, 0x9808, REGR(goldDevNum, 0x9808) | (1<<8) | (1<<10));   // tstadc_en=1, rx_obs_sel=adc
	printf("snoop: enable_tstadc : reg_9808 = 0x%x\n", REGR(goldDevNum, 0x9808));
    mSleep(10);
}

// Routine to reset and arm the tstadc
void arm_rx_buffer () { 
    A_UINT32 addr;

    for (addr=0; addr<CAPTURE_LEN/4; addr++) {
        mem_write(CAP_DATA_PTR+(addr<<2), 0x0);
    }
    REGW(goldDevNum, 0x8054, 0x0);                                   // pcu::tst_addac::test_mode
    mSleep(5);
    
    REGW(goldDevNum, 0x000c,CAP_DESC_PTR);                           // mac::dru::rxdp::linkptr
    REGW(goldDevNum, 0x0008,0x4);                                    // mac::dru::cr::rxe
    
    REGW(goldDevNum, 0x8054, (1<<20) | (1<<14) | 2);                 // pcu::tst_addac::[arm_rx_buffer
    mSleep(5);	
}
    

// Routine to capture the last n samples of tstadc data
void begin_capture (A_UINT32 n, double *i, double *q) { 
    A_UINT32 desc5 = CAP_DESC_PTR+0x14;
    A_UINT32 k = 0;
    A_UINT32 dwords = CAPTURE_LEN/4;
    A_UINT32 addr = 0;
    A_UINT32 rdaddr = 0;
    A_UINT32 rddata = 0;

	double ADC_OFFSET	 = pow(2,ADC_BITS)/2;
	double ADC_SCALE	 = ADC_VPP/pow(2,ADC_BITS);

    REGW(goldDevNum, 0x8054, REGR(goldDevNum, 0x8054) | (1<<19));            // pcu::tst_addac::begin_capture
    mSleep(10);

    while (((mem_read(desc5) & 0x1)) != 0x1) { 1; }
    mem_write(desc5, 0x0);

    // read tstadc data except last 2 samples which are empty
    for (addr=(dwords-n/2-1); addr<(dwords-1); addr++) {
        rdaddr = CAP_DATA_PTR + (addr<<2);
        rddata = mem_read(rdaddr);
    
        q[2*k]   = ((((rddata >> 8) & 0xff) << 1) - ADC_OFFSET) * ADC_SCALE;
        i[2*k]   = (((rddata & 0xff) << 1) - ADC_OFFSET) * ADC_SCALE;
        q[2*k+1] = ((((rddata >> 24) & 0xff) << 1) - ADC_OFFSET) * ADC_SCALE;
        i[2*k+1] = ((((rddata >> 16) & 0xff) << 1) - ADC_OFFSET) * ADC_SCALE;

        k++;    
    }
}


// Routine to capture the last n samples of tstadc data
// and estimate the power spectral density over averages
void estimate_spectrum (A_UINT32 n, double *wr, double *wi, A_UINT32 averages, A_UINT32 lvl_offset, double *psd) { 

    A_UINT32 j = 0;
    A_UINT32 k = 0;
    double *i, *q, *iq;
    double psd_offset = -PSD_SCALE*log((n*n)*averages);

	i  = (double *) malloc(n*sizeof(double));
	q  = (double *) malloc(n*sizeof(double));
	iq = (double *) malloc(n*sizeof(double));

	if ((i==NULL) || (q==NULL) || (iq==NULL)) {
			printf("could not allocate memory for i, q or iq\n");
			if (i != NULL) { free(i); }
			if (q != NULL) { free(q); }
			if (iq != NULL) { free(iq); }
			exit(0);
	}
	printf("snoop: estimate_spectrum : n=%d, avgs = %d, lvl_offset=%d\n", n, averages, lvl_offset);

    for (j=0; j<averages; j++) {
//      printf("snoop: estimate_spectrum : j=%d, ", j);

        arm_rx_buffer();
        begin_capture(n, &(i[0]), &(q[0]));
//      printf("snoop: estimate_spectrum : begun capture, ");

        fft(n,&(i[0]),&(q[0]),wr,wi);
//      printf("snoop: estimate_spectrum : did fft 1, ");
        mag(n,i,q,&(iq[0]));
//      printf("snoop: estimate_spectrum : did mag, ");
        fftshift(n,&(iq[0]));
//      printf("snoop: estimate_spectrum : did fft 2\n ");

        for (k=0; k<n; k++) {
            if (iq[k]!=0) { psd[k] += iq[k]; }
        }

    }
    // ---------------------------------------------------------------
    // PSD is calculated as:
    //
    //     averages
    //       ----         --             --
    //        \           |  *iq[j]/n  |
    //         >  20*log10|  -----------  |
    //        /           |   averages   |
    //       ----         __             __
    //       j=0
    //
    // ---------------------------------------------------------------
    for (k=0; k<n; k++) {
        psd[k] = PSD_SCALE*log(psd[k])+psd_offset+lvl_offset;
    }

    psd[n/2] = (psd[n/2-1]+psd[n/2+1])/2;

	free(i);
	free(q);
	free(iq);
      printf("snoop: estimate_spectrum : done \n");
}



// Routine to calculate turbo mode filter response at a given frequency
double turbo_resp (A_INT32 freq) { 
	A_UINT32  x = abs(freq);
	double retval;

	if (x <= 19e6) {
		retval = 0.0;
	} else if (x <= 20e6) {
		retval = interp(0.0,-0.5,19000000,20000000,x);
	} else if (x <= 30e6) {
		retval = interp(-0.5,-7.0,20000000,30000000,x);
	} else if (x <= 40e6) {
		retval = interp(-7.0,-13.5,30000000,40000000,x);
	} else {
		retval = -13.5;
	}
	return(retval);
}
           
// Routine to select the resolution of the rssi register
void select_rssi_out (double resolution) { 
	A_UINT32  format = 0;
	if (resolution == .5)  { 
		format = 1; 
	} else if (resolution == .25) { 
		format = 2; 
	}

	REGW(goldDevNum, 0x985c,REGR(goldDevNum, 0x985c) & ~(3<<30) | (format<<30));
}

// Routine to read the resolution of the rssi register
double read_rssi_out () { 
	A_UINT32 format = (REGR(goldDevNum, 0x985c) >> 30) & 0x3;

	if (format == 1) { 
		return .5; 
	} else if (format == 2) { 
		return .25; 
	} else { 
		return 1; 
	}
}

// Routine to read back the rssi register
A_UINT32 read_rssi () { 
    return sign_ext(REGR(goldDevNum, 0x9c1c),8);
}



// Routine to read back minccapwr register
A_UINT32 read_minccapwr () { 
    return sign_ext((REGR(goldDevNum, 0x9864) >> 19),9);
}

// Routine to read testout
A_UINT32 read_testout (A_UINT32 bits, A_UINT32 offset) {
//    if (not defined offset) { offset = 0; }
    REGW(goldDevNum, 0x987c+(bits<<2),0x0);
    return bit_rev((REGR(goldDevNum, 0x9c00) >> (32-bits-offset)) & mask(bits),bits);
}


A_UINT32 mem_read(A_UINT32 addr) {
	A_UINT32 buffer;
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[goldDevNum];

	pLibDev->devMap.OSmemRead(goldDevNum,addr, (A_UCHAR *)&buffer, 4);
	return(buffer);
}

void mem_write(A_UINT32 addr, A_UINT32 val) {
	LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[goldDevNum];
	pLibDev->devMap.OSmemWrite(goldDevNum, addr, (A_UCHAR *)&val, 4);
}
