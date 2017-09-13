
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <debug.h>
#include <string.h>
#include <sys/stat.h>
#include "def.h"
#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/ossl_typ.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>

extern inline int
cpydata (const uint8_t * data, int data_size, uint8_t ** result);

char root_keyfile[] ="/etc/RootPrivateKey.pem";
char root_certfile[] ="/etc/RootCertificate.pem";
char host_certfile[] ="/etc/HostCertificate.pem";
char fake_certfile[] ="/var/tmp/fakeCert.pem";
extern char tmpfilePath[];

#ifdef DEBUG
#define dbgprintf  printf
#else
#define dbgprintf
#endif

#define ENDSTR "-----\n"
#define ENDSTR2 "-----\r"
int
_gnutls_fbase64_decode (const char *header, const unsigned char * data,
			size_t data_size, uint8_t ** result)
{
  int ret;
  static const char top[] = "-----BEGIN ";
  static const char bottom[] = "\n-----END ";
  uint8_t *rdata;
  int rdata_size;
  uint8_t *kdata;
  int kdata_size;
  char pem_header[128];

  strcpy (pem_header, top);
  if (header != NULL)
    strcat (pem_header, header);


	rdata = memmem(data, data_size, pem_header, strlen (pem_header));

  if (rdata == NULL)
      return GNUTLS_E_BASE64_UNEXPECTED_HEADER_ERROR;


  data_size -= (unsigned long int) rdata - (unsigned long int) data;

  if (data_size < 4 + strlen (bottom))
    {
      return GNUTLS_E_BASE64_DECODING_ERROR;
    }
  kdata = memmem (rdata, data_size, ENDSTR, sizeof (ENDSTR) - 1);
  /* allow CR as well.
   */

  if (kdata == NULL){
	kdata = memmem(rdata, data_size, ENDSTR2, sizeof (ENDSTR2) - 1);
  }	
  if (kdata == NULL)
      return GNUTLS_E_BASE64_DECODING_ERROR;

  data_size -= strlen (ENDSTR);
  data_size -= (unsigned long int) kdata - (unsigned long int) rdata;

  rdata = kdata + strlen (ENDSTR);

  /* position is now after the ---BEGIN--- headers */

	
  kdata = memmem (rdata, data_size, bottom, strlen (bottom));

  if (kdata == NULL)
    {

      return GNUTLS_E_BASE64_DECODING_ERROR;
    }

  /* position of kdata is before the ----END--- footer 
   */
  rdata_size = (unsigned long int) kdata - (unsigned long int) rdata;

  if (rdata_size < 4)
    {
      return GNUTLS_E_BASE64_DECODING_ERROR;
    }

  kdata_size = cpydata (rdata, rdata_size, &kdata);

  if (kdata_size < 0)
    {

      return kdata_size;
    }

  if (kdata_size < 4)
    {
      return GNUTLS_E_BASE64_DECODING_ERROR;
    }

  if ((ret = _gnutls_base64_decode (kdata, kdata_size, result)) < 0)
    {
      return GNUTLS_E_BASE64_DECODING_ERROR;
    }
  free (kdata);

  return ret;
}



int
gnutls_pem_base64_decode_alloc (const char *header,
				const gnutls_datum_t * b64_data,
				gnutls_datum_t * result)
{
  unsigned char *ret;
  int size;

  if (result == NULL)
    return GNUTLS_E_INVALID_REQUEST;

  size =
    _gnutls_fbase64_decode (header, b64_data->data, b64_data->size, &ret);
  if (size < 0)
    return size;

  result->data = ret;
  result->size = size;
  return 0;
}



typedef struct pw_cb_data
	{
	const void *password;
	const char *prompt_info;
	} PW_CB_DATA;
typedef int pem_password_cb(char *buf, int size, int rwflag, void *userdata);

