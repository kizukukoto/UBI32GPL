#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <getopt.h>
#include <errno.h>
#include <err.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <fcntl.h>
#include <stdint.h>
#include <net/ethernet.h>
#define SIOCDEVPRIVATE   0x89F0  /* to 89FF */
#define ATHR_GMAC_QOS_CTRL_IOC      (SIOCDEVPRIVATE | 0x01)
#define ATHR_GMAC_CTRL_IOC          (SIOCDEVPRIVATE | 0x02)
#define ATHR_PHY_CTRL_IOC           (SIOCDEVPRIVATE | 0x03)
#define ATHR_VLAN_IGMP_IOC          (SIOCDEVPRIVATE | 0x04)
#define ATHR_HW_ACL_IOC             (SIOCDEVPRIVATE | 0x05)

#define ATHR_PHY_FORCE           ((ATHR_PHY_CTRL_IOC << 16) | 0x1)
#define ATHR_PHY_RD              ((ATHR_PHY_CTRL_IOC << 16) | 0x2)
#define ATHR_PHY_WR              ((ATHR_PHY_CTRL_IOC << 16) | 0x3)
#define ATHR_ACL_COMMIT          ((ATHR_HW_ACL_IOC << 16) | 0x1)
#define ATHR_ACL_FLUSH           ((ATHR_HW_ACL_IOC << 16) | 0x2)
#define ATHR_JUMBO_FRAME                  ((ATHR_GMAC_CTRL_IOC << 16) | 0x7)                   /* Jumbo packet frame size*/
#define ATHR_FRAME_SIZE_CTL               ((ATHR_GMAC_CTRL_IOC << 16) | 0x8)                   /* Jumbo packet frame size*/



struct eth_cfg_params {
     int  cmd;
     char    ad_name[IFNAMSIZ];      /* if name, e.g. "eth0" */
     uint16_t vlanid;
     uint16_t portnum;
     uint32_t phy_reg;
     uint32_t tos;
     uint32_t val;
     uint8_t duplex;
     uint8_t  mac_addr[6];
 //    uint8_t buf[128];
};

struct ifreq ifr;
struct eth_cfg_params etd;
int s,opt_force = 0,duplex = 1;
const char *progname;

#ifdef IP8000


#else
u_int32_t
regread(u_int32_t phy_reg,u_int16_t portno)
{

	etd.phy_reg = phy_reg;
        etd.cmd     = ATHR_PHY_RD;
	etd.portnum = portno;
	if (ioctl(s,ATHR_PHY_CTRL_IOC, &ifr) < 0)
		err(1, etd.ad_name);

	return etd.val;
}
#endif

static void athr_en_jumboframe(int value)
{
	etd.cmd = ATHR_JUMBO_FRAME;
	etd.val=value;
	if (ioctl(s,ATHR_GMAC_CTRL_IOC, &ifr) < 0)
        err(1,etd.ad_name);
}
static void athr_set_framesize(int sz)
{
  etd.cmd = ATHR_FRAME_SIZE_CTL;
  etd.val = sz;
  if (ioctl(s,ATHR_GMAC_CTRL_IOC, &ifr) < 0)
  	err(1,etd.ad_name);
  
}
static void athr_commit_acl_rules()
{
  etd.cmd = ATHR_ACL_COMMIT;
  if (ioctl(s,ATHR_HW_ACL_IOC,&ifr) < 0)
     printf("%s ioctl error\n",__func__);
  
}
static void athr_flush_acl_rules()
{
  etd.cmd = ATHR_ACL_FLUSH;
  if (ioctl(s,ATHR_HW_ACL_IOC,&ifr) < 0)
     printf("%s ioctl error\n",__func__);
}


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> 

int _system(const char *fmt, ...)
{
	va_list args;
	int i;
	char buf[512];

	va_start(args, fmt);
	vsprintf(buf, fmt,args);
	va_end(args);

//	DEBUG_MSG("%s\n",buf);
	i = system(buf);
	return i;

}



#ifdef IP8000
#else
static void
regwrite(u_int32_t phy_reg,u_int32_t val,u_int16_t portno)
{
        
	etd.val     = val;
	etd.phy_reg = phy_reg;
        etd.portnum = portno;
        if(opt_force)  {
             etd.duplex   = duplex;
             etd.cmd      = ATHR_PHY_FORCE;
	    if (ioctl(s,ATHR_PHY_CTRL_IOC, &ifr) < 0)
		err(1, etd.ad_name);
            opt_force = 0;
        }
        else {
            etd.cmd = ATHR_PHY_WR;
	    if (ioctl(s,ATHR_PHY_CTRL_IOC, &ifr) < 0)
		err(1, etd.ad_name);
        }
}
u_int32_t
__ethregread(u_int32_t phy_reg,u_int16_t portno)
{

	etd.phy_reg = phy_reg;
        etd.cmd     = ATHR_PHY_RD;
	etd.portnum = portno;
	if (ioctl(s,ATHR_PHY_CTRL_IOC, &ifr) < 0)
		err(1, etd.ad_name);

	return etd.val;
}
#endif

#ifdef IP8000
// try use ioctl 
struct MY_PHY_REG {
	uint32_t cmd;
	uint32_t addr;
	uint32_t value;
	uint32_t result;
};


