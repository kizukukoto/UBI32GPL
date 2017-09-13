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

#ifndef _SPECTRAL_IOCTL_H_
#define _SPECTRAL_IOCTL_H_

#include "dfs_ioctl.h"
/*
 * ioctl defines
 */

#define	SPECTRAL_SET_CONFIG		(DFS_LAST_IOCTL + 1)
#define	SPECTRAL_GET_CONFIG		(DFS_LAST_IOCTL + 2)
#define	SPECTRAL_SHOW_INTERFERENCE	(DFS_LAST_IOCTL + 3) 
#define	SPECTRAL_ENABLE_SCAN	        (DFS_LAST_IOCTL + 4) 
#define	SPECTRAL_DISABLE_SCAN	        (DFS_LAST_IOCTL + 5) 
#define	SPECTRAL_ACTIVATE_SCAN	        (DFS_LAST_IOCTL + 6) 
#define	SPECTRAL_STOP_SCAN	        (DFS_LAST_IOCTL + 7) 
#define	SPECTRAL_SET_DEBUG_LEVEL  	(DFS_LAST_IOCTL + 8) 
#define	SPECTRAL_IS_ACTIVE  	        (DFS_LAST_IOCTL + 9) 
#define	SPECTRAL_IS_ENABLED  	        (DFS_LAST_IOCTL + 10) 
#define	SPECTRAL_CLASSIFY_SCAN	        (DFS_LAST_IOCTL + 11) 
#define	SPECTRAL_GET_CLASSIFIER_CONFIG	(DFS_LAST_IOCTL + 12)
#define	SPECTRAL_EACS           	(DFS_LAST_IOCTL + 13)
#define	SPECTRAL_ACTIVATE_FULL_SCAN    	(DFS_LAST_IOCTL + 14)
#define	SPECTRAL_STOP_FULL_SCAN    	    (DFS_LAST_IOCTL + 15)

/* 
 * ioctl parameter types
 */

#define	SPECTRAL_PARAM_FFT_PERIOD	1
#define	SPECTRAL_PARAM_SCAN_PERIOD      2
#define	SPECTRAL_PARAM_SCAN_COUNT	3
#define	SPECTRAL_PARAM_SHORT_REPORT	4
#define	SPECTRAL_PARAM_ACTIVE	        5
#define	SPECTRAL_PARAM_STOP	        6
#define	SPECTRAL_PARAM_ENABLE	        7

struct spectral_ioctl_params {
	int16_t		spectral_fft_period;
	int16_t		spectral_period;
	int16_t		spectral_count;	
	HAL_BOOL	spectral_short_report;	
};

#define	SPECTRAL_IOCTL_PARAM_NOVAL	-65535

#endif