int import_rsa_raw(RSA *rsa, BIGNUM *m_value, BIGNUM *e_value, gnutls_datum_t *d, gnutls_datum_t *p, gnutls_datum_t *q, gnutls_datum_t *u)
{
	int ret = INVALID_ARG;
	/* We need the RSA components non-NULL */
	if(!rsa->n && ((rsa->n=BN_new()) == NULL)) goto err;
	if(!rsa->d && ((rsa->d=BN_new()) == NULL)) goto err;
	if(!rsa->e && ((rsa->e=BN_new()) == NULL)) goto err;
	if(!rsa->p && ((rsa->p=BN_new()) == NULL)) goto err;
	if(!rsa->q && ((rsa->q=BN_new()) == NULL)) goto err;
	if(!rsa->dmp1 && ((rsa->dmp1=BN_new()) == NULL)) goto err;
	if(!rsa->dmq1 && ((rsa->dmq1=BN_new()) == NULL)) goto err;
	if(!rsa->iqmp && ((rsa->iqmp=BN_new()) == NULL)) goto err;
	
	rsa->d = BN_bin2bn(d->data, d->size, rsa->d);
	rsa->p = BN_bin2bn(p->data, p->size, rsa->p);
	rsa->q = BN_bin2bn(q->data, q->size, rsa->q);
	rsa->iqmp = BN_bin2bn(u->data, u->size, rsa->iqmp);	
	BN_copy(rsa->n,m_value);
	BN_copy(rsa->e,e_value);
		


	ret = SUCCESS;
	
err:	
	
	return ret; 
	
}

int add_ext(X509 *cert, int nid, char *value)
	{
	X509_EXTENSION *ex;
	X509V3_CTX ctx;
	/* This sets the 'context' of the extensions. */
	/* No configuration database */
	X509V3_set_ctx_nodb(&ctx);
	/* Issuer and subject certs: both the target since it is self signed,
	 * no request and no CRL
	 */
	X509V3_set_ctx(&ctx, cert, cert, NULL, NULL, 0);
	ex = X509V3_EXT_conf_nid(NULL, &ctx, nid, value);
	if (!ex){
		dbgprintf("ex error \n");
		return 0;
	}
	X509_add_ext(cert,ex,-1);
	X509_EXTENSION_free(ex);
	return 1;
	}

int get_key(EVP_PKEY *root_privkey)
{
		int ret = SUCCESS;

		BIO *root_key=NULL, *host_key = NULL;
		
	root_key = BIO_new_file(root_keyfile, "r");
	if (!root_key){
		dbgprintf("open file %s error\n", root_keyfile);
		ret = ERROR;
		goto end;
	}	
	root_privkey = PEM_read_bio_PrivateKey(root_key, &root_privkey, 0, NULL);
	if (root_privkey == NULL){
		dbgprintf("unable to load root_privkey\n");
		ret = ERROR;		
	}
end:
	if (root_key != NULL) BIO_free(root_key);
	if (root_privkey == NULL){
		dbgprintf("unable to load %s\n", "request key");
		ret = ERROR;
	}	
	return ret;
	
}

int get_b64_cert(char *filename, char **data, int *data_size){
	
	struct stat *filestats = (struct stat *) malloc(sizeof(struct stat));
	FILE *fp=NULL;
	char *tmpBuf=NULL;
	char *tmpBuf1=NULL;
	char *tmpBuf2=NULL;
	int size;
	char head[]="\xa\x09\x09";
	int i, count, ret = -1;
	int counter;
	fp = fopen(filename, "rb");

	if (fp){
		stat(filename, filestats);
		size = filestats->st_size;
		tmpBuf = malloc(size+1);
		fread(tmpBuf, sizeof(char),size, fp);

		_gnutls_base64_encode(tmpBuf, size, &tmpBuf1);		
		//tranfser format
		tmpBuf2 = malloc(B64SIZE(size)*2);
		if (tmpBuf2 == NULL){
			dbgprintf("alloc tmpBuf2 fail\n");
			goto err;
		}
		for(i=0, counter=0; i < B64SIZE(size); i++, counter++){
		     if((tmpBuf1[0] == 0x00) || (((i % 60)==0))){
		     		memcpy(&tmpBuf2[counter], head, sizeof(head)-1);
		     		counter +=(sizeof(head)-1);
		     }		     
		     tmpBuf2[counter] = tmpBuf1[i];	
		}

		*data_size = counter;
		*data = calloc(sizeof(char), counter);
		memcpy(*data, tmpBuf2, counter);

		
		free(tmpBuf2);
		free(tmpBuf1);
		free(tmpBuf);
		free(filestats);
		fclose(fp);		
		ret = 0;				
	}else{
		dbgprintf("open file %s error \n", filename);
		ret = -1;
	}
err:	
	return ret;
}


