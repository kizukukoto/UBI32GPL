#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "md5.h"
#include "base64.h"

char *share_key_mac = "a5797fcce39d0e942841031a1c85ab8d";


char *get_rand_iv ( int iv_len )
{
    int i = 0;
    char *retval = (char*) malloc(iv_len+1);

    srandom((unsigned int)time(NULL));
    for (i=0; i<iv_len; i++)
    {
	retval[i] = (random() % 255)+1;
    }

    retval[i] = 0;

    return retval;
}


char *XOR (char* dest, int dest_len, char *value, int vlen, char *key, int klen)
{
    char *retval = dest;
    
    short unsigned int k = 0;
    short unsigned int v = 0;

    for (v; v<vlen && v<klen; v++ )
    {
	retval[v] = value[v] ^ key[k];
	k=(++k<vlen?k:0);
    }

    return retval;
}

char *substr (char *dest, int dest_len, char *string, int start, int length )
{
    char *retval = dest;

    memcpy(retval, string+start, dest_len);

    if ( dest_len <= length )
    {
        // dest[dest_len-1] = '\0';
        // printf("dest_len(%d) <= length(%d)\n", dest_len, length);
    }
    else
    {
        dest[length] = '\0';
    }

    return retval;
}

char *packH ( char *dest, int dest_len, char* string ) 
{
    char *result = dest;
    int i = 0;


    for (i=0; i<dest_len-1; i++)
    {
	char hex[16];
	int temp = 0;
	substr(hex, 16, string, i*2, 2);
	sscanf(hex, "%02X", &temp);
	result[i] = temp;
    }

    return result;
}

char *md5_encrypt ( char *plain_text, char *password, int iv_len )
{
    static char result[2048];
    int i = 0, n = 0;
    char iv[512], strEncText[2048], strXOR[512];
    char my_plain_text[2048];
    char *tmp = NULL;
    int strEncText_len = 0;
    int block_iv_len = 32;
    int old_iv_len = 16;

    memset(result, 0, sizeof(result));
    memset(strEncText, 0, sizeof(strEncText));
    memset(my_plain_text, 0, sizeof(my_plain_text));

    strcpy(my_plain_text, plain_text);

    n = strlen(my_plain_text);

    if ( n % 16 )
    {
	int i = 0;

	for (i=0; i< 16 - (n % 16); i++ )
	{
	    my_plain_text[n+i] = ' ';
	}
    }

    tmp = get_rand_iv(iv_len);
    memcpy(strEncText, tmp, iv_len);
    strEncText_len += iv_len;
    free(tmp);

    XOR(strXOR, 512, password, 16, strEncText, 16);
    substr(iv, sizeof(iv), strXOR, 0, 512);

    while ( i < n )
    {
        char block[1024], strSubStr[512], *strXOR = NULL, *strMD5_iv = NULL;
	char pack[64], *block_iv = NULL;
	memset(strSubStr, 0, sizeof(strSubStr));
	substr(strSubStr, 512, my_plain_text, i, 16);

        strMD5_iv = MDString(iv, old_iv_len);
	packH(pack, sizeof(pack), strMD5_iv);
        XOR(block, 16, strSubStr, 16, pack, 16);

        memcpy(strEncText+strEncText_len, block, 16);
	strEncText_len += 16;
	
        block_iv = (char*) malloc(16+32+1);
	memcpy(block_iv, block, 16);
	memcpy(block_iv+16, iv, old_iv_len);
	old_iv_len = 32;

	substr(strSubStr, 512, block_iv, 0, 512);
	XOR(iv, 512, strSubStr, 32, password, 32);

	i += 16;
    }

    base64encode(strEncText, strEncText_len, result, sizeof(result));

    return result;
}

char *md5_decrypt ( char *enc_text, char *password, int iv_len )
{
    static char my_plain_text[2048];
    int i = 0, n = 0;
    char iv[512], base64_dec_text[2048], strXOR[512];
    char base64_enc_substr[1024];
    char base64_enc_block[1024];
    char base64_enc_xor[1024];
    char base64_enc_iv[1024];
    char base64_enc_block_iv[1024];
    char base64_enc_pack[1024];
    char strSubStr[1024];;
    char *tmp = NULL;
    int block_iv_len = 32;
    int old_iv_len = 16;
    
    i = iv_len;
    n = base64decode(enc_text, strlen(enc_text), base64_dec_text, sizeof(base64_dec_text));
    memset(my_plain_text, 0, sizeof(my_plain_text));

    substr(strSubStr, sizeof(strSubStr), base64_dec_text, 0, iv_len);
    base64encode(strSubStr, iv_len, base64_enc_substr, sizeof(base64_enc_substr));

    XOR(strXOR, iv_len, password, 16, strSubStr, sizeof(strSubStr));
    base64encode(strXOR, iv_len, base64_enc_xor, sizeof(base64_enc_xor));

    substr(iv, sizeof(iv), strXOR, 0, 512);
    base64encode(iv, strlen(iv), base64_enc_iv, sizeof(base64_enc_iv));

    i = iv_len;

    while ( i < n )
    {
	char block[1024], strSubStr[512], strXOR[512], *strMD5_iv = NULL;
	char pack[512], block_iv[512];

	memset(strSubStr, 0, sizeof(strSubStr));
	substr(block, sizeof(block), base64_dec_text, i, iv_len);
        base64encode(block, iv_len, base64_enc_block, sizeof(base64_enc_block));

	strMD5_iv = MDString(iv, old_iv_len);
        base64encode(iv, old_iv_len, base64_enc_iv, sizeof(base64_enc_iv));
	old_iv_len = 32;

	packH(pack, sizeof(pack) , strMD5_iv);
        base64encode(pack, iv_len, base64_enc_pack, sizeof(base64_enc_pack));


	XOR(strXOR, sizeof(strXOR), block, sizeof(block), pack, sizeof(pack));
        base64encode(strXOR, iv_len, base64_enc_xor, sizeof(base64_enc_xor));

	strncat(my_plain_text, strXOR, strlen(my_plain_text)+strlen(strXOR)+1);
	
	memcpy(block_iv, block, iv_len);
	memcpy(block_iv+iv_len, iv, iv_len*2);
        base64encode(block_iv, block_iv_len, base64_enc_block_iv, sizeof(base64_enc_block_iv));
	block_iv_len = 48;

	substr(strSubStr, 32, block_iv, 0, 512);
        base64encode(strSubStr, 32, base64_enc_substr, sizeof(base64_enc_substr));
	memset(iv, 0, sizeof(iv));
	XOR(iv, 32, strSubStr, sizeof(strSubStr), password, 32);
        base64encode(iv, old_iv_len, base64_enc_iv, sizeof(base64_enc_iv));

	i += 16;
    }
    
    return my_plain_text;
}


