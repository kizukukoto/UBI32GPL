
typedef unsigned char u8_t;
typedef unsigned int u16_t;

/*
 * netbios_name
 */
struct netbios_name {
	char name[16];
	u8_t type;		/* Type of name e.g. NETBIOS_WORKSTATION */
	u16_t name_flags;	/* As specified in STD19 4.2.18 */
};
#define NETBIOS_NAME_FLAGS_GLOBAL_UNIQUE 0x8000

/*
 * Name types
 */
#define NETBIOS_WORKSTATION 0x00
#define NETBIOS_DOMAIN_MASTER_BROWSER 0x1B
#define NETBIOS_MASTER_BROWSER 0x1D
#define NETBIOS_BROWSER_ELECTION_SERVICE 0x1E
#define NETBIOS_SERVER_SERVICE 0x20

typedef enum { IP_ADDR, SUBNET_MASK, DEFAULT_GATEWAY, HW_ADDR } ADDR_T;