u_int32_t ethregread(u_int32_t phy_reg)
{

        struct ifreq ifreq_ioctl;
	struct MY_PHY_REG *reg;
        
        strcpy(ifreq_ioctl.ifr_name,  "eth0");

	reg = (struct MY_PHY_REG *) ifreq_ioctl.ifr_ifru.ifru_slave;

	reg->cmd = 1; // read it
	reg->addr = phy_reg;
	reg->value = 0;
	reg->result = -1;

        ioctl(s, SIOCDEVPRIVATE+1, &ifreq_ioctl);

//printf("read %08x %08x\n",reg->addr,reg->value);

	return reg->value;

}

void ethregwrite(u_int32_t phy_reg,u_int32_t val)
{
        struct ifreq ifreq_ioctl;
	struct MY_PHY_REG *reg;
        
        strcpy(ifreq_ioctl.ifr_name,  "eth0");

	reg = (struct MY_PHY_REG *) ifreq_ioctl.ifr_ifru.ifru_slave;

	reg->cmd = 2; // write it
	reg->addr = phy_reg;
	reg->value = val;
	reg->result = -1;

        ioctl(s, SIOCDEVPRIVATE+1, &ifreq_ioctl);
	
//printf("write %08x %08x\n",reg->addr,reg->value);
}

#if 0
// file access
u_int32_t ethregread(u_int32_t phy_reg)
{
//	return regread(phy_reg, 0x3f);
	FILE *faddr = NULL;
	FILE *fval = NULL;
	char buff[128];
	char buff2[128];

	FILE *f;
	int switch_value;
	
	u_int32_t ret_value;
	
	ret_value = 0;
	
	sprintf(buff,"echo 0x%x > /proc/switch/ar8327-smi/reg/addr",phy_reg);
	_system(buff);

		if ((f = fopen("/proc/switch/ar8327-smi/reg/addr", "r")) != NULL)
		{
			fscanf(f, "%x", &switch_value);
			//printf("/proc/switch/ar8276-smi/reg/addr=0x50(%x)\n",switch_value);
			fclose(f);
			if(switch_value != phy_reg) {
				printf("error phy_reg\n");
				goto out;
			}
		}



#if 0
	faddr = popen(buff, "r");
	if( faddr == NULL) {
		printf("error faddr\n");
		goto out;
	}
#endif
	
//	sprintf(buff,"cat /proc/switch/ar8327-smi/reg/val");
	sprintf(buff,"/proc/switch/ar8327-smi/reg/val");
	
	fval = fopen(buff, "r");
	if( fval == NULL) {
		printf("error faddr\n");
		goto out;
	}
	
	fgets(buff, sizeof(buff), fval);
	sscanf(buff,"%x",&ret_value);
	
//	printf("read %x %x\n",phy_reg,ret_value);
out:	
	if( faddr )
		fclose(faddr);
	if( fval )
		fclose(fval);
		
	return ret_value;

}

void ethregwrite(u_int32_t phy_reg,u_int32_t val)
{
	//ethregread(phy_reg);
//	regwrite(phy_reg, val, 0x3f);
	FILE *faddr = NULL;
	FILE *fval = NULL;
	char buff[128];
	char buff2[128];
	
//	u_int32_t ret_value;
	FILE *f;
	int switch_value;
	
//	ret_value = 0;
	
	sprintf(buff,"echo 0x%x > /proc/switch/ar8327-smi/reg/addr",phy_reg);
	_system(buff);

		if ((f = fopen("/proc/switch/ar8327-smi/reg/addr", "r")) != NULL)
		{
			fscanf(f, "%x", &switch_value);
			//printf("/proc/switch/ar8276-smi/reg/addr=0x50(%x)\n",switch_value);
			fclose(f);
			if(switch_value != phy_reg) {
				printf("error phy_reg\n");
				goto out;
			}
		}

#if 0
	faddr = popen(buff, "r");
	if( faddr == NULL) {
		printf("error faddr\n");
		goto out;
	}
#endif
	
	sprintf(buff,"echo 0x%x > /proc/switch/ar8327-smi/reg/val",val);
	_system(buff);

#if 0	
	fval = popen(buff, "r");
	if( fval == NULL) {
		printf("error faddr\n");
		goto out;
	}
#endif	

//	printf("write %x %x\n",phy_reg,val);
out:	
	if( faddr )
		fclose(faddr);
	if( fval )
		fclose(fval);
		

}
#endif


#else
u_int32_t ethregread(u_int32_t phy_reg)
{
	return regread(phy_reg, 0x3f);
}

void ethregwrite(u_int32_t phy_reg,u_int32_t val)
{
	//ethregread(phy_reg);
	regwrite(phy_reg, val, 0x3f);
}
#endif
//ethregwrite(phy_reg, val) regwrite(phy_reg, val, 0x3f)
//#define ethregwrite(phy_reg, val) regwrite(phy_reg, val, 0x3f)


/*************************************************************
 *	ATU(address table)
 * ***********************************************************/
struct atu_entry{
	union {
		uint32_t atu_data[3];
	};
};

