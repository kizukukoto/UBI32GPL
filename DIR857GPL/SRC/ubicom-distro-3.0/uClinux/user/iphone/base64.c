#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "def.h"
#include "debug.h"

///*
// * base64 encoder
// *
// * encode 3 8-bit binary bytes as 4 '6-bit' characters
// */
//char *b64_encode( unsigned char *src ,int src_len, unsigned char* space, int space_len){
//	static const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
//
//	unsigned char *out = space;
//	unsigned char *in=src;
//	int sub_len,len;
//	int out_len;
//
//	out_len=0;
//
//	if (src_len < 1 ) return NULL;
//	if (!src) return NULL;
//	if (!space) return NULL;
//	if (space_len < 1) return NULL;
//
//
//	/* Required space is 4/3 source length  plus one for NULL terminator*/
//	if ( space_len < ((1 +src_len/3) * 4 + 1) )return NULL;
//
//	memset(space,0,space_len);
//
//	for (len=0;len < src_len;in=in+3, len=len+3)
//	{
//
//		sub_len = ( ( len + 3  < src_len ) ? 3: src_len - len);
//
//		/* This is a little inefficient on space but covers ALL the
//		   corner cases as far as length goes */
//		switch(sub_len)
//		{
//			case 3:
//				out[out_len++] = cb64[ in[0] >> 2 ];
//				out[out_len++] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ] ;
//				out[out_len++] = cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] ;
//				out[out_len++] = cb64[ in[2] & 0x3f ];
//				break;
//			case 2:
//				out[out_len++] = cb64[ in[0] >> 2 ];
//				out[out_len++] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ] ;
//				out[out_len++] = cb64[ ((in[1] & 0x0f) << 2) ];
//				out[out_len++] = (unsigned char) '=';
//				break;
//			case 1:
//				out[out_len++] = cb64[ in[0] >> 2 ];
//				out[out_len++] = cb64[ ((in[0] & 0x03) << 4)  ] ;
//				out[out_len++] = (unsigned char) '=';
//				out[out_len++] = (unsigned char) '=';
//				break;
//			default:
//				break;
//				/* do nothing*/
//		}
//	}
//	out[out_len]='\0';
//	return out;
//}
//
///* Base-64 decoding.  This represents binary data as printable ASCII
// ** characters.  Three 8-bit binary bytes are turned into four 6-bit
// ** values, like so:
// **
// **   [11111111]  [22222222]  [33333333]
// **
// **   [111111] [112222] [222233] [333333]
// **
// ** Then the 6-bit values are represented using the characters "A-Za-z0-9+/".
// */
//
//static int b64_decode_table[256] = {
const static int b64_decode_table[256] = {
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
	52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
	-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
	15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
	-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
	41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
};

/* Do base-64 decoding on a string.  Ignore any non-base64 bytes.
 ** Return the actual number of bytes generated.  The decoded size will
 ** be at most 3/4 the size of the encoded, and may be smaller if there
 ** are padding characters (blanks, newlines).
 */
int b64_decode( const char* str, unsigned char* space, int size ){
	const char* cp;
	int space_idx, phase;
	int d, prev_d=0;
	unsigned char c;

	space_idx = 0;
	phase = 0;
	if(str==NULL)
		return space_idx;

	for ( cp = str; *cp != '\0'; ++cp )
	{
		d = b64_decode_table[(int)*cp];
		if ( d != -1 )
		{
			switch ( phase )
			{
				case 0:
					++phase;
					break;
				case 1:
					c = ( ( prev_d << 2 ) | ( ( d & 0x30 ) >> 4 ) );
					if ( space_idx < size )
						space[space_idx++] = c;
					++phase;
					break;
				case 2:
					c = ( ( ( prev_d & 0xf ) << 4 ) | ( ( d & 0x3c ) >> 2 ) );
					if ( space_idx < size )
						space[space_idx++] = c;
					++phase;
					break;
				case 3:
					c = ( ( ( prev_d & 0x03 ) << 6 ) | d );
					if ( space_idx < size )
						space[space_idx++] = c;
					phase = 0;
					break;
			}
			prev_d = d;
		}
	}
	return space_idx;
}


static const uint8_t b64table[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char asciitable[128] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
  0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
  0x3a, 0x3b, 0x3c, 0x3d, 0xff, 0xff,
  0xff, 0xf1, 0xff, 0xff, 0xff, 0x00,	/* 0xf1 for '=' */
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
  0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
  0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12,
  0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
  0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e,
  0x1f, 0x20, 0x21, 0x22, 0x23, 0x24,
  0x25, 0x26, 0x27, 0x28, 0x29, 0x2a,
  0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
  0x31, 0x32, 0x33, 0xff, 0xff, 0xff,
  0xff, 0xff
};

