
MANLIB_API void set_dev_nums(A_INT8 gold_dev, A_INT8 dut_dev) ;
MANLIB_API void force_minccapwr (A_UINT32 maxccapwr) ;
MANLIB_API double detect_signal (A_UINT32 desc_cnt, A_UINT32 adc_des_size, A_UINT32 mode, A_UINT32 *gain) ;
MANLIB_API void config_capture (A_UINT32 dut_dev, A_UCHAR *RX_ID, A_UCHAR *BSS_ID, A_UINT32 channel, A_UINT32 turbo, A_UINT32 *gain, A_UINT32 mode) ;
MANLIB_API void trigger_sweep (A_UINT32 dut_dev, A_UINT32 channel, A_UINT32 mode, A_UINT32 averages, A_UINT32 path_loss, double *psd) ;