int __ether_atoe(const char *a, unsigned char *e)
{
        char m[128], *p, *x;
        int i = 0;
	char *c;

	p = m;
	strcpy(p, a);

        memset(e, 0, ETHER_ADDR_LEN);
        for (i = 0; i < 6; i++) {
		x = strsep(&p, ":-");
                e[i] = (unsigned char) strtoul(x, &c, 16);
        }
        return 0;
}
static int __atu_add(unsigned char *mac, int vlan, char portmap,  struct atu_entry *en)
{
	uint32_t r, reg;

	r = ethregread(0x600);
	memcpy(&reg, mac + 2, 4);
        ethregwrite(0x600, reg);

	reg = 0x80000000;
	memcpy((char *)&reg + 2, mac, 2);
	reg |= (portmap << 16);
        ethregwrite(0x604, reg);		
	
	reg = 0xf;
	reg |= (vlan << 8);
        ethregwrite(0x608, reg);

	reg = 0x80000002;
        ethregwrite(0x60c, reg);
	return 0;
}
int atu_add(unsigned char *mac, int vlan, char portmap)
{
	return __atu_add(mac, vlan, portmap, NULL);
}
int atu_dump_entry_result()
{
	uint32_t reg0, reg, status;
	unsigned char *e;

	reg = ethregread(0x608);
	status = reg & 0xF;
	printf ("status %X :", status);
	if (status == 0) {
		printf("entry not find\n");
		return -1;
	} else if (status == 0xF)
		printf("static");
	else if (status <= 0x7)
		printf("dynamic");
	else
		printf("dynamic age out\n");
	printf(" VID: %d, leaky: %d, ", (reg >> 8) & 0x3FF, (reg >> 4) & 0x1);
	reg0 = ethregread(0x600);
	reg = ethregread(0x604);
	e = (unsigned char *)&reg0;
	printf("%02X:%02X:%02X:%02X:%02X:%02X, dportmap: 0x%02X\n",
		e[0], e[1], e[2], e[3], ((unsigned char *)&reg)[2], ((unsigned char *)&reg)[3],
		(reg >> 16) & 0x7f);
	return (reg >> 16) & 0x7f;
}

static int atu_result()
{
	uint32_t reg, status;

	reg = ethregread(0x608);
	status = reg & 0xF;
	if (status == 0)
		return -1;
	return (ethregread(0x604) >> 16) & 0x7f;
}
int atu_get_next()
{
	int i = 30;
        ethregwrite(0x600, 0);
        ethregwrite(0x604, 0);
        ethregwrite(0x608, 0);
        ethregwrite(0x60c, 0x80000006);

	while (atu_dump_entry_result() != -1){
        	ethregwrite(0x60c, 0x80000006);
		if (!i--) break;
	}
	return 0;
}
static int __atu_search(unsigned char *mac, int vlan, struct atu_entry *en)
{
	uint32_t reg;

	//sleep(1);
	memcpy(&reg, mac + 2, 4);
        ethregwrite(0x600, reg);
	//sleep(1);
	reg = 0x80000000;
	memcpy((char *)&reg + 2, mac, 2);
        ethregwrite(0x604, reg);		
	
	//sleep(1);
	//fprintf(stderr, "no delay %s %d vlan: %d\n", __FUNCTION__, __LINE__, vlan);
	reg = (vlan & 0x3ff) << 8;
        ethregwrite(0x608, reg);		

	//return 0;
	reg = 0x80000007;
        ethregwrite(0x60c, reg);		
	//return 0;
	return atu_result();
	//return atu_dump_entry_result();
/*
ethreg -i eth0 0x600=0x0c07ac28
ethreg -i eth0 0x604=0x80000000
ethreg -i eth0 0x608=0x200
ethreg -i eth0 0x60c=0x80000007
ethreg -i eth0 0x600
ethreg -i eth0 0x604

*/	
}
int atu_search(unsigned char *mac, int vlan)
{
	return __atu_search(mac, vlan, NULL); 
}

int atu_purge(unsigned char *mac, int vlan)
{
	if (__atu_search(mac, vlan, NULL) == -1)
		return -1;
        ethregwrite(0x60c, 0x80000003);		
	return 0;
}

void atu_flushall()
{
	ethregwrite(0x60c, 0x80000001);
}
/*********************************************************************
 *
 * ACL stuff
 *
 * ********************************************************************/
static char *ACL_RULE_TYPE[] = {
	"NULL",			/* Invalid Rule		*/
	"MAC",			/* 3'b001 MAC rule	*/
	"IPv4",			/* 3'b010 IPv4 rule	*/
	"IPv6_Rule1",		/* 3'b011 IPv6 rule1?	*/
	"IPv6_Rule2",		/* 3'b100 IPv6 rule2	*/
	"IPv6_Rule3",		/* 3'b101 IPv6 rule3	*/
	"Window",		/* 3'b110 Window rule	*/
	"EnhanceMAC",		/* 3'b111 Enhanced MAC	*/
	NULL,
};
static int debug_acl;
struct acl_rule_entry
{
	uint8_t patten[20];
	uint8_t mask[20];
	uint8_t action[20];
};

