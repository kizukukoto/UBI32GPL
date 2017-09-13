#ifndef __LANG_H__
#define __LANG_H__

#define LP_HDR_SIZE	40
#define LP_MTD		"/dev/mtd4"
//#define LP_MTD_BLOCK	"/dev/mtdblock4"
#define LP_MTD_MOUNT	"/tmp/lang_pack"
#define LP_LINKNAME	"/www/lang.js"
#define LP_TARGETNAME	LP_MTD_MOUNT"/lang.js"
#define LP_MODEL_REGEX	"DIR-652[A-Z]{0,2}_[A-Z][0-9]"
#define LP_VER_REGEX	"[0-9].[0-9][0-9]b[0-9][0-9]"
#define LP_REG_REGEX	"[A-Z][A-Z]"
#define LP_DATE_REGEX	"[A-Za-z]{3}, [0-9]{2} [A-Za-z]{3} [0-9]{4}"

#define LP_SIZE_KEY	"lang_size"
#define LP_VERSION_KEY	"lang_version"
#define LP_REGION_KEY	"lang_region"
#define LP_DATE_KEY	"lang_date"

/* cgi/lib/liblang.c */
extern int lang_pack_get_info(char *, char *, char *, char *, char *);
extern int lang_pack_get_header(const char *, char *);
extern int lang_pack_verify(const char *);
extern int lang_pack_update_info(const char *);
extern int lang_pack_umount();
extern int lang_pack_mount();
extern int lang_pack_erase();
#endif
