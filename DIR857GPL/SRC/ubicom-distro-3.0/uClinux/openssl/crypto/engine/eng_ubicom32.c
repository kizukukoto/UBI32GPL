/* crypto/engine/eng_openssl.c */
/* Written by Geoff Thorpe (geoff@geoffthorpe.net) for the OpenSSL
 * project 2000.
 */
/* ====================================================================
 * Copyright (c) 1999-2001 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.OpenSSL.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    licensing@OpenSSL.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.OpenSSL.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */
/* ====================================================================
 * Copyright 2002 Sun Microsystems, Inc. ALL RIGHTS RESERVED.
 * ECDH support in OpenSSL originally developed by 
 * SUN MICROSYSTEMS, INC., and contributed to the OpenSSL project.
 */


#include <stdio.h>
#include <openssl/crypto.h>
#include "cryptlib.h"
#include <openssl/engine.h>
#include <openssl/dso.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include "eng_ubicom32.h"

u8 crypto_ubicom32_on = 0;

pid_t mypid;
int myfid;

static int openssl_ciphers(ENGINE *e, const EVP_CIPHER **cipher,
				const int **nids, int nid);
static int openssl_digests(ENGINE *e, const EVP_MD **digest,
				const int **nids, int nid);

/* The constants used when creating the ENGINE */
static const char *engine_openssl_id = "ubicom32";
static const char *engine_openssl_name = "Ubicom32 hardware engine support";

/* This internal function is used by ENGINE_openssl() and possibly by the
 * "dynamic" ENGINE support too */
static int bind_helper(ENGINE *e)
	{
	if(!ENGINE_set_id(e, engine_openssl_id)
			|| !ENGINE_set_name(e, engine_openssl_name)
			|| !ENGINE_set_ciphers(e, openssl_ciphers)
			|| !ENGINE_set_digests(e, openssl_digests)
			)
		return 0;
	/* If we add errors to this ENGINE, ensure the error handling is setup here */
	myfid = open("/dev/crypto", O_RDWR, 0);
	/* openssl_load_error_strings(); */
	return 1;
	}

static ENGINE *engine_openssl(void)
	{
	ENGINE *ret = ENGINE_new();
	if(!ret)
		return NULL;
	if(!bind_helper(ret))
		{
		ENGINE_free(ret);
		return NULL;
		}
	return ret;
	}

void ENGINE_load_ubicom32(void)
	{
	ENGINE *toadd = engine_openssl();
	if(!toadd) return;
	ENGINE_add(toadd);
	/* If the "add" worked, it gets a structural reference. So either way,
	 * we release our just-created reference. */
	ENGINE_free(toadd);
	ERR_clear_error();
	}

static int ubicom32_cipher_nids[] = {
#if 0
	NID_aes_128_ecb,
	NID_aes_128_cbc,

	NID_aes_192_ecb,
	NID_aes_192_cbc,

	NID_aes_256_ecb,
#endif
	NID_aes_256_cbc,
};

static int
ubicom32_aes_init_key (EVP_CIPHER_CTX *ctx, const unsigned char *key,
		      const unsigned char *iv, int enc)
{
	struct ubicom32_aes_ctx *uctx = ctx->cipher_data;

	uctx->key_len = ctx->key_len;
	memcpy(uctx->key, key, ctx->key_len);

	switch (ctx->key_len) {
	case 16:	// 128 bits
		uctx->ctrl = SEC_KEY_128_BITS | SEC_ALG_AES;
		break;

        case 24:
                uctx->ctrl = SEC_KEY_192_BITS | SEC_ALG_AES;
                break;

        case 32:
                uctx->ctrl = SEC_KEY_256_BITS | SEC_ALG_AES;
                break;

	default:
		OPENSSL_assert(0);
		break;
        }
	
	if (enc) {
		uctx->ctrl |= SEC_DIR_ENCRYPT;
	}

	return 1;
}

#define SECURITY_BASE SEC_BASE

#define SEC_SET_KEY_8W(x) \
        asm volatile ( \
        "       ; write key to Security Keyblock  \n\t" \
        "       move.4 0x10(%0), 0(%1)            \n\t" \
        "       move.4 0x14(%0), 4(%1)            \n\t" \
        "       move.4 0x18(%0), 8(%1)            \n\t" \
        "       move.4 0x1c(%0), 12(%1)           \n\t" \
        "       move.4 0x20(%0), 16(%1)           \n\t" \
        "       move.4 0x24(%0), 20(%1)           \n\t" \
        "       move.4 0x28(%0), 24(%1)           \n\t" \
        "       move.4 0x2c(%0), 28(%1)           \n\t" \
        :                                               \
        :  "a"(SECURITY_BASE), "a"(x)                   \
        )