enum ACL_RULE_SEL
{
	ACL_RULE_PATTERN,
	ACL_RULE_MASK,
	ACL_RULE_ACTION,
	ACL_RULE_MAX
};
static void acl_write_raw_data(uint32_t *b, int n, enum ACL_RULE_SEL sel, char *msg)
{

	ethregwrite(0x404, b[0]);
	ethregwrite(0x408, b[1]);
	ethregwrite(0x40c, b[2]);
	ethregwrite(0x410, b[3]);
	ethregwrite(0x414, b[4]);
	ethregwrite(0x400, 0x80000000 + n + (sel <<8));

	if (debug_acl) {
		if (msg)
			printf("%s\n", msg);
		printf("0x404: %08X\n", b[0]);
		printf("0x408: %08X\n", b[1]);
		printf("0x40c: %08X\n", b[2]);
		printf("0x410: %08X\n", b[3]);
		printf("0x414: %08X\n", b[4]);
		printf("0x400: %08X\n\n", 0x80000000 + n + (sel <<8));
	}

}
static void acl_fetch_raw_data(uint32_t *b)
{
	/* ignored endian-type, assume big-endian */
	b[0] = ethregread(0x404);	/* Byte[0:3]  */
	b[1] = ethregread(0x408);	/* Byte[4:7]  */
	b[2] = ethregread(0x40c);	/* Byte[8:11] */
	b[3] = ethregread(0x410);	/* Byte[12:15]*/

	/* byte[16] located LSB/MSB of b[4], depend on big/little endian ... */
	b[4] = ethregread(0x414);	/* Byte[16]   */
/*
	printf("404: 0x%08X\n", b[0]);
	printf("408: 0x%08X\n", b[1]);
	printf("40c: 0x%08X\n", b[2]);
	printf("410: 0x%08X\n", b[3]);
	printf("414: 0x%08X\n", b[4]);
*/
}

static int acl_fetch(int n, enum ACL_RULE_SEL r, struct acl_rule_entry *e)
{
	if (r >= ACL_RULE_MAX)
		return -1;
	ethregwrite(0x400, 0x80000400 + n + (r << 8));
	//printf("%s% d: 0x%08X\n", __FUNCTION__, __LINE__, 0x80000400 + n + (r << 8));
	switch (r) {
	case ACL_RULE_PATTERN:
		acl_fetch_raw_data((void *)e->patten);
		break;
	case ACL_RULE_MASK:
		acl_fetch_raw_data((void *)e->mask);
		break;
	case ACL_RULE_ACTION:
		acl_fetch_raw_data((void *)e->action);
		break;
	default:
		return -1;
	}
	return 0;
}
static int acl_dump_entry(int n, int debug)
{
	struct acl_rule_entry e;
	uint32_t reg;
	acl_fetch(n, ACL_RULE_MASK, &e);

	reg = *(uint32_t *)(e.mask + 16);
	//printf("%s%d:\n", __FUNCTION__, __LINE__);
	if (!(reg & 0x03))
		return 0;	//unused

	printf("rule %d: 0x%08X: valid: 0x%02X = %s\n", n, reg, (reg & 0x3), ACL_RULE_TYPE[reg & 0x3]);
	acl_fetch(n, ACL_RULE_PATTERN, &e);
	acl_fetch(n, ACL_RULE_ACTION, &e);
	if (debug) {
		uint32_t *p = (uint32_t *)&e;
		int n;

		printf("patten:\n");
		for (n = 0;n < 5;n++, p++)
			printf("%08X\n", *p);
		printf("mask:\n");
		for (n = 0;n < 5;n++, p++)
			printf("%08X\n", *p);
		printf("action:\n");
		for (n = 0;n < 5;n++, p++)
			printf("%08X\n", *p);
	}
	return 0;
}

void acl_dump(int debug)
{
	int i;
	uint32_t reg;

	for (i = 0; i < 95; i++) {
		ethregwrite(0x400, 0x80000500 + i);
		reg = ethregread(0x414);
		//printf("rule %d: 0x%08X: valid: 0x%02X = %s\n", i, reg, (reg & 0x3), ACL_RULE_TYPE[reg & 0x3]);
		acl_dump_entry(i, debug);
	}
}

void acl_disable_entry(int n)
{
	struct acl_rule_entry e;
	uint32_t reg;

	acl_fetch(n, ACL_RULE_MASK, &e);

	reg = *(uint32_t *)(e.mask + 16);
	//printf("%08X\n", reg);
	if (!(reg & 0x03))
		return;	//unused
	printf("disable rule:%d type: %s\n", n, ACL_RULE_TYPE[reg & 0x03]);
	reg &= ~0x7;
	ethregwrite(0x414, reg);
	ethregwrite(0x400, 0x80000100 + n);
}

void acl_enable_entry(int n, const char *s)
{
	struct acl_rule_entry e;
	uint32_t reg;
	int i;
	char **p = ACL_RULE_TYPE;

	for (i = 0; *p != NULL; p++, i++) {
		if (strncmp(*p, s, strlen(s)) == 0)
			break;
	}
	if (*p == NULL)
		return;
	acl_fetch(n, ACL_RULE_MASK, &e);
	printf("enabled rule %d type: %s\n", n, s);
	reg = *(uint32_t *)(e.mask + 16);
	reg &= ~0x7;
	reg |= i;
	ethregwrite(0x414, reg);
	ethregwrite(0x400, 0x80000100 + n);
	return;
}

struct acl_mac_pattern{
	unsigned char	da_l[4];
	unsigned char	sa_l[2];
	unsigned char	da_h[2];
	unsigned char	sa_h[4];
	unsigned short	proto_type;
	unsigned short	pri:3,
			cfi:1,
			vid:12;
	unsigned char	unused[3];
	unsigned char	invert:1,
			ports:7;
	
};//__attribute__ ((packed));

