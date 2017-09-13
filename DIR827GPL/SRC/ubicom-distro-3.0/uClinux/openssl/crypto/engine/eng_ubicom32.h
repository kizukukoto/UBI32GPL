#include <stdint.h>

asm(".include   \"ip5000.inc\"");

#define likely(x) x
#define unlikely(x) x

#define SHA1_H0            0x67452301UL
#define SHA1_H1            0xefcdab89UL
#define SHA1_H2            0x98badcfeUL
#define SHA1_H3            0x10325476UL
#define SHA1_H4            0xc3d2e1f0UL

#define SEC_ALIGNED(x) (((unsigned int)x) & 0x3)

/* copies from ip5000.h */
#define OCP_BASE 0x01000000
#define SEC_BASE (OCP_BASE + 0x400)
#define GEN_CLK_PLL_SECURITY_BIT_NO 31

#define SECURITY_CTRL_KEY_SIZE(x) ((x) << 8)
#define SECURITY_CTRL_HASH_ALG_NONE (0 << 4)
#define SECURITY_CTRL_HASH_ALG_MD5 (1 << 4)
#define SECURITY_CTRL_HASH_ALG_SHA1 (2 << 4)
#define SECURITY_CTRL_CBC (1 << 3)
#define SECURITY_CTRL_CIPHER_ALG_AES (0 << 1)
#define SECURITY_CTRL_CIPHER_ALG_NONE (1 << 1)
#define SECURITY_CTRL_CIPHER_ALG_DES (2 << 1)
#define SECURITY_CTRL_CIPHER_ALG_3DES (3 << 1)
#define SECURITY_CTRL_ENCIPHER (1 << 0)
#define SECURITY_CTRL_DECIPHER (0 << 0)


#define SEC_KEY_128_BITS        SECURITY_CTRL_KEY_SIZE(0)
#define SEC_KEY_192_BITS        SECURITY_CTRL_KEY_SIZE(1)
#define SEC_KEY_256_BITS        SECURITY_CTRL_KEY_SIZE(2)

#define SEC_HASH_NONE           SECURITY_CTRL_HASH_ALG_NONE
#define SEC_HASH_MD5            SECURITY_CTRL_HASH_ALG_MD5
#define SEC_HASH_SHA1           SECURITY_CTRL_HASH_ALG_SHA1

#define SEC_CBC_SET             SECURITY_CTRL_CBC
#define SEC_CBC_NONE            0

#define SEC_ALG_AES             SECURITY_CTRL_CIPHER_ALG_AES
#define SEC_ALG_NONE            SECURITY_CTRL_CIPHER_ALG_NONE
#define SEC_ALG_DES             SECURITY_CTRL_CIPHER_ALG_DES
#define SEC_ALG_3DES            SECURITY_CTRL_CIPHER_ALG_3DES

#define SEC_DIR_ENCRYPT         SECURITY_CTRL_ENCIPHER
#define SEC_DIR_DECRYPT         0

#define UBICOM_CHUNK 512
#define SHA1_BLOCK_SIZE 64
#define AES_BLOCK_SIZE 16
#define AES_MAX_KEY_SIZE 32

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

extern u8 crypto_ubicom32_on;

#define SEC_COPY_2W(t, s) \
        asm volatile ( \
        "       move.4 0(%0), 0(%1)             \n\t"   \
        "       move.4 4(%0), 4(%1)             \n\t"   \
        :                                               \
        : "a"(t), "a"(s)                                \
        )

#define SEC_COPY_4W(t, s) \
        asm volatile ( \
        "       move.4 0(%0), 0(%1)             \n\t"   \
        "       move.4 4(%0), 4(%1)             \n\t"   \
        "       move.4 8(%0), 8(%1)             \n\t"   \
        "       move.4 12(%0), 12(%1)           \n\t"   \
        :                                               \
        : "a"(t), "a"(s)                                \
        )

#define SEC_COPY_5W(t, s) \
        asm volatile ( \
        "       move.4 0(%0), 0(%1)             \n\t"   \
        "       move.4 4(%0), 4(%1)             \n\t"   \
        "       move.4 8(%0), 8(%1)             \n\t"   \
        "       move.4 12(%0), 12(%1)           \n\t"   \
        "       move.4 16(%0), 16(%1)           \n\t"   \
        :                                               \
        : "a"(t), "a"(s)                                \
        )

static inline void hw_crypto_set_ctrl(uint32_t c)
{
        asm volatile (                                  
        "       move.4 0(%0), %1                \n\t"   
        :                                       
        : "a"(SEC_BASE), "d"(c)    
        );
}
#define ubicom32_eng_set_ctrl(c) hw_crypto_set_ctrl(c)

static inline void hw_crypto_turn_on(void)
{
        asm volatile (                                  
        "       moveai  A4, %0                    \n\t" 
        "       bset    0x0(A4), 0x0(A4), %1      \n\t"
        "       cycles 11                         \n\t" 
        :                                               
        : "i"(OCP_BASE >> 7), "i"(GEN_CLK_PLL_SECURITY_BIT_NO) 
        : "a4"
        );
        crypto_ubicom32_on = 1;
}