#define SEC_SET_KEY_256(k)      SEC_SET_KEY_8W(k)
#define SEC_SET_IV_4W(x) \
        asm volatile ( \
        "       ; write IV to Security Keyblock  \n\t" \
        "       move.4 0x50(%0), 0(%1)            \n\t" \
        "       move.4 0x54(%0), 4(%1)            \n\t" \
        "       move.4 0x58(%0), 8(%1)            \n\t" \
        "       move.4 0x5c(%0), 12(%1)           \n\t" \
        :                                               \
        :  "a"(SECURITY_BASE), "a"(x)                   \
        )

#define SEC_SET_INPUT_4W(x) \
        asm volatile ( \
        "       ; write key to Security Keyblock  \n\t" \
        "       move.4 0x30(%0), 0(%1)            \n\t" \
        "       move.4 0x34(%0), 4(%1)            \n\t" \
        "       move.4 0x38(%0), 8(%1)            \n\t" \
        "       move.4 0x3c(%0), 12(%1)           \n\t" \
        :                                               \
        :  "a"(SECURITY_BASE), "a"(x)                   \
        )

#define SEC_GET_OUTPUT_4W(x) \
        asm volatile ( \
        "       ; read output from Security Keyblock  \n\t" \
        "       move.4 0(%1), 0x50(%0)            \n\t" \
        "       move.4 4(%1), 0x54(%0)            \n\t" \
        "       move.4 8(%1), 0x58(%0)            \n\t" \
        "       move.4 12(%1), 0x5c(%0)           \n\t" \
        :                                               \
        :  "a"(SECURITY_BASE), "a"(x)                   \
        )


inline void aes_hw_set_key(const u8 *key, u8 key_len)
{
        /*
 	 * switch case has more overhead than 4 move.4 instructions, so just copy 256 bits
 	 */
        SEC_SET_KEY_256(key);
}

static inline void aes_hw_set_iv(const u8 *iv)
{
        SEC_SET_IV_4W(iv);
}

static inline void aes_hw_cipher(u8 *out, const u8 *in)
{
        SEC_SET_INPUT_4W(in);

        asm volatile (                                                
        "       ; start AES by writing 0x40(SECURITY_BASE)      \n\t" 
        "       move.4 0x40(%0), #0x01                          \n\t" 
        "       pipe_flush 0                                    \n\t" 
        "                                                       \n\t" 
        "       ; wait for the module to calculate the output   \n\t" 
        "       btst 0x04(%0), #0                               \n\t" 
        "       jmpne.f .-4                                     \n\t" 
        :                                                             
        : "a"(SEC_BASE)                                               
        );
        
        SEC_GET_OUTPUT_4W(out);
}


static void cbc_aes_encrypt_loop(unsigned char *out, const unsigned char *in, unsigned char *iv, unsigned int n)
{
        while (likely(n)) {
                aes_hw_set_iv(iv);
                aes_hw_cipher(out, in);
                SEC_COPY_4W(iv, out);
                out += AES_BLOCK_SIZE;
                in += AES_BLOCK_SIZE;
                n -= AES_BLOCK_SIZE;    
        }
}

static void cbc_aes_decrypt_loop(unsigned char *out, const unsigned char *in, unsigned char *iv, unsigned int n)
{
        while (likely(n)) {
        	aes_hw_set_iv(iv);
		SEC_COPY_4W(iv, in);
                aes_hw_cipher(out, in);
                out += AES_BLOCK_SIZE;
                in += AES_BLOCK_SIZE;
                n -= AES_BLOCK_SIZE;
        }
}

