/*
 * NVRAM variable manipulation
 *
 */

#ifndef _bcmnvram_h_
#define _bcmnvram_h_

#ifndef _LANGUAGE_ASSEMBLY

#include "typedefs.h"

#ifdef  __cplusplus
extern "C" {
#endif
#define __USE_ATH_NVRAM_COMPATIBLE
#ifdef __USE_ATH_NVRAM_COMPATIBLE
#include "nvram_compatible.h"
#endif //USE_ATH_NVRAM_COMPATIBLE
#include <linux/ioctl.h>

#define NVRAM_IOC_MAGIC  'K'
#define NVRAM_IOCGSIZE    _IOR(NVRAM_IOC_MAGIC, 1, int)

struct nvram_header {
	uint32 magic;
	uint32 len;
	uint32 crc_ver_init;	/* 0:7 crc, 8:15 ver, 16:31 sdram_init */
	uint32 config_refresh;	/* 0:15 sdram_config, 16:31 sdram_refresh */
	uint32 config_ncdl;	/* ncdl values for memc */
};

struct nvram_tuple {
	char *name;
	char *value;
	struct nvram_tuple *next;
};

/*
 * Initialize NVRAM access. May be unnecessary or undefined on certain
 * platforms.
 */
extern int BCMINIT(nvram_init)(void *sbh);

/*
 * Disable NVRAM access. May be unnecessary or undefined on certain
 * platforms.
 */
extern void BCMINIT(nvram_exit)(void *sbh);

/*
 * Get the value of an NVRAM variable. The pointer returned may be
 * invalid after a set.
 * @param	name	name of variable to get
 * @return	value of variable or NULL if undefined
 */
extern char * BCMINIT(nvram_get)(const char *name);

/* 
 * Read the reset GPIO value from the nvram and set the GPIO
 * as input
 */
extern int BCMINITFN(nvram_resetgpio_init)(void *sbh);

/* 
 * Get the value of an NVRAM variable.
 * @param	name	name of variable to get
 * @return	value of variable or NUL if undefined
 */
#define nvram_safe_get(name) (BCMINIT(nvram_get)(name) ? : "")

/*
 * Match an NVRAM variable.
 * @param	name	name of variable to match
 * @param	match	value to compare against value of variable
 * @return	TRUE if variable is defined and its value is string equal
 *		to match or FALSE otherwise
 */
static INLINE int
nvram_match(char *name, char *match) {
	const char *value = BCMINIT(nvram_get)(name);
#ifdef __USE_ATH_NVRAM_COMPATIBLE
	return value ? strcmp(value, match) : 1;
#else
	return (value && !strcmp(value, match));
#endif
}

/*
 * Inversely match an NVRAM variable.
 * @param	name	name of variable to match
 * @param	match	value to compare against value of variable
 * @return	TRUE if variable is defined and its value is not string
 *		equal to invmatch or FALSE otherwise
 */
static INLINE int
nvram_invmatch(char *name, char *invmatch) {
	const char *value = BCMINIT(nvram_get)(name);
	return (value && strcmp(value, invmatch));
}

/*
 * Set the value of an NVRAM variable. The name and value strings are
 * copied into private storage. Pointers to previously set values
 * may become invalid. The new value may be immediately
 * retrieved but will not be permanently stored until a commit.
 * @param	name	name of variable to set
 * @param	value	value of variable
 * @return	0 on success and errno on failure
 */
extern int BCMINIT(nvram_set)(const char *name, const char *value);

/*
 * Unset an NVRAM variable. Pointers to previously set values
 * remain valid until a set.
 * @param	name	name of variable to unset
 * @return	0 on success and errno on failure
 * NOTE: use nvram_commit to commit this change to flash.
 */
extern int BCMINIT(nvram_unset)(const char *name);

/*
 * Commit NVRAM variables to permanent storage. All pointers to values
 * may be invalid after a commit.
 * NVRAM values are undefined after a commit.
 * @return	0 on success and errno on failure
 */
extern int BCMINIT(nvram_commit)(void);

/*
 * Get all NVRAM variables (format name=value\0 ... \0\0).
 * @param	buf	buffer to store variables
 * @param	count	size of buffer in bytes
 * @return	0 on success and errno on failure
 */
extern int BCMINIT(nvram_getall)(char *buf, int count);

#endif /* _LANGUAGE_ASSEMBLY */

#define NVRAM_MAGIC		0x48534C46	/* 'FLSH' */
#define NVRAM_CLEAR_MAGIC		0x0
#define NVRAM_INVALID_MAGIC	0xFFFFFFFF
#define NVRAM_VERSION		1
#define NVRAM_HEADER_SIZE	20
/* NVRAM_SPACE defined to drivers, app access by ioctl */
//#define NVRAM_SPACE		0x8000
//#define NVRAM_SPACE		0x10000
//#define NVRAM_SPACE		0xA0000

#define NVRAM_MAX_VALUE_LEN 255
#define NVRAM_MAX_PARAM_LEN 64
#ifdef  __cplusplus
}
#endif

#endif /* _bcmnvram_h_ */
