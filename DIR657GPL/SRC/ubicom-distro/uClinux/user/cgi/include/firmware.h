#ifndef FIRMWARE_H
#define FIRMWARE_H

#define CONFIG_BIN_SIGNATURE	0xaabbccdd

#define MAX_IMAGE		(4*1024*1024)
#define BUF_SIZE		(256)

#ifndef MTD_DRIVER
#define SAVE_FILE		"log.dat"
#define CONFIG_FILE		"default.mib"
#else
#define SAVE_FILE		"/dev/mtd/1"
#define CONFIG_FILE		"/dev/mtd/4"
#endif

#define SIGNUATURE_FILE		"/tmp/sign"

#define IMAGE_SIGNATURE		0xcae00001ul
#define IMAGE_UNKOWN		0xfffffffful

struct __attribute__ ((packed)) signature {
	u_int32_t	version;
	u_int32_t	time;
	u_int32_t	signature;
};

u_int sum_byte(unsigned char* buf, long len);
#endif