extern pid_t mypid;
extern int myfid;
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>

#define S_SYSCALL 1

static inline void ubicom32_eng_lock(void)
{
#ifdef S_SYSCALL
	mypid = getpid();
#else
	int retfd;
	ioctl(myfid, CRIOGET, &retfd);
#endif
	
}

static inline void ubicom32_eng_unlock(void)
{
#ifdef S_SYSCALL
	mypid = getpid();
#else
	int retfd;
	ioctl(myfid, CRIOGET, &retfd);
#endif
}

static inline void ubicom32_eng_check(void)
{
	// start PLL if not done
	if (crypto_ubicom32_on) {
		return;
	}
	hw_crypto_turn_on();
}


static inline void ubicom32_aes_cbc_cipher_loop(unsigned char *out, const unsigned char *in, unsigned char *iv, size_t n, int enc)
{
        asm volatile (
        "; set init. iv 4w                      \n\t"
        "       move.4 0x50(%0), 0x0(%3)        \n\t"
        "       move.4 0x54(%0), 0x4(%3)        \n\t"
        "       move.4 0x58(%0), 0x8(%3)        \n\t"
        "       move.4 0x5c(%0), 0xc(%3)        \n\t"
        "                                       \n\t"
        "; we know n > 0, so we can always      \n\t"
        "; load the first block                 \n\t"
        "; set input 4w                         \n\t"
        "       move.4 0x30(%0), 0x0(%2)        \n\t"
        "       move.4 0x34(%0), 0x4(%2)        \n\t"
        "       move.4 0x38(%0), 0x8(%2)        \n\t"
        "       move.4 0x3c(%0), 0xc(%2)        \n\t"
        "                                       \n\t"
        "; kickoff hw                           \n\t"
        "       move.4 0x40(%0), %2             \n\t"
        "                                       \n\t"
        "; update n & flush                     \n\t"
        "       add.4 %4, #-16, %4              \n\t"
        "       pipe_flush 0                    \n\t"
        "                                       \n\t"
        "; while (n):  work on 2nd block        \n\t"
        " 1:    lsl.4 d15, %4, #0x0             \n\t"
        "       jmpeq.f 5f                      \n\t"
        "                                       \n\t"
        "; set input 4w (2nd)                   \n\t"
        "       move.4 0x30(%0), 0x10(%2)       \n\t"
        "       move.4 0x34(%0), 0x14(%2)       \n\t"
        "       move.4 0x38(%0), 0x18(%2)       \n\t"
        "       move.4 0x3c(%0), 0x1c(%2)       \n\t"
        "                                       \n\t"
        "; update n/in asap while waiting       \n\t"
        "       add.4 %4, #-16, %4              \n\t"
        "       move.4 d15, 16(%2)++            \n\t"
        "                                       \n\t"
        "; wait for the previous output         \n\t"
        "       btst 0x04(%0), #0               \n\t"
        "       jmpne.f -4                      \n\t"
        "                                       \n\t"
        "; read previous output                 \n\t"
        "       move.4 0x0(%1), 0x50(%0)        \n\t"
        "       move.4 0x4(%1), 0x54(%0)        \n\t"
        "       move.4 0x8(%1), 0x58(%0)        \n\t"
        "       move.4 0xc(%1), 0x5c(%0)        \n\t"
        "                                       \n\t"
        "; kick off hw for 2nd input            \n\t"
        "       move.4 0x40(%0), %2             \n\t"
        "                                       \n\t"
        "; update out asap                      \n\t"
        "       move.4 d15, 16(%1)++            \n\t"
        "                                       \n\t"
        "; go back to loop                      \n\t"
        "       jmpt 1b                         \n\t"
        "                                       \n\t"
        "; wait for last output                 \n\t" 
        " 5:    btst 0x04(%0), #0               \n\t"
        "       jmpne.f -4                      \n\t"
        "                                       \n\t"
        "; read last output                     \n\t"
        "       move.4 0x0(%1), 0x50(%0)        \n\t"
        "       move.4 0x4(%1), 0x54(%0)        \n\t"
        "       move.4 0x8(%1), 0x58(%0)        \n\t"
        "       move.4 0xc(%1), 0x5c(%0)        \n\t"
        "                                       \n\t"
        :
        : "a"(SEC_BASE), "a"(out), "a"(in), "a"(iv), "d"(n)
        : "d15"
        );

	/* only need to copy new iv back in the case of encryption */
	if (!enc) {
		return;
	}
	asm volatile (
        "; copy out iv                          \n\t"
        "       move.4 0x0(%1), 0x50(%0)        \n\t"
        "       move.4 0x4(%1), 0x54(%0)        \n\t"
        "       move.4 0x8(%1), 0x58(%0)        \n\t"
        "       move.4 0xc(%1), 0x5c(%0)        \n\t"
        "                                       \n\t"
        :
        : "a"(SEC_BASE), "a"(iv)
	);
}

