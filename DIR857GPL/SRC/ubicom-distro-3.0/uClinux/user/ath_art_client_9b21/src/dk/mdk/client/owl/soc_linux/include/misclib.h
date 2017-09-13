#define NFLC            3               /* number of flash config segments */
#define FLC_BOOTLINE    0
#define FLC_BOARDDATA   1
#define FLC_RADIOCFG    2
#define DEFAULT_BOOT_LINE \
        "tffs:(0,0):/fl/APIMG1 f=0x00 e=192.168.1.20:0xffffff00 o=ae"
typedef unsigned int UINT32;
typedef unsigned short UINT16;
/*Board configuration data to allow software to determine
  * configuration at run time.Stored in a read only flash
  * *configuration block. * */
struct ar531x_boarddata
{
  UINT32 magic;			/* board data is valid */
#define BD_MAGIC        0x35333131	/* "5311", for all 531x platforms */
  UINT16 cksum;			/* checksum (starting with BD_REV 2) */
  UINT16 rev;			/* revision of this struct */
#define BD_REV  4
  char boardName[64];		/* Name of board */
  UINT16 major;			/* Board major number */
  UINT16 minor;			/* Board minor number */
  UINT32 config;		/* Board configuration */
#define BD_ENET0        0x00000001	/* ENET0 is stuffed */
#define BD_ENET1        0x00000002	/* ENET1 is stuffed */
#define BD_UART1        0x00000004	/* UART1 is stuffed */
#define BD_UART0        0x00000008	/* UART0 is stuffed (dma) */
#define BD_RSTFACTORY   0x00000010	/* Reset factory defaults stuffed */
#define BD_SYSLED       0x00000020	/* System LED stuffed */
#define BD_EXTUARTCLK   0x00000040	/* External UART clock */
#define BD_CPUFREQ      0x00000080	/* cpu freq is valid in nvram */
#define BD_SYSFREQ      0x00000100	/* sys freq is set in nvram */
#define BD_WLAN0        0x00000200	/* use WLAN0 */
#define BD_MEMCAP       0x00000400	/* CAP SDRAM @ memCap for testing */
#define BD_DISWATCHDOG  0x00000800	/* disable system watchdog */
#define BD_WLAN1        0x00001000	/* Enable WLAN1 (ar5212) */
#define BD_ISCASPER     0x00002000	/* FLAG for low cost ar5212 */
#define BD_WLAN0_2G_EN  0x00004000	/* FLAG for radio0_2G */
#define BD_WLAN0_5G_EN  0x00008000	/* FLAG for radio0_2G */
#define BD_WLAN1_2G_EN  0x00020000	/* FLAG for radio0_2G */
#define BD_WLAN1_5G_EN  0x00040000	/* FLAG for radio0_2G */
#define BD_LOCALBUS     0x00080000	/* Enable Local Bus */
  UINT16 resetConfigGpio;	/* Reset factory GPIO pin */
  UINT16 sysLedGpio;		/* System LED GPIO pin */

  UINT32 cpuFreq;		/* CPU core frequency in Hz */
  UINT32 sysFreq;		/* System frequency in Hz */
  UINT32 cntFreq;		/* Calculated C0_COUNT frequency */

  A_UINT8 wlan0Mac[6];
  A_UINT8 enet0Mac[6];
  A_UINT8 enet1Mac[6];

  UINT16 pciId;			/* Pseudo PCIID for common code */
  UINT16 memCap;		/* cap bank1 in MB */

#define SIZEOF_BD_REV2  120	/* padded by 2 bytes */

  /* version 3 */
  A_UINT8 wlan1Mac[6];		/* (ar5212) */
};
extern struct ar531x_boarddata sysBoardData;
int bdChange(void), bdShow(struct ar531x_boarddata *);
UINT16 sysBoardDataChecksum(struct ar531x_boarddata *bd);       /* for MDK */
void writeProdDataMacAddr(A_UINT32 devNum, A_UCHAR *mac0Addr, A_UCHAR *mac1Addr); 
