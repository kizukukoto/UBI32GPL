#ifndef __DEF_H__
#define __DEF_H__

struct gnutls_datum{
	unsigned char *data;
	unsigned int size;
};
typedef struct gnutls_datum gnutls_datum_t;
#if 0
typedef short int16_t;  //already defined in /ubicom-distro/uClinux/uClibc/include/stdint.h
typedef unsigned char uint8_t;  //already defined in /ubicom-distro/uClinux/uClibc/include/stdint.h
#endif
//typedef int16_t lockdownd_error_t;
typedef unsigned short lockdownd_error_t;


#define SUCCESS                    0
#define INVALID_ARG               -1
#define LOCKDOWN_E_INVALID_CONF              -2
#define LOCKDOWN_E_PAIRING_FAILED            -4
#define LOCKDOWN_E_SSL_ERROR                 -5
#define LOCKDOWN_E_DICT_ERROR                -6
#define LOCKDOWN_E_START_SERVICE_FAILED      -7
#define LOCKDOWN_E_NOT_ENOUGH_DATA           -8
#define LOCKDOWN_E_SET_VALUE_PROHIBITED      -9
#define LOCKDOWN_E_GET_VALUE_PROHIBITED     -10
#define LOCKDOWN_E_REMOVE_VALUE_PROHIBITED  -11
#define LOCKDOWN_E_MUX_ERROR                -12

#define LOCKDOWN_E_UNKNOWN_ERROR           -256

#define USERPREF_E_SUCCESS             0
#define USERPREF_E_INVALID_ARG        -1
#define USERPREF_E_INVALID_CONF       -2
#define USERPREF_E_SSL_ERROR          -3
#define GNUTLS_E_MEMORY_ERROR 	      -20
#define GNUTLS_E_BASE64_UNEXPECTED_HEADER_ERROR -21
#define GNUTLS_E_BASE64_DECODING_ERROR	-22
#define GNUTLS_E_INVALID_REQUEST -23
#define GNUTLS_E_MEMORY_ERROR 	      -20

#define USERPREF_E_UNKNOWN_ERROR    -256
#define ERROR          -3
#define SUCCESS		0
//typedef int16_t userpref_error_t;
typedef unsigned short userpref_error_t;
#define B64SIZE( data_size) ((data_size%3==0)?((data_size*4)/3):(4+((data_size/3)*4)))

#define THIS_FILE "ref"

#endif //#define __DEF_H__