inline static int
encode (char *result, const uint8_t * data, int left)
{

  int data_len;

  if (left > 3)
    data_len = 3;
  else
    data_len = left;

  switch (data_len)
    {
    case 3:
      result[0] = b64table[(data[0] >> 2)];
      result[1] =
	b64table[(((((data[0] & 0x03) & 0xff) << 4) & 0xff) |
		  (data[1] >> 4))];
      result[2] =
	b64table[((((data[1] & 0x0f) << 2) & 0xff) | (data[2] >> 6))];
      result[3] = b64table[(((data[2] << 2) & 0xff) >> 2)];
      break;
    case 2:
      result[0] = b64table[(data[0] >> 2)];
      result[1] =
	b64table[(((((data[0] & 0x03) & 0xff) << 4) & 0xff) |
		  (data[1] >> 4))];
      result[2] = b64table[(((data[1] << 4) & 0xff) >> 2)];
      result[3] = '=';
      break;
    case 1:
      result[0] = b64table[(data[0] >> 2)];
      result[1] = b64table[(((((data[0] & 0x03) & 0xff) << 4) & 0xff))];
      result[2] = '=';
      result[3] = '=';
      break;
    default:
      return -1;
    }

  return 4;

}
/* data must be 4 bytes
 * result should be 3 bytes
 */
#define TOASCII(c) (c < 127 ? asciitable[c] : 0xff)
inline static int
decode (uint8_t * result, const unsigned char * data)
{
  uint8_t a1, a2;
  int ret = 3;

  a1 = TOASCII (data[0]);
  a2 = TOASCII (data[1]);
  if (a1 == 0xff || a2 == 0xff)
    return -1;
  result[0] = ((a1 << 2) & 0xff) | ((a2 >> 4) & 0xff);

  a1 = a2;
  a2 = TOASCII (data[2]);
  if (a2 == 0xff)
    return -1;
  result[1] = ((a1 << 4) & 0xff) | ((a2 >> 2) & 0xff);

  a1 = a2;
  a2 = TOASCII (data[3]);
  if (a2 == 0xff)
    return -1;
  result[2] = ((a1 << 6) & 0xff) | (a2 & 0xff);

  if (data[2] == '=')
    ret--;

  if (data[3] == '=')
    ret--;
  return ret;
}

/* encodes data and puts the result into result (locally allocated)
 * The result_size is the return value
 */
int
_gnutls_base64_encode (const uint8_t * data, size_t data_size,
		       uint8_t ** result)
{
  unsigned int i, j;
  int ret, tmp;
  char tmpres[4];

  ret = B64SIZE (data_size);

  (*result) = malloc (ret + 1);
  if ((*result) == NULL)
    return GNUTLS_E_MEMORY_ERROR;

  for (i = j = 0; i < data_size; i += 3, j += 4)
    {
      tmp = encode (tmpres, &data[i], data_size - i);
      if (tmp == -1)
	{
	  free ((*result));
	  return GNUTLS_E_MEMORY_ERROR;
	}
      memcpy (&(*result)[j], tmpres, tmp);
    }
  (*result)[ret] = 0;		/* null terminated */

  return ret;
}

/* decodes data and puts the result into result (locally allocated)
 * The result_size is the return value
 */
int
_gnutls_base64_decode (uint8_t * data, size_t data_size,
		       uint8_t ** result)
{
  unsigned int i, j;
  int ret, tmp, est;
  uint8_t tmpres[3];

  est = ((data_size * 3) / 4) + 1;

  (*result) = malloc (est);

  if ((*result) == NULL){
    return GNUTLS_E_MEMORY_ERROR;
  }	

  ret = 0;
  for (i = j = 0; i < data_size; i += 4, j += 3)
    {
      tmp = decode (tmpres, &data[i]);
      if (tmp < 0)
	{
	  free (*result);
	  *result = NULL;
	  return tmp;
	}
      memcpy (&(*result)[j], tmpres, tmp);
      ret += tmp;
    }
  return ret;
}

inline int
cpydata (const uint8_t * data, int data_size, uint8_t ** result)
{
  int i, j;

  (*result) = malloc (data_size);
  if (*result == NULL)
    return GNUTLS_E_MEMORY_ERROR;

  for (j = i = 0; i < data_size; i++)
    {
      if (data[i] == '\n' || data[i] == '\r')
	continue;
      (*result)[j] = data[i];
      j++;
    }
  return j;
}