static int
ubicom32_aes_cbc_cipher(EVP_CIPHER_CTX *ctx, unsigned char *out_arg,
		   const unsigned char *in_arg, unsigned int nbytes)
{
	struct ubicom32_aes_ctx *uctx = ctx->cipher_data;
	const  void *inp;
	unsigned char  *out;
	int    inp_misaligned, out_misaligned;

	if (nbytes == 0) {
		return 1;
	}
	if (nbytes & 0xF) {
		fprintf(stderr, "nbytes not block size = %u\n", nbytes);
	}

	/* nbytes is always multiple of AES_BLOCK_SIZE in ECB and CBC
	 * modes and arbitrary value in byte-oriented modes, such as
 	 * CFB and OFB... 
 	 */

	/*
 	 * follow padlock analysis on cache usage later
 	 */
	inp_misaligned = (((size_t)in_arg) & 0x03);
	out_misaligned = (((size_t)out_arg) & 0x03);

	if (out_misaligned) {
		myprintf("output misaligned\n");
		out = alloca(nbytes);
		if (!out) {
			fprintf(stderr, "can not alloc out for %s\n", __func__);
			return 0;
		}
	}
	else
		out = out_arg;

	if (inp_misaligned) {
		memcpy(out, in_arg, nbytes);
		inp = out;
	} else
		inp = in_arg;

	ubicom32_eng_lock();
	ubicom32_eng_check();

	ubicom32_eng_set_ctrl(uctx->ctrl | SEC_CBC_SET);
	aes_hw_set_key(uctx->key, uctx->key_len);

#if 1	
	if (ctx->encrypt) {
		myprintf("before encrypt:\n");
		hex_dump("out:", out, nbytes);
		hex_dump("in:", inp, nbytes);
		cbc_aes_encrypt_loop(out, inp, ctx->iv, nbytes);
	} else {
		myprintf("before decrypt:\n");
		hex_dump("out:", out, nbytes);
		hex_dump("in:", inp, nbytes);
		cbc_aes_decrypt_loop(out, inp, ctx->iv, nbytes);
	}
#else
	ubicom32_aes_cbc_cipher_loop(out, inp, ctx->iv, nbytes, ctx->encrypt);
#endif

	ubicom32_eng_unlock();

	/* clean the reallign buffer if it was used */
	if (out_misaligned) {
		size_t blocks = nbytes >> 4;
		unsigned long *p = (unsigned long *)out;
		memcpy(out_arg, out, nbytes);

		while (likely(blocks--)) {
			*p++ = 0;
			*p++ = 0;
			*p++ = 0;
			*p++ = 0;
		}
		free(out);
	}
	return 1;
}

const EVP_CIPHER ubicom32_aes_256_cbc = {
        NID_aes_256_cbc,
        16, 32, 16,
        EVP_CIPH_CBC_MODE,
        ubicom32_aes_init_key,
        ubicom32_aes_cbc_cipher,
        NULL,
        sizeof(struct ubicom32_aes_ctx),
        EVP_CIPHER_set_asn1_iv,
        EVP_CIPHER_get_asn1_iv,
        NULL
};

static int openssl_ciphers(ENGINE *e, const EVP_CIPHER **cipher,
			const int **nids, int nid)
	{
	if(!cipher)
		{
		/* We are returning a list of supported nids */
		*nids = ubicom32_cipher_nids;
		return sizeof(ubicom32_cipher_nids) / sizeof (int);
		}
	/* We are being asked for a specific cipher */
	if(nid == NID_aes_256_cbc)
		*cipher = &ubicom32_aes_256_cbc;
	else
		{
		*cipher = NULL;
		return 0;
		}
	return 1;
	}

/* Much the same sort of comment as for TEST_ENG_OPENSSL_RC4 */
#include <openssl/sha.h>

#define HASH_SECURITY_BLOCK_CONTROL_INIT_NO_ENCYPTION 2
#define HASH_SECURITY_BLOCK_CONTROL_INIT_SHA1 ((1 << 5) | HASH_SECURITY_BLOCK_CONTROL_INIT_NO_ENCYPTION)

static int ubicom32_sha1_init(EVP_MD_CTX *ctx)
{
        struct ubicom32_sha1_ctx *sctx = ctx->md_data;
        sctx->state[0] = SHA1_H0;
        sctx->state[1] = SHA1_H1;
        sctx->state[2] = SHA1_H2;
        sctx->state[3] = SHA1_H3;
        sctx->state[4] = SHA1_H4;
        sctx->count = 0;
	return 1;
}