lockdownd_error_t lockdownd_gen_pair_cert(gnutls_datum_t public_key, gnutls_datum_t * odevice_cert,
									   gnutls_datum_t * ohost_cert, gnutls_datum_t * oroot_cert)
{
	if (!public_key.data || !odevice_cert || !ohost_cert || !oroot_cert)
		return INVALID_ARG;
	lockdownd_error_t ret = LOCKDOWN_E_UNKNOWN_ERROR;
	userpref_error_t uret = USERPREF_E_UNKNOWN_ERROR;


	/* now decode the PEM encoded key */
	gnutls_datum_t der_pub_key;
	int res;

	res = gnutls_pem_base64_decode_alloc("RSA PUBLIC KEY", &public_key, &der_pub_key);

	if( res != SUCCESS) {
		dbgprintf("Error %d!!!\n",res);
		return -1;
	}


	RSA *pub_rsa;

	
	pub_rsa=d2i_RSAPublicKey(NULL,(const unsigned char **)&der_pub_key.data,(long)der_pub_key.size);

	ret = SUCCESS;


	
	if (SUCCESS == ret) {
		
		gnutls_datum_t essentially_null = { (unsigned char*)strdup("abababababababab"), strlen("abababababababab") };
		RSA *fake_rsa_pkey = NULL;
		EVP_PKEY *pkey=NULL, *root_privkey=NULL;//, *host_privkey=NULL;
		X509 *x509_fake=NULL;
		
		if ((pkey = EVP_PKEY_new()) == NULL){
			dbgprintf("init pkey error\n");
			goto err;
		}
		if ((root_privkey = EVP_PKEY_new()) == NULL){
			dbgprintf("init root_privkey error\n");
			goto err;
		}
		
		ret = get_key(root_privkey);				
		if (ret == SUCCESS){
			if ((fake_rsa_pkey = RSA_new()) == NULL){
				dbgprintf("init rsa error\n");
				goto err;
			}		
			if ((x509_fake=X509_new()) == NULL){
				dbgprintf("init x509_fake error\n");
				 goto err;
			}	 
			X509_set_version(x509_fake,2);
			X509_set_serialNumber(x509_fake, 0);
			X509_gmtime_adj(X509_get_notBefore(x509_fake),0);
		        X509_gmtime_adj(X509_get_notAfter(x509_fake),(long)(60 * 60 * 24 * 365 * 10));		
			
			ret = import_rsa_raw(fake_rsa_pkey, pub_rsa->n, pub_rsa->e, &essentially_null, &essentially_null,
					     &essentially_null, &essentially_null); 
			if(ret != SUCCESS){
				dbgprintf("import_rsa_raw error\n");
				goto err;											   	
			}
			if (!(ret = EVP_PKEY_set1_RSA(pkey, fake_rsa_pkey)))
			{
				dbgprintf("EVP_PKEY_set1_RSA error\n");
				goto err;			
			}
			
			if(!(ret = X509_set_pubkey(x509_fake,pkey)))
			{
				dbgprintf("x509 set key fail \n");
				goto err;	
			}
			add_ext(x509_fake, NID_basic_constraints, "critical,CA:FALSE");
			add_ext(x509_fake, NID_subject_key_identifier, "hash");
					
			if (!(ret=X509_sign(x509_fake,root_privkey,EVP_sha1()))){
				goto err;
			}

		}

//			X509_print_fp(stdout,x509_fake);
			/* if everything went well, export in PEM format */
		FILE *fake_cert=NULL;
		fake_cert = fopen(fake_certfile, "wb");
		if (!fake_cert){
			printf("open file %s error\n", fake_certfile);
			ret = ERROR;
			goto err;
		}
		PEM_write_X509(fake_cert,x509_fake);
		fclose(fake_cert);
		X509_free(x509_fake);					
	}	
		
	get_b64_cert(fake_certfile, &odevice_cert->data, &odevice_cert->size);
	get_b64_cert(host_certfile, &ohost_cert->data, &ohost_cert->size);
	get_b64_cert(root_certfile, &oroot_cert->data, &oroot_cert->size);
	


		
	RSA_free(pub_rsa);		
err:
	return ret;
}