struct acl_mac_mask{
	unsigned char	da_l[4];
	unsigned char	sa_l[2];
	unsigned char	da_h[2];
	unsigned char	sa_h[4];
	unsigned short	proto_type;
	unsigned short	pri:3,
			cfi:1,
			vid:12;
	unsigned char	unused[3];
	unsigned char	rule_valid	:2,
			frame_with_tag_mask:1,
			frame_with_tag	:1,
			vid_mask	:1,
			rule_type	:3;
};

struct acl_action
{
	/* bit [31:16] */
	unsigned short	ctag_pri	:3,
			cfi		:1,
			ctag_vid	:12;
	/* bit [15:0] */
	unsigned short	stag_pri	:3,
			dei		:1,
			stag_vid	:12;
	/* bit [63:48] */
	unsigned short	des_port_l	:3,
			enq_pri_over_en	:1,
			enq_pri		:3,
			arp_wcmp	:1,
			arp_index	:7,
			arp_index_over_en:1;
			
	/* bit [47:40] */
	unsigned char	force_l3	:2,
			lookup_vid_change_en:1,
			trans_ctag_change_en:1,
			trans_stag_change_en:1,
			ctag_dei_change_en:1,
			ctag_pri_remap_en:1,
			stag_dei_change_en:1;
	/* bit [39:32] */
	unsigned char	stag_pri_remap_en:1,
			dscp_remap_en	:1,
			dscp		:6;
	/* bit [95:80] */
	unsigned short	reserved:15,
			acl_int_en      :1;

	/* bit [79:64] */
	unsigned short	acl_eg_trans_bypass:1,
			acl_rate_en	:1,
			acl_rate_sel	:5,
			acl_dp_action	:3,
			mirror_en	:1,
			des_port_en	:1,
			des_port_h	:4;
	unsigned int	unused[2];
};//__attribute__((packed));
struct acl_mac_entry
{
	struct acl_mac_pattern pattern;
	struct acl_mac_mask mask;
	struct acl_action action;
};
/*
 * convert "00:12:34:56:78:9a/ff:ff:ff:ff:ff:ff" to binary raw data of ACL rule.
 * */
static int __acl_mac_mask2bin(const unsigned char *s, struct acl_mac_entry *e, int da)
{
	char *p, *h;
	unsigned char m1[ETHER_ADDR_LEN], m2[ETHER_ADDR_LEN];

	if ((s = (void *)strdup((char *)s)) == NULL)
		return -1;
	h = (char *)s;
	p = strsep((char **)&s, "/");
	
	if (p == NULL)
		return -1;
	__ether_atoe(p, m1);
	/* mask support "00:11:22:33:44:55/..." without slash '/', oa ".../4" as mask bits */
	memset(m2, 0xff, ETHER_ADDR_LEN);
	if (s)
		__ether_atoe((char *)s, m2);

	if (da) {
		memcpy(e->pattern.da_h, m1, 2);
		memcpy(e->pattern.da_l, m1 + 2, 4);
		memcpy(e->mask.da_h, m2, 2);
		memcpy(e->mask.da_l, m2 + 2, 4);
	} else {
		memcpy(e->pattern.sa_h, m1, 4);
		memcpy(e->pattern.sa_l, m1 + 4, 2);
		memcpy(e->mask.sa_h, m2, 4);
		memcpy(e->mask.sa_l, m2 + 4, 2);
	}
	free(h);
	return 0;
}

static int acl_get_frist_unused()
{
	struct acl_rule_entry e;
	int i;
	uint32_t reg;

	for (i = 0; i < 95; i++) {
		ethregwrite(0x400, 0x80000500 + i);
		reg = ethregread(0x414);
		acl_fetch(i, ACL_RULE_MASK, &e);

		reg = *(uint32_t *)(e.mask + 16);
		if (!(reg & 0x03))
			return i;
	}
	return -1;
}
static void mac_add_help()
{
	fprintf(stderr, "r:T:m:hn:d:s:p:v:j:i\n"
	" n: rule number[0-95], default: 0, large 95 will append frist unused rule\n"
	" d|s: dest/src eg: 00:11:22:33:44:55/ff:ff:ff:ff:ff:ff, default 0/0\n"
	" p: port bit map max 0x3f as default\n"
	" v: vid, default: 0\n"
	" j: 7(drop, default) or 0(forward), 1(to cpu), 3(redirect)\n"
	" T: ethernet protocols number if given\n"
	" i: invert match\n"
	" m: combine rule: 3 - start & end (default), 0 - start, 1 - continue, 2 - end\n"
	" r: redirect dest\n"
	"eg: ./mcastd acl add -n 12 -d 00:17:31:30:48:e8 -j 1 -v 2 -p 0x3f\n"
	);
}
/* 
 * ./mcastd acl add -n 12 -d ff:ff:ff:ff:ff:ff/ff:00:00:00:00:00 -j 0 -v 2 -p 0x3f
 * */