static int ubicom32_sha1_update(EVP_MD_CTX *ctx,const void *data,size_t len)
{
        struct ubicom32_sha1_ctx *sctx = ctx->md_data;
	int index, clen;

	/* how much is already in the buffer? */
	index = sctx->count & 0x3f;

	myprintf("%s: data=%p, len=%u\n", __func__, data, len);
	hex_dump("data:", data, len);
	hex_dump("buf:", sctx->buf, index);

	sctx->count += len;

	if (index + len < SHA1_BLOCK_SIZE) {
		goto store_only;
	}

	ubicom32_eng_lock();
	ubicom32_eng_check();

	/* init digest set ctrl register too */
	sha1_init_digest(sctx->state);

	if (unlikely(index == 0 && SEC_ALIGNED(data))) {
fast_process:
		myprintf("here1\n");
		while (likely(len >= SHA1_BLOCK_SIZE)) {
			sha1_transform_feed(data);
			data += SHA1_BLOCK_SIZE;
			len -= SHA1_BLOCK_SIZE;
			sha1_transform_wait();
		}
		goto store;
	}

	/* process one stored block */
	if (index) {

		myprintf("here2\n");
		clen = SHA1_BLOCK_SIZE - index;
		memcpy(sctx->buf + index, data, clen);
		sha1_transform_feed(sctx->buf);
		data += clen;
		len -= clen;
		index = 0;
		sha1_transform_wait();
	}

	if (likely(SEC_ALIGNED(data))) {
		goto fast_process;
	}

	/* process as many blocks as possible */
	if (likely(len >= SHA1_BLOCK_SIZE)) {
		memcpy(sctx->buf, data, SHA1_BLOCK_SIZE);
		do {
			sha1_transform_feed(sctx->buf);
			data += SHA1_BLOCK_SIZE;
			len -= SHA1_BLOCK_SIZE;
			if (likely(len >= SHA1_BLOCK_SIZE)) {
				memcpy(sctx->buf, data, SHA1_BLOCK_SIZE);
				sha1_transform_wait();
				continue;
			}
			/* it is the last block */	
			sha1_transform_wait();
			break;
		} while (1);
		myprintf("here3\n");
	}

store:
	sha1_output_digest(sctx->state);
	ubicom32_eng_unlock();

		myprintf("here4\n");
store_only:
	/* anything left? */
	if (len)
		memcpy(sctx->buf + index , data, len);

		myprintf("here5\n");
	return 1;
}

/* Add padding and return the message digest. */
static int ubicom32_sha1_final(EVP_MD_CTX *ctx,unsigned char *out)
{
	struct ubicom32_sha1_ctx *sctx = ctx->md_data;
	u64 bits;
	unsigned int index, end;

		myprintf("here6\n");
	/* must perform manual padding */
	index = sctx->count & 0x3f;
	end =  (index < 56) ? SHA1_BLOCK_SIZE : (2 * SHA1_BLOCK_SIZE);

	/* start pad with 1 */
	sctx->buf[index] = 0x80;

	/* pad with zeros */
	index++;
	memset(sctx->buf + index, 0x00, end - index - 8);

		myprintf("here7\n");
	/* append message length */
	bits = sctx->count << 3 ;
	SEC_COPY_2W(sctx->buf + end - 8, &bits);

	/* force to use the sctx->buf and ignore the partial buf */
	sctx->count = sctx->count & ~0x3f;
	ubicom32_sha1_update(ctx, sctx->buf, end);

	/* copy digest to out */
	SEC_COPY_5W(out, sctx->state);

		myprintf("here8\n");
	/* wipe context */
	sha1_wipe_out(sctx);

	return 1;
}

static int ubicom32_digest_nids[] = {NID_sha1};
static int ubicom32_digest_nids_number = 1;

static const EVP_MD ubicom32_sha_md=
	{
	NID_sha1,
	NID_undef,
	SHA_DIGEST_LENGTH,
	0,
	ubicom32_sha1_init,
	ubicom32_sha1_update,
	ubicom32_sha1_final,
	NULL,
	NULL,
	EVP_PKEY_NULL_method,
	SHA_CBLOCK,
	sizeof(EVP_MD *) + sizeof(struct ubicom32_sha1_ctx),
	};

static int openssl_digests(ENGINE *e, const EVP_MD **digest,
			const int **nids, int nid)
	{
	if(!digest)
		{
		/* We are returning a list of supported nids */
		*nids = ubicom32_digest_nids;
		return ubicom32_digest_nids_number;
		}
	/* We are being asked for a specific digest */
	if(nid == NID_sha1)
		*digest = &ubicom32_sha_md;
	else
		{
		*digest = NULL;
		return 0;
		}
	return 1;
	}
