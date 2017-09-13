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

#ifndef _SPECTRAL_CLASSIFIER_H_
#define _SPECTRAL_CLASSIFIER_H_

#ifdef WIN32
#pragma pack(push, spectral_classifier, 1)
#define __ATTRIB_PACK
#else
#ifndef __ATTRIB_PACK
#define __ATTRIB_PACK __attribute__ ((packed))
#endif
#endif

typedef struct spectral_burst {

    u_int8_t prev_narrowb;
    u_int8_t prev_wideb;
    u_int8_t max_bin;
    u_int8_t min_bin;
    u_int16_t burst_width;
    u_int16_t burst_rssi;
    u_int64_t burst_start;
    u_int64_t burst_stop;
    u_int16_t burst_wideb_width;
    u_int16_t burst_wideb_rssi;

} __ATTRIB_PACK SPECTRAL_BURST;

#ifdef WIN32
#pragma pack(pop, spectral_classifier)
#endif
#ifdef __ATTRIB_PACK
#undef __ATTRIB_PACK
#endif

#endif