#define HASH_SECURITY_BLOCK_CONTROL_INIT_NO_ENCYPTION 2
#define HASH_SECURITY_BLOCK_CONTROL_INIT_SHA1 ((1 << 5) | HASH_SECURITY_BLOCK_CONTROL_INIT_NO_ENCYPTION)

struct ubicom32_sha1_ctx {
	u64 count;		/* message length */
	u32 state[5];
	u8 buf[2 * SHA1_BLOCK_SIZE];
};

struct ubicom32_aes_ctx {
        u8 key[AES_MAX_KEY_SIZE];
        u32 ctrl;
        int key_len;
};

static inline void sha1_clear_2ws(u8 *buf, int wc)
{
	asm volatile (
	"1:	move.4 (%0)4++, #0		\n\t"
	"	move.4 (%0)4++, #0		\n\t"
	"	sub.4 %1, #2, %1		\n\t"
	"	jmple.f 1b			\n\t"
	:
	: "a"(buf), "d"(wc)
	);
}

/* only wipe out count, state, and 1st half of buf - 9 bytes at most */
#define sha1_wipe_out(sctx) sha1_clear_2ws((u8 *)sctx, 2 + 5 + 16 - 2)

static inline void sha1_init_digest(u32 *digest)
{
	hw_crypto_set_ctrl(HASH_SECURITY_BLOCK_CONTROL_INIT_SHA1);
	asm volatile (
	"	; move digests to hash_output regs	\n\t"
	"	move.4 0x70(%0), 0x0(%1)		\n\t"
	"	move.4 0x74(%0), 0x4(%1)		\n\t"
	"	move.4 0x78(%0), 0x8(%1)		\n\t"
	"	move.4 0x7c(%0), 0xc(%1)		\n\t"
	"	move.4 0x80(%0), 0x10(%1)		\n\t"
	:
	: "a"(SEC_BASE), "a"(digest)
	);
}

static inline void sha1_transform_feed(const u8 *in)
{
	asm volatile (
	"	; write the 1st 16 bytes	\n\t"
	"	move.4 0x30(%0), 0x0(%1)	\n\t"
	"	move.4 0x34(%0), 0x4(%1)	\n\t"
	"	move.4 0x38(%0), 0x8(%1)	\n\t"
	"	move.4 0x3c(%0), 0xc(%1)	\n\t"
	"	move.4 0x40(%0), %1		\n\t"
	"	; write the 2nd 16 bytes	\n\t"
	"	move.4 0x30(%0), 0x10(%1)	\n\t"
	"	move.4 0x34(%0), 0x14(%1)	\n\t"
	"	move.4 0x38(%0), 0x18(%1)	\n\t"
	"	move.4 0x3c(%0), 0x1c(%1)	\n\t"
	"	move.4 0x40(%0), %1		\n\t"
	"	; write the 3rd 16 bytes	\n\t"
	"	move.4 0x30(%0), 0x20(%1)	\n\t"
	"	move.4 0x34(%0), 0x24(%1)	\n\t"
	"	move.4 0x38(%0), 0x28(%1)	\n\t"
	"	move.4 0x3c(%0), 0x2c(%1)	\n\t"
	"	move.4 0x40(%0), %1		\n\t"
	"	; write the 4th 16 bytes	\n\t"
	"	move.4 0x30(%0), 0x30(%1)	\n\t"
	"	move.4 0x34(%0), 0x34(%1)	\n\t"
	"	move.4 0x38(%0), 0x38(%1)	\n\t"
	"	move.4 0x3c(%0), 0x3c(%1)	\n\t"
	"	move.4 0x40(%0), %1		\n\t"
	"	pipe_flush 0			\n\t" 
	:
	: "a"(SEC_BASE), "a"(in)
	);
}

static inline void sha1_transform_wait(void)
{
	asm volatile (
	"	btst 0x04(%0), #0		\n\t"
	"	jmpne.f -4			\n\t"
	:
	: "a"(SEC_BASE)
	);
}

static inline void sha1_output_digest(u32 *digest)
{
	asm volatile (
	"	move.4 0x0(%1), 0x70(%0)		\n\t"
	"	move.4 0x4(%1), 0x74(%0)		\n\t"
	"	move.4 0x8(%1), 0x78(%0)		\n\t"
	"	move.4 0xc(%1), 0x7c(%0)		\n\t"
	"	move.4 0x10(%1), 0x80(%0)		\n\t"
	: 
	: "a"(SEC_BASE), "a"(digest)
	);
}

static inline void hex_dump(const char *msg, const void *buf, int b_size)
{
#ifdef DEBUG_DUMP
        u8 *b = (u8 *)buf;
        int i;
        if (msg) {
                printf("%s:\t", msg);
        }

        for (i=0; i < b_size; i++) {
                printf("%02x ", b[i]);
                if ((i & 3) == 3) {
                        printf(" ");
                }
                if ((i & 31) == 31) {
                        printf("\n");
                }
        }
        printf("\n");
#endif
}

#ifdef DEBUG_DUMP
#define myprintf printf
#else
#define myprintf(...)
#endif

