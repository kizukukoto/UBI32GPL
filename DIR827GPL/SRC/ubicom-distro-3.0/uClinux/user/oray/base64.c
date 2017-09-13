#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Base-64 decoding.  This represents binary data as printable ASCII
 ** characters.  Three 8-bit binary bytes are turned into four 6-bit
 ** values, like so:
 **
 **   [11111111]  [22222222]  [33333333]
 **
 **   [111111] [112222] [222233] [333333]
 **
 ** Then the 6-bit values are represented using the characters "A-Za-z0-9+/".
 */

static int b64_decode_table[256] = {
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

static char base64_bits_to_char(unsigned char sixbits)
{

	unsigned char c;
	if (sixbits < 26) {
		c = 'A' + sixbits;
	} else if (sixbits < 52) {
		c = 'a' + sixbits - 26;
	} else if (sixbits < 62) {
		c = '0' + sixbits - 52;
	} else if (sixbits == 62) {
		c = '+';
	} else {
		c = '/';
	}
	return c;
}

/*
 * Base64 Encoding Process:
 *
 *	Picking up 6 bits at a time from the stream and
 *	convert each 6 bit sequence into an ASCII character.
 *	Every 3 characters becomes 4 ASCII bytes.
 *
 *	Bits Used	Bits Shifted	Bits Used	Bits Shifted
 *		6		2		0		8
 *		2		0		4		4
 *		4		0		2		6
 *		6		0		0		8
 */

/*
 * base64_encode_internal
 */
static int base64_encode_internal(const unsigned char *data, size_t len, char *base64, size_t cpl)
{
	size_t n = 0;
	size_t cnt = 0;
	while ((n + 2) < len) {
		*base64++ = base64_bits_to_char(data[0] >> 2);
		cnt++;
		if (cpl && !(cnt % cpl)) {
			*base64++ = '\r';
			*base64++ = '\n';
		}
		*base64++ = base64_bits_to_char(((data[0] & 0x3) << 4) | (data[1] >> 4));
		cnt++;
		if (cpl && !(cnt % cpl)) {
			*base64++ = '\r';
			*base64++ = '\n';
		}
		*base64++ = base64_bits_to_char(((data[1] & 0xf) << 2) | (data[2] >> 6));
		cnt++;
		if (cpl && !(cnt % cpl)) {
			*base64++ = '\r';
			*base64++ = '\n';
		}
		*base64++ = base64_bits_to_char(data[2] & 0x3f);
		cnt++;
		if (((n + 3) != len) && cpl && !(cnt % cpl)) {
			*base64++ = '\r';
			*base64++ = '\n';
		}
		data += 3;
		n += 3;
	}

	/*
	 * The last 2 characters might need to be encoded.
	 */
	if ((n + 1) < len) {
		*base64++ = base64_bits_to_char(data[0] >> 2);
		cnt++;
		if (cpl && !(cnt % cpl)) {
			*base64++ = '\r';
			*base64++ = '\n';
		}
		*base64++ = base64_bits_to_char(((data[0] & 0x3) << 4) | (data[1] >> 4));
		cnt++;
		if (cpl && !(cnt % cpl)) {
			*base64++ = '\r';
			*base64++ = '\n';
		}
		*base64++ = base64_bits_to_char((data[1] & 0xf) << 2);
		cnt++;
		if (cpl && !(cnt % cpl)) {
			*base64++ = '\r';
			*base64++ = '\n';
		}
		*base64++ = '=';
		*base64 = 0;
		return 0;
	}

	/*
	 * Only a single character at the end.
	 */
	if (n < len) {
		*base64++ = base64_bits_to_char(data[0] >> 2);
		cnt++;
		if (cpl && !(cnt % cpl)) {
			*base64++ = '\r';
			*base64++ = '\n';
		}

		*base64++ = base64_bits_to_char((data[0] & 0x3) << 4);
		cnt++;
		if (cpl && !(cnt % cpl)) {
			*base64++ = '\r';
			*base64++ = '\n';
		}

		*base64++ = '=';
		cnt++;
		if (cpl && !(cnt % cpl)) {
			*base64++ = '\r';
			*base64++ = '\n';
		}
		*base64++ = '=';
	}

	/*
	 * terminate the string
	 */
	*base64 = 0;
	return 0;
}
int b64_encode(const unsigned char *data, size_t len, char *s)
{
	return base64_encode_internal(data, len, s, 0);
}