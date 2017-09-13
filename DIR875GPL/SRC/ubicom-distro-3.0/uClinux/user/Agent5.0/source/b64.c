/*=======================================================================
  
       Copyright(c) 2009, Works Systems, Inc. All rights reserved.
  
       This software is supplied under the terms of a license agreement 
       with Works Systems, Inc, and may not be copied nor disclosed except 
       in accordance with the terms of that agreement.
  
  =======================================================================*/
/*!
 * \file b64.h
 *
 * \brief The base64 algorithm(only need the encode) implementation
 */
#include "b64.h"
#include "log.h"

int b64_encode(const unsigned char *input, int input_len, char *output, int output_len)
{
    static const char b64_alpha[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int inlen;
    int outlen;
    int mod;
    
    if((input_len + 2) / 3 * 4 + 1 > output_len) { //Make sure the output buffer length is enough
        tr_log(LOG_WARNING, "Out put buffer is not enough!");
        return -1;
    }

    mod = input_len % 3;
    input_len -= mod;
    for(inlen = 0, outlen = 0; input_len > inlen; ) {
        output[outlen++] = b64_alpha[input[inlen] >> 2];
        output[outlen++] = b64_alpha[((input[inlen] & 0x03) << 4) | (input[inlen + 1] >> 4)];
        inlen++;
        output[outlen++] = b64_alpha[((input[inlen] & 0x0f) << 2) | (input[inlen + 1] >> 6)];
        output[outlen++] = b64_alpha[input[++inlen] & 0x3f];
        inlen++;
    }

    if(mod) {
        output[outlen++] = b64_alpha[input[inlen] >> 2];
        output[outlen++] = b64_alpha[((input[inlen] & 0x03) << 4) | (mod > 1 ? (input[inlen + 1] >> 4) : 0)];
        if(mod == 2) {
            output[outlen++] = b64_alpha[(input[inlen + 1] & 0x0f) << 2];
        } else {
            output[outlen++] = '=';
        }
        output[outlen++] = '=';
    }

    output[outlen] = '\0';

    return 0;
}

int b64_decode(char *src, int src_len, unsigned char *dst, int des_len)
{

    int i = 0, j = 0;

    unsigned char base64_decode_map[256] = {
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 62, 255, 255, 255, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 255, 255,
        255, 0, 255, 255, 255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 255, 255, 255, 255, 255, 255, 26, 27, 28,
        29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

    if (src_len/4 * 3 - 2 > des_len || src_len%4) {//make sure the least data is enough
        return -1;
    }

    for (; i < src_len; i += 4) {
        dst[j++] = base64_decode_map[(int)src[i]] << 2 |
            base64_decode_map[(int)src[i + 1]] >> 4;

        dst[j++] = base64_decode_map[(int)src[i + 1]] << 4 |
            base64_decode_map[(int)src[i + 2]] >> 2;

        dst[j++] = base64_decode_map[(int)src[i + 2]] << 6 |
            base64_decode_map[(int)src[i + 3]];

    }

    dst[j] = '\0';
    return 0;

}

int string_is_b64(const char *str)
{
	const char *s;
	unsigned int len = strlen(str);

	if(len % 4 != 0) /*base64 must 4 multiple*/
		return 0;

	s = str;
	while(*s) {
		if((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9') || *s == '+' || *s == '/') {
			s++;
		} else if(*s == '=') {
			if((len - (int)(s - str) > 2) || s[len - (s - str)] != '\0'){  /*= occur before in partial of string*/
				return 0;
			} else if(s[-2] != '=' ){
				return 1;
			} else {
				return 0;
			}
		} else {
			return 0;
		}
	}


	return 1;
}
