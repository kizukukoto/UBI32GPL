/*
 * Ubicom32 DSPUTILS
 *
 * Copyright (C) 2007 Marc Hoffman <marc.hoffman@analog.com>
 * Copyright (c) 2006 Michael Benjamin <michael.benjamin@analog.com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <unistd.h>
#include "libavcodec/avcodec.h"
#include "libavcodec/dsputil.h"
#include "dsputil_ip7k.h"

int off;


void ff_ip7k_idct (DCTELEM *block);
//void ff_ip7k_fdct (DCTELEM *block);


static void ff_ip7k_put_pixels_clamped_c(const DCTELEM *block, uint8_t *restrict pixels,
                                 int line_size)
{
    int i;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    /* read the pixels */
    for(i=0;i<8;i++) {
        pixels[0] = cm[block[0]];
        pixels[1] = cm[block[1]];
        pixels[2] = cm[block[2]];
        pixels[3] = cm[block[3]];
        pixels[4] = cm[block[4]];
        pixels[5] = cm[block[5]];
        pixels[6] = cm[block[6]];
        pixels[7] = cm[block[7]];

        pixels += line_size;
        block += 8;
    }
}

static void ff_ip7k_add_pixels_clamped_c(const DCTELEM *block, uint8_t *restrict pixels,
                          int line_size)
{
    int i;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    /* read the pixels */
    for(i=0;i<8;i++) {
        pixels[0] = cm[pixels[0] + block[0]];
        pixels[1] = cm[pixels[1] + block[1]];
        pixels[2] = cm[pixels[2] + block[2]];
        pixels[3] = cm[pixels[3] + block[3]];
        pixels[4] = cm[pixels[4] + block[4]];
        pixels[5] = cm[pixels[5] + block[5]];
        pixels[6] = cm[pixels[6] + block[6]];
        pixels[7] = cm[pixels[7] + block[7]];
        pixels += line_size;
        block += 8;
    }
}

static void ip7k_idct_add (uint8_t *dest, int line_size, DCTELEM *block)
{
    ff_ip7k_idct (block);
    ff_ip7k_add_pixels_clamped_c (block, dest, line_size);
}

static void ip7k_idct_put (uint8_t *dest, int line_size, DCTELEM *block)
{
    ff_ip7k_idct (block);
    ff_ip7k_put_pixels_clamped_c (block, dest, line_size);
}


static void ip7k_clear_blocks (DCTELEM *blocks)
{
    // This is just a simple memset.
//
}

/*
  decoder optimization
  start on 2/11 100 frames of 352x240@25 compiled with no optimization -g debugging
  9.824s ~ 2.44x off
  6.360s ~ 1.58x off with -O2
  5.740s ~ 1.43x off with idcts

  2.64s    2/20 same sman.mp4 decode only

*/

void dsputil_init_ip7k( DSPContext* c, AVCodecContext *avctx )
{

    load_idct_firmware();

//    c->get_pixels = get_pixels_c;
//    c->diff_pixels = diff_pixels_c;
    c->put_pixels_clamped = ff_ip7k_put_pixels_clamped_c;
//    c->put_signed_pixels_clamped = put_signed_pixels_clamped_c;
    c->add_pixels_clamped = ff_ip7k_add_pixels_clamped_c;
/*
    c->add_pixels8 = add_pixels8_c;
    c->add_pixels4 = add_pixels4_c;
    c->sum_abs_dctelem = sum_abs_dctelem_c;
    c->gmc1 = gmc1_c;
    c->gmc = ff_gmc_c;
    c->clear_block = clear_block_c;
    c->clear_blocks = clear_blocks_c;
    c->pix_sum = pix_sum_c;
    c->pix_norm1 = pix_norm1_c;
    
    c->sad[0]= pix_abs16_c;
    c->sad[1]= pix_abs8_c;
    c->vsad[0]= vsad16_c;
    c->vsad[4]= vsad_intra16_c;

    c->pix_abs[0][0] = pix_abs16_c;
    c->pix_abs[0][1] = pix_abs16_x2_c;
    c->pix_abs[0][2] = pix_abs16_y2_c;
    c->pix_abs[0][3] = pix_abs16_xy2_c;
    
    c->pix_abs[1][0] = pix_abs8_c;
    c->pix_abs[1][1] = pix_abs8_x2_c;
    c->pix_abs[1][2] = pix_abs8_y2_c;
    c->pix_abs[1][3] = pix_abs8_xy2_c;

    c->sse[0]= sse16_c;
    c->sse[1]= sse8_c;
    c->sse[2]= sse4_c;
*/
    c->idct_permutation_type = FF_NO_IDCT_PERM;
    c->idct               = ff_ip7k_idct;
    c->idct_add           = ip7k_idct_add;
    c->idct_put           = ip7k_idct_put;
}