int acl_add_mac_entry(int argc, char *argv[])
{
	int c;
	unsigned int n = 0, vid = 0;

	struct acl_mac_entry e;

	if (argc < 5) {
		mac_add_help();
		return -1;
	}
	memset(&e, 0, sizeof(struct acl_mac_entry));
	e.mask.vid_mask = 1;		/* vid mask, not range*/
	e.mask.rule_valid = 3;		/* start & end 	*/
	e.mask.rule_type = 1;		/* MAC rule	*/
	e.action.acl_dp_action = 7;	/* drop		*/
	e.pattern.ports = 0x3F;		/* src all ports*/
	while (1) {
		/* TODO: add proto_type for ipv4/6 */
		c = getopt(argc, argv, "r:T:m:hn:d:s:p:v:j:iD");
		if (c == -1)
			break;
		switch (c) {
		case 'h':
			mac_add_help();
			return -1;
		case 'n':
			n = atoi(optarg);
			if (n > 95)
				n = acl_get_frist_unused();
			if (n == -1) {
				printf("acl rule full max 0-95\n");
				return -1;
			}
			break;
		case 'd':
			__acl_mac_mask2bin(optarg, &e, 1);
			break;
		case 's':
			__acl_mac_mask2bin(optarg, &e, 0);
			break;
		case 'p':
			e.pattern.ports = strtoul(optarg, NULL, 0);
			break;
		case 'v':
			vid = strtoul(optarg, NULL, 0);
			e.pattern.vid = vid;
			e.mask.vid = 0xfff;
			break;
		case 'j':
			e.action.acl_dp_action = atoi(optarg) & 0x7;
			break;
		case 'i':
			e.pattern.invert = 1;
			break;
		case 'D':
			debug_acl = 1;
			break;
		case 'T':
			e.pattern.proto_type = strtoul(optarg, NULL, 0);
			e.mask.proto_type = 0xFFFF;
			break;
		case 'm': /* start|end|continue */
			e.mask.rule_valid = strtoul(optarg, NULL, 0) & 0x3; 
			break;
		case 'r':
			e.action.des_port_h = (strtoul(optarg, NULL, 0) >> 3) & 0xf; 
			e.action.des_port_l = strtoul(optarg, NULL, 0) & 0x7; 
			e.action.des_port_en = 1;
			break;
		default:
			mac_add_help();
			return -1;
		}
	}
	acl_write_raw_data((uint32_t *)&e.pattern, n, ACL_RULE_PATTERN, "Pattern");
	acl_write_raw_data((uint32_t *)&e.action, n, ACL_RULE_ACTION, "Action");
	acl_write_raw_data((uint32_t *)&e.mask, n, ACL_RULE_MASK, "Mask");
	return 0;
	//return acl_add_mac_entry(12);

}
/* test/demo function */ 
int __acl_add_mac_entry(int n)
{
	struct acl_mac_entry e;
	unsigned char m1[ETHER_ADDR_LEN], m2[ETHER_ADDR_LEN];

	__ether_atoe("00:17:31:30:48:e8", m1);
	__ether_atoe("ff:ff:ff:ff:ff:ff", m2);
	
	memset(&e, 0, sizeof(struct acl_mac_entry));

	memcpy(e.pattern.da_h, m1, 2);
	memcpy(e.pattern.da_l, m1 + 2, 4);
	memcpy(e.mask.da_h, m2, 2);
	memcpy(e.mask.da_l, m2, 4);
	printf("%02X:%02X, pat:%d, mask: %d, act: %d\n", m1[0], m1[1],
		sizeof(struct acl_mac_pattern),
		sizeof(struct acl_mac_mask),
		sizeof(struct acl_action));
	e.mask.rule_type = 1;
	e.mask.rule_valid = 3;
	e.action.acl_dp_action = 7;//drop
	e.mask.vid_mask = 1;
	e.pattern.ports = 0x3f;
	/*
	e.action.arp_wcmp= 1;//drop
	e.action.ctag_pri = 7;//drop
	e.action.ctag_vid = 0xfff;
	e.action.stag_pri_remap_en = 1;
	e.action.force_l3 = 2;
	e.action.acl_int_en = 1;
	*/
	acl_write_raw_data((uint32_t *)&e.pattern, n, ACL_RULE_PATTERN, "Pattern");
	acl_write_raw_data((uint32_t *)&e.action, n, ACL_RULE_ACTION, "Action");
	acl_write_raw_data((uint32_t *)&e.mask, n, ACL_RULE_MASK, "Mask");
	return 0;
}	

/*************************************************
 * IPv4 pattern
 *************************************************/
struct acl_ipv4_pattern{
	unsigned int	da;
	unsigned int	sa;
	/* byte 11:8 */
	unsigned short	dport;
	unsigned char	dscp;
	unsigned char	ip_proto;
	/* byte 15:12 */
	unsigned char	reserved3:2,
			tcp_flags:6;
	unsigned char	reserved:1,
			dhcpv4:1,
			ripv1:1,
			sport_type:1,	/* 1: ICMP type code, 0: TCP/UDP SPORT */
			reserved2:4;
	unsigned short	sport;
	/* byte 16 */
	unsigned char	unused[3];
	unsigned char	invert:1,
			ports:7;
	
};

