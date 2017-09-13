


#define HEAD_LEN	 0x18
#define MDK_HEAD_LEN	 10
#define MDK_DATA_LEN	 2300
#define DESC_LEN	 0x18



#define VERBOSE	 0
#define CAPTURE_LEN	 4096
#define ADC_BITS	 9
#define ADC_VPP	 1
#define PSD_SCALE	 20/log(10)



void slice (A_UINT32 n, double plus_pct, double minus_pct, 	A_UINT32 *beg, A_UINT32 *end) ; 
A_INT32 bit_rev (A_INT32 val, A_INT32 bits) ; 
A_UINT32 mask (A_UINT32 n) ; 
A_INT32 sign_ext (A_INT32 field, A_UINT32 wid) ; 
A_INT32 sign_trunc (A_INT32 signed_val, A_UINT32 wid) ; 
A_UINT32 field_read (A_UINT32 base, A_UINT32 reg, A_UINT32 ofs, A_UINT32 bits) ; 
void field_write (A_UINT32 base, A_UINT32 reg, A_UINT32 ofs, A_UINT32 bits, A_UINT32 unsignd) ; 
A_UINT32 txrxatten () ; 
A_UINT32 blnaatten_db_2ghz () ; 
A_UINT32 bswatten_db_2ghz () ; 
void adjust_agc (A_UINT32 switch_settling, A_UINT32 agc_settling, double coarse_high,  
		 double coarsepwr_const, double relpwr, A_UINT32 min_num_gain_change,
		 double total_desired, double pga_desired_size, double adc_desired_size) ;

void force_gain (A_UINT32 rfgain, A_UINT32 bbgain, A_UINT32 blnaatten_db_2ghz, 
		 A_UINT32 bswatten_db_2ghz, A_UINT32 txrxatten) ;

void read_gain (A_UINT32 *rfgain,A_UINT32 *bbgain,A_UINT32 *blnaatten_db_2ghz, 
		A_UINT32 *bswatten_db_2ghz,A_UINT32 *txrxatten) ;

double total_gain (A_UINT32 *gain, A_UINT32 mode) ;

A_UINT32 pad_rx_buffer (A_UINT32 buf_len) ; 
void report_gain (A_UINT32 rx_desc_ptr, A_UINT32 desc_cnt, A_UINT32 *gain) ; 
double report_status (A_UINT32 rx_desc_ptr, A_UINT32 desc_cnt) ; 
double report_rssi (A_UINT32 rx_desc_ptr, A_UINT32 desc_cnt) ; 
A_INT32 adc_desired_size () ; 
void enable_tstadc () ; 
void arm_rx_buffer () ; 
void begin_capture (A_UINT32 n, double *i, double *q) ; 
void estimate_spectrum (A_UINT32 n, double *wr, double *wi, A_UINT32 averages, A_UINT32 lvl_offset, double *psd) ; 
double turbo_resp (A_INT32 freq) ; 
void select_rssi_out (double resolution) ; 
double read_rssi_out () ; 
A_UINT32 read_rssi () ; 
A_UINT32 read_minccapwr () ; 
A_UINT32 read_testout (A_UINT32 bits, A_UINT32 offset) ;



A_UINT32 mem_read(A_UINT32 addr);
void mem_write(A_UINT32 addr, A_UINT32 val);
