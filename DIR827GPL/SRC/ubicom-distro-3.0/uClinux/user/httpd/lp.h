#define LP_READ_BUF 1024
#define LP_INVALID  (-10000)
#define VLP_MTD     "/var/www/lingualMTD.js"
#define LP_MTD      "/var/LP/lingualMTD"
#define LP_VER_LEN  50
#define LP_VER_PATH    "/var/lp_ver"
#define LP_REG_PATH	"/var/lp_reg"
#define LP_DATE_PATH "/var/lp_date"
#define LP_BUILD_PATH "/var/lp_build"
#define LP_MTD_BUILD_PATH "/var/LP/lp_build"
#define FW_UPGRADE_FILE				"/var/firm/image.bin"
/* upgrade fw/loader */
#define ERROR_UPGRADE             0
#define FW_UPGRADE                1
#define LD_UPGRADE                2

#ifdef CONFIG_LP_FW_VER_REGION
void get_firmware_version_with_LP_reg(char *fw_ver);
#endif
int check_LP_ver(void);
int link_LP(char *lingual);
int lp_upgrade(char *url, FILE *stream, int *total);
int lp_clear(char *url, FILE *stream);