struct acl_ipv4_mask{
	unsigned int	da;
	unsigned int	sa;
	/* byte 11:8 */
	unsigned short	dport;
	unsigned char	dscp;
	unsigned char	ip_proto;
	/* byte 15:12 */
	unsigned char	reserved3:2,
			tcp_flags_mask:6;
	unsigned char	reserved:1,
			dhcpv4:1,
			ripv1:1,
			reserved2:3,
			dport_mask:1,	/* 1: B11 mask, 0: B11 range */
			sport_mask:1;	/* 1: B12 mask, 0: B12 range */
	unsigned short	sport;
	/* byte 16 */
	unsigned char	unused[3];
	unsigned char	rule_valid	:2,
			frame_with_tag_mask:1,
			frame_with_tag	:1,
			vid_mask	:1,
			rule_type	:3;
	
};
struct acl_ipv4_entry
{
	struct acl_ipv4_pattern pattern;
	struct acl_ipv4_mask mask;
	struct acl_action action;
};


static void acl_add_help()
{
	fprintf(stderr, "r:T:m:hn:d:s:p:v:j:i\n"
	"[action]\n"
	" n: rule number[0-95], default: 0, large 95 will append frist unused rule\n"
	" j: 7(drop, default) or 0(forward), 1(to cpu), 3(redirect)\n"
	"[common options] for all rule type\n"
	" p: port bit map max 0x3f as default\n"
	" i: invert match\n"
	" m: combine rule: 3 - start & end (default), 0 - start, 1 - continue, 2 - end\n"
	" r: redirect dest\n"
	"[mac rule options] see \"mcastd acl add -h\"\n"
	" v: vid, default: 0\n"
	" d|s: dest/src eg: 00:11:22:33:44:55/ff:ff:ff:ff:ff:ff, default 0/0\n"
	" T: ethernet protocols number if given\n"
	"[ipv4 rule options]\n"
	" d|s: dest/src ipv4 and port eg: -d 1.2.3.4:80/255.255.255.255 for ipv4+dport 80\n"
	" T:protocol, valid as \"tcp\" or \"udp\" or \"icmp\", default are all protocols\n"
	"eg: ./mcastd acl ipv4 add -n 20 -d 192.168.0.1:80/255.255.255.0 -T tcp -p 0x1e -j 7 -D\n"
	);
}

static int acl_ipv4_set_addr(struct acl_ipv4_entry *e, const char *data, int dir_src)
{
	char *p, *h, *ipv4, *mask = "255.255.255.255";
	unsigned int addr, amask;
	unsigned short port = 0;
	h = strdup(data);
	p = h;
	/* data = "1.2.3.4:80/255.255.255.0 or 1.2.3.4/24 or 1.2.3.4:1024/32" */
	ipv4 = strsep(&p, "/");
	if (p == NULL)
		goto ip; /* case in: data="1.2.3.4" or "1.2.3.4:80", w/o '/' */
	mask = p;
#if 0
	if (inet_pton(AF_INET, mask, &amask) <= 0) {
		int i, n;
		i = strtoul(mask, NULL, 0);	/* bit eg: 1.2.3.4/24 mask 24 */
		if (i <= 0 || i >= 32)
			return err;
		amask = 0xffffffff;
		while (i++ <= 32) {
			amask |= 1 << (32 - i);
		}
	}
#endif
ip:
	p = ipv4;
	ipv4 = strsep(&p, ":");
	if (p)
		port = strtoul(p, NULL, 0) & 0xffff;
	/* full addr/mask port/mask */
	inet_pton(AF_INET, ipv4, dir_src ? &e->pattern.sa: &e->pattern.da);
	inet_pton(AF_INET, mask, dir_src ? &e->mask.sa: &e->mask.da);
	if (port) {
		if (dir_src) {
			e->pattern.sport = port;
			e->mask.sport = 0xffff;
		} else {
			e->pattern.dport = port;
			e->mask.dport = 0xffff;
		}
	}

	free(h);
	return 0;
err:
	free(h);
	return -1;
}
#define ACL_COMMON_OPTIONS	"n:p:j:iDm:r:"
static int __acl_common_action(int argc, char *argv[], struct acl_mac_entry *e, unsigned int *n)
{
	int c;

	optind = 1;
	opterr = 0;

	while (1) {
		c = getopt(argc, argv, ACL_COMMON_OPTIONS);

		if (c == -1)
			break;
		switch (c) {
		case 'n':
			*n = atoi(optarg);
			if (*n > 95)
				*n = acl_get_frist_unused();
			if (*n == -1) {
				printf("acl rule full max 0-95\n");
				return -1;
			}
			break;
		case 'p':
			e->pattern.ports = strtoul(optarg, NULL, 0);
			break;
		case 'j':
			e->action.acl_dp_action = atoi(optarg) & 0x7;
			break;
		case 'i':
			e->pattern.invert = 1;
			break;
		case 'D':
			debug_acl = 1;
			break;
		case 'm': /* start|end|continue */
			e->mask.rule_valid = strtoul(optarg, NULL, 0) & 0x3; 
			break;
		case 'r':
			e->action.des_port_h = (strtoul(optarg, NULL, 0) >> 3) & 0xf; 
			e->action.des_port_l = strtoul(optarg, NULL, 0) & 0x7; 
			e->action.des_port_en = 1;
			break;
		default:
			break;
		}
	}
	return 0;
}

