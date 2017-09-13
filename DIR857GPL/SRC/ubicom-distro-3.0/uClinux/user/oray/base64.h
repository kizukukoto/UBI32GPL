#ifndef __BASE64_H__
#define __BASE64_H__

int b64_encode(const unsigned char *data, size_t len, char *s);
int b64_decode( const char* str, unsigned char* space, int size );
#endif