int acl_ipv4_add_entry(int argc, char *argv[])
{
	int c;
	unsigned int n = 0, vid = 0;
	struct acl_ipv4_entry e;

	if (argc < 5) {
		acl_add_help();
		return -1;
	}
	memset(&e, 0, sizeof(struct acl_ipv4_entry));
	e.mask.rule_valid = 3;		/* start & end 	*/
	e.mask.rule_type = 2;		/* IPv4 rule	*/
	e.action.acl_dp_action = 7;	/* drop		*/
	e.pattern.ports = 0x3F;		/* src all ports*/
	e.mask.dport_mask = 1;
	e.mask.sport_mask = 1;
	while (1) {
		/* TODO: add proto_type for ipv4/6 */
		c = getopt(argc, argv, "s:d:T:"ACL_COMMON_OPTIONS);
		if (c == -1)
			break;
		switch (c) {
		case 's':
			if (acl_ipv4_set_addr(&e, optarg, 1) != 0)
				return -1;
			break;
		case 'd':
			if (acl_ipv4_set_addr(&e, optarg, 0) != 0)
				return -1;
			break;
		case 'T':
			if (strncasecmp(optarg, "udp", 3) == 0)
				e.pattern.ip_proto = 17;
			if (strncasecmp(optarg, "tcp", 3) == 0)
				e.pattern.ip_proto = 6;
			/* TODO: icmp type not implement yet */
			if (strncasecmp(optarg, "icmp", 3) == 0)
				e.pattern.ip_proto = 6;
			e.mask.ip_proto = 0xff;

			break;
		case '?':
			fprintf(stderr, "%s %d\n", __FUNCTION__, __LINE__);
			printf("unknow opt %c\n", optopt);
			return 0;
		}
	}
	if (__acl_common_action(argc, argv, &e, &n) != 0)
		return -1;
	acl_write_raw_data((uint32_t *)&e.pattern, n, ACL_RULE_PATTERN, "Pattern");
	acl_write_raw_data((uint32_t *)&e.action, n, ACL_RULE_ACTION, "Action");
	acl_write_raw_data((uint32_t *)&e.mask, n, ACL_RULE_MASK, "Mask");
}
int acl_add_entry(int argc, char *argv[])
{
	int c;
	unsigned int n = 0, vid = 0;

	struct acl_mac_entry e;

	if (argc < 5) {
		mac_add_help();
		return -1;
	}
	memset(&e, 0, sizeof(struct acl_mac_entry));
	e.mask.vid_mask = 1;		/* vid mask, not range*/
	e.mask.rule_valid = 3;		/* start & end 	*/
	e.mask.rule_type = 1;		/* MAC rule	*/
	e.action.acl_dp_action = 7;	/* drop		*/
	e.pattern.ports = 0x3F;		/* src all ports*/
	while (1) {
		/* TODO: add proto_type for ipv4/6 */
		c = getopt(argc, argv, "r:T:m:hn:d:s:p:v:j:iD");
		if (c == -1)
			break;
		switch (c) {
		case 'h':
			acl_add_help();
			return -1;
		case 'n':
			n = atoi(optarg);
			if (n > 95)
				n = acl_get_frist_unused();
			if (n == -1) {
				printf("acl rule full max 0-95\n");
				return -1;
			}
			break;
		case 'd':
			__acl_mac_mask2bin(optarg, &e, 1);
			break;
		case 's':
			__acl_mac_mask2bin(optarg, &e, 0);
			break;
		case 'p':
			e.pattern.ports = strtoul(optarg, NULL, 0);
			break;
		case 'v':
			vid = strtoul(optarg, NULL, 0);
			e.pattern.vid = vid;
			e.mask.vid = 0xfff;
			break;
		case 'j':
			e.action.acl_dp_action = atoi(optarg) & 0x7;
			break;
		case 'i':
			e.pattern.invert = 1;
			break;
		case 'D':
			debug_acl = 1;
			break;
		case 'T':
			e.pattern.proto_type = strtoul(optarg, NULL, 0);
			e.mask.proto_type = 0xFFFF;
			break;
		case 'm': /* start|end|continue */
			e.mask.rule_valid = strtoul(optarg, NULL, 0) & 0x3; 
			break;
		case 'r':
			e.action.des_port_h = (strtoul(optarg, NULL, 0) >> 3) & 0xf; 
			e.action.des_port_l = strtoul(optarg, NULL, 0) & 0x7; 
			e.action.des_port_en = 1;
			break;
		default:
			mac_add_help();
			return -1;
		}
	}
	acl_write_raw_data((uint32_t *)&e.pattern, n, ACL_RULE_PATTERN, "Pattern");
	acl_write_raw_data((uint32_t *)&e.action, n, ACL_RULE_ACTION, "Action");
	acl_write_raw_data((uint32_t *)&e.mask, n, ACL_RULE_MASK, "Mask");
	return 0;
	//return acl_add_mac_entry(12);

}

void reg_init()
{
	const char *ifname = "eth0";
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		err(1, "socket");
	strncpy(etd.ad_name, ifname, sizeof (etd.ad_name));
        strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
        ifr.ifr_data = (void *) &etd;
}

#ifdef __DEBUG__
int main(int argc, char *argv[])
{
	reg_init();
	//atu_get_next();
	//atu_flushall();
	acl_add_mac_entry(12);
	acl_dump();
	return 0;
}
#endif
