/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright © 2007 Atheros Communications, Inc.,  All Rights Reserved.
 */

/*
 * Manage the atheros ethernet PHY.
 *
 * All definitions in this file are operating system independent!
 */

//#include <linux/config.h>
#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>

#include "athrs17_phy.h"
#include "mdio.h"

/* PHY selections and access functions */
typedef enum {
    PHY_SRCPORT_INFO, 
    PHY_PORTINFO_SIZE,
} PHY_CAP_TYPE;

typedef enum {
    PHY_SRCPORT_NONE,
    PHY_SRCPORT_VLANTAG, 
    PHY_SRCPORT_TRAILER,
} PHY_SRCPORT_TYPE;

typedef enum {
    AG7100_PHY_SPEED_10T,
    AG7100_PHY_SPEED_100TX,
    AG7100_PHY_SPEED_1000T,
}ag7100_phy_speed_t;

#define DRV_LOG(DBG_SW, X0, X1, X2, X3, X4, X5, X6)
#define DRV_MSG(x,a,b,c,d,e,f)
#define DRV_PRINT(DBG_SW,X)

#define ATHR_LAN_PORT_VLAN          1
#define ATHR_WAN_PORT_VLAN          2

#define CONFIG_PORT0_AS_SWITCH 1
/*depend on connection between cpu mac and s17 mac*/
#if defined (CONFIG_PORT0_AS_SWITCH)
#define ENET_UNIT_LAN 0  
#define ENET_UNIT_WAN 1
#define CFG_BOARD_AP96 1
#else
#define ENET_UNIT_LAN 1  
#define ENET_UNIT_WAN 0
#define CFG_BOARD_PB45 1
#endif

#define TRUE    1
#define FALSE   0

#define ATHR_PHY0_ADDR   0x0
#define ATHR_PHY1_ADDR   0x1
#define ATHR_PHY2_ADDR   0x2
#define ATHR_PHY3_ADDR   0x3
#define ATHR_PHY4_ADDR   0x4
#define ATHR_IND_PHY 4

#define MODULE_NAME "ATHRS17"

/***STUFF***/
#define ETHERNET_JUMBO_FRAME 9600
#define SUPPORT_JUMBO_FRAME 1
#define SUPPORT_FLOW_CONTROL 1

//#define ETHSWITCH_VIRTUAL_LAN_AND_WAN 0

#define	SWITCH_PORTS_STAT_CONFIG (25|(2<<5)|(13<<10))
#define ATHEROS_PORT_TO_PHY(p) (p-1)	// PHY ID = PORT ID -1

#define	ATHEROS_MIN_PORT_ID 0
#define	ATHEROS_MAX_PORT_ID 5
#define	ATHEROS_MIN_PHY_ID 1
#define	ATHEROS_MAX_PHY_ID ATHEROS_MAX_PORT_ID
#define	ATHEROS_CPU_PORT_ID ATHEROS_MIN_PORT_ID
#define	ATHEROS_PHY_PORT_MASK ((1<<(ATHEROS_MAX_PHY_ID+1)) - (1<<ATHEROS_MIN_PHY_ID))
#define	ATHEROS_CPU_PORT_MASK (1<<ATHEROS_CPU_PORT_ID)
#define	ATHEROS_ALL_PORT_MASK (ATHEROS_PHY_PORT_MASK | ATHEROS_CPU_PORT_MASK)

#if defined(ETHSWITCH_VIRTUAL_LAN_AND_WAN)
#define	ATHEROS_ETHERTYPE_VLAN 0x8100	// Arbitrary value here
#define	ETHSWITCH_AR8316_LAN_ID 0x2	// This value is arbitrary, but must be different than WAN ID (below)
#define	ETHSWITCH_AR8316_WAN_ID 0x1	// This value is arbitrary, but must be different than LAN ID (above)
#define	ATHEROS_WAN_PORT_ID ATHEROS_MAX_PORT_ID	// Indicate which PHY port is for WAN
#define	ATHEROS_LAN_PORT_MASK (~(1<<ATHEROS_WAN_PORT_ID) & ATHEROS_PHY_PORT_MASK)
#define	ATHEROS_WAN_PORT_MASK ( (1<<ATHEROS_WAN_PORT_ID) & ATHEROS_PHY_PORT_MASK)
#define	ATHEROS_LAN_VLAN_MASK 0x3d560
#define	ATHEROS_WAN_VLAN_MASK 0x37fe0
#endif
/*
 * Track per-PHY port information.
 */
typedef struct {
    BOOL   isEnetPort;       /* normal enet port */
    BOOL   isPhyAlive;       /* last known state of link */
    int    ethUnit;          /* MAC associated with this phy port */
    uint32_t phyBase;
    uint32_t phyAddr;          /* PHY registers associated with this phy port */
    uint32_t VLANTableSetting; /* Value to be written to VLAN table */
} athrPhyInfo_t;

/*
 * Per-PHY information, indexed by PHY unit number.
 */
static athrPhyInfo_t athrPhyInfo[] = {
    {TRUE,   /* phy port 0 -- LAN port 0 */
     FALSE,
     ENET_UNIT_LAN,
     0,
     ATHR_PHY0_ADDR,
     ATHR_LAN_PORT_VLAN
    },

    {TRUE,   /* phy port 1 -- LAN port 1 */
     FALSE,
     ENET_UNIT_LAN,
     0,
     ATHR_PHY1_ADDR,
     ATHR_LAN_PORT_VLAN
    },

    {TRUE,   /* phy port 2 -- LAN port 2 */
     FALSE,
     ENET_UNIT_LAN,
     0,
     ATHR_PHY2_ADDR, 
     ATHR_LAN_PORT_VLAN
    },

    {TRUE,   /* phy port 3 -- LAN port 3 */
     FALSE,
     ENET_UNIT_LAN,
     0,
     ATHR_PHY3_ADDR, 
     ATHR_LAN_PORT_VLAN
    },

    {TRUE,   /* phy port 4 -- WAN port or LAN port 4 */
     FALSE,
     ENET_UNIT_WAN,
     0,
     ATHR_PHY4_ADDR, 
     ATHR_LAN_PORT_VLAN   /* Send to all ports */
    },
    
    {FALSE,  /* phy port 5 -- CPU port (no RJ45 connector) */
     TRUE,
     ENET_UNIT_LAN,
     0,
     0x00, 
     ATHR_LAN_PORT_VLAN    /* Send to all ports */
    },
};

static uint8_t athr16_init_flag = 0;

//#define ATHR_PHY_MAX (sizeof(ipPhyInfo) / sizeof(ipPhyInfo[0]))
#define ATHR_PHY_MAX 5

/* Range of valid PHY IDs is [MIN..MAX] */
#define ATHR_ID_MIN 0
#define ATHR_ID_MAX (ATHR_PHY_MAX-1)

/* Convenience macros to access myPhyInfo */
#define ATHR_IS_ENET_PORT(phyUnit) (athrPhyInfo[phyUnit].isEnetPort)
#define ATHR_IS_PHY_ALIVE(phyUnit) (athrPhyInfo[phyUnit].isPhyAlive)
#define ATHR_ETHUNIT(phyUnit) (athrPhyInfo[phyUnit].ethUnit)
#define ATHR_PHYBASE(phyUnit) (athrPhyInfo[phyUnit].phyBase)
#define ATHR_PHYADDR(phyUnit) (athrPhyInfo[phyUnit].phyAddr)
#define ATHR_VLAN_TABLE_SETTING(phyUnit) (athrPhyInfo[phyUnit].VLANTableSetting)


#define ATHR_IS_ETHUNIT(phyUnit, ethUnit) \
            (ATHR_IS_ENET_PORT(phyUnit) &&        \
            ATHR_ETHUNIT(phyUnit) == (ethUnit))

#define ATHR_IS_WAN_PORT(phyUnit) (!(ATHR_ETHUNIT(phyUnit)==ENET_UNIT_LAN))
            
/* Forward references */
static BOOL athrs17_phy_is_link_alive(int phyUnit);

void phy_mode_setup(void)
{
#ifdef S17_VER_1_0
    printk("phy_mode_setup\n");

    /*work around for phy4 rgmii mode*/
    phy_reg_write(ATHR_PHYBASE(ATHR_IND_PHY), ATHR_PHYADDR(ATHR_IND_PHY), 29, 18);     
    phy_reg_write(ATHR_PHYBASE(ATHR_IND_PHY), ATHR_PHYADDR(ATHR_IND_PHY), 30, 0x4c0c);    

    /*rx delay*/ 
    phy_reg_write(ATHR_PHYBASE(ATHR_IND_PHY), ATHR_PHYADDR(ATHR_IND_PHY), 29, 0);     
    phy_reg_write(ATHR_PHYBASE(ATHR_IND_PHY), ATHR_PHYADDR(ATHR_IND_PHY), 30, 0x82ee);  

    /*tx delay*/ 
    phy_reg_write(ATHR_PHYBASE(ATHR_IND_PHY), ATHR_PHYADDR(ATHR_IND_PHY), 29, 5);     
    phy_reg_write(ATHR_PHYBASE(ATHR_IND_PHY), ATHR_PHYADDR(ATHR_IND_PHY), 30, 0x3d46);    
#endif
}

void athrs17_reg_init()
{
    /* if using header for register configuration, we have to     */
    /* configure s17 register after frame transmission is enabled */

	u32_t phy;

	if (athr16_init_flag)
		return;

	printk("********** athrs17_reg_init start. **********\n");
	printk("Atheros switch chip ID = 0x%hx%hx\n", phy_reg_read(0, 0, 2), phy_reg_read(0, 0, 3));

	athrs17_reg_write(0x0010, (athrs17_reg_read(0x0010) & 0xfcffffff) | (1 << 31) | (1 << 24));	// Disable SPI and set LED open drain
	// Enable RGMII for CPU port
	athrs17_reg_write(0x0010, athrs17_reg_read(0x0010) | (1 << 30));	// For AR8327 package
	athrs17_reg_write(0x0004, (athrs17_reg_read(0x0004) & 0xf80fffff) | (0x3 << 24) | (0x2 << 22) | (1 << 26));
	athrs17_reg_write(0x000c, athrs17_reg_read(0x000c) | (1 << 24)); // For AR8327 RGMII global delay enable

  if ((athrs17_reg_read(0x0000) & 0xffff) == 0x1201) {
  	for (phy = 0x0; phy <= 0x4; phy++)
     {
     	/* For 100M waveform */
     	phy_reg_write(0, phy, 0x1d, 0x0);
	    phy_reg_write(0, phy, 0x1e, 0x02ea);
	    /* Turn On Gigabit Clock */
	    phy_reg_write(0, phy, 0x1d, 0x3d);
	    phy_reg_write(0, phy, 0x1e, 0x68a0);
     }
  }

#if !defined(ETHSWITCH_VIRTUAL_LAN_AND_WAN)
	athrs17_reg_write(0x000c, athrs17_reg_read(0x000c) | (1 << 17)); // PHY4_RGMII_EN


	/*
	 * MAC5/PHY4: Disable Atheros specific feature of disable MDC/MDIO access when power down.
	 */
	phy_reg_write(0, ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID), 0x1D, 0x03);
	u16_t debug_reg3 = phy_reg_read(0, ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID), 0x1E);
	phy_reg_write(0, ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID), 0x1E, debug_reg3 & ~(1 << 8));
	printk("\tSystem Mode PHY[%d:0x03]: %hx -> %hx\n", ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID),
		debug_reg3, phy_reg_read(0, ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID), 0x1E));

	/*
	 * MAC5/PHY4: Set RGMII I/F to enabel TX clock delay after HW/SW reset.
	 */
	phy_reg_write(0, ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID), 0x1D, 0x05);
	u16_t debug_sysmode = phy_reg_read(0, ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID), 0x1E);
	phy_reg_write(0, ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID), 0x1E, debug_sysmode | (1 << 8));
	printk("\tSystem Mode PHY[%d:0x05]: %hx -> %hx\n", ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID),
		debug_sysmode, phy_reg_read(0, ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID), 0x1E));

	phy_reg_write(0, ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID), 0x1D, 0x12);
	u16_t debug_rgmiimode = phy_reg_read(0, ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID), 0x1E);
	phy_reg_write(0, ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID), 0x1E, debug_rgmiimode | (1 << 3));
	printk("\tSystem Mode PHY[%d:0x12]: %hx -> %hx\n", ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID),
		debug_rgmiimode, phy_reg_read(0, ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID), 0x1E));
#endif

/* Eanble IGMP snooping (to CPU port - port 0) and allow VLAN tag */
		athrs17_reg_write(0x210,
			athrs17_reg_read(0x210) | (7 << 8) | (7 << 16) | (7 << 24));
		athrs17_reg_write(0x214,
			athrs17_reg_read(0x214) | (7 << 0) | (7 << 8));

	for (phy = ATHEROS_MIN_PHY_ID; phy <= ATHEROS_MAX_PHY_ID; phy++) {
		
#if defined(ETHSWITCH_VIRTUAL_LAN_AND_WAN)
		if (phy == ATHEROS_WAN_PORT_ID) {
			athrs17_reg_write(0x424 + (phy << 3),
				(athrs17_reg_read(0x424 + (phy << 3)) & ~(3 << 12)) | (1 << 12));	// Strip tag off egress frames from WAN port
			athrs17_reg_write(0x660 + (phy * 0xc),
				(athrs17_reg_read(0x660 + (phy * 0xc)) & ~(ATHEROS_LAN_PORT_MASK << 0)) |
				(ATHEROS_CPU_PORT_MASK << 0) | (1 << 8));
			athrs17_reg_write(0x420 + (phy << 3),
				(athrs17_reg_read(0x420 + (phy << 3)) & ~0x0fff0000) | (ETHSWITCH_AR8316_WAN_ID << 16));
		} else {
			athrs17_reg_write(0x424 + (phy << 3),
				(athrs17_reg_read(0x424 + (phy << 3)) & ~(3 << 12)) | (1 << 12));	// Strip tag off egress frames from LAN ports
			athrs17_reg_write(0x660 + (phy * 0xc),
				(athrs17_reg_read(0x660 + (phy * 0xc)) & ~(ATHEROS_WAN_PORT_MASK << 0)) |
				(ATHEROS_CPU_PORT_MASK << 0) | (1 << 8));
			athrs17_reg_write(0x420 + (phy << 3),
				(athrs17_reg_read(0x420 + (phy << 3)) & ~0x0fff0000) | (ETHSWITCH_AR8316_LAN_ID << 16));
		}

#endif
		/*
		 * Reason: Enable smartspeed function but allow more re-try times to fix RJ45 will detect
		 *	   the wrong signal and link up to 10Mbps.
		 * Modified: John Huang
		 * Date: 2009.12.02
		 */
		phy_reg_write(0, ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID), 0x14,
			phy_reg_read(0, ATHEROS_PORT_TO_PHY(ATHEROS_MAX_PORT_ID), 0x0014) | 0x1C); //set PHY 0x14[4:2]=111

		printk("Atheros switch port[%d]: control0 = 0x%x control1 = 0x%x lookup control = 0x%x\n", phy,
			athrs17_reg_read(0x420 + (phy << 3)), athrs17_reg_read(0x424 + (phy << 3)), athrs17_reg_read(0x660 + (phy * 0xc)));
	}

#if defined(ETHSWITCH_VIRTUAL_LAN_AND_WAN)
	// CPU egress all frames with tag and hardware IGMP snooping
	athrs17_reg_write(0x424 + (ATHEROS_CPU_PORT_ID << 8),
		(athrs17_reg_read(0x424 + (ATHEROS_CPU_PORT_ID << 8)) & ~(3 << 12)) | (2 << 12));	// Enable CPU port as tagged port
	athrs17_reg_write(0x660 + (ATHEROS_CPU_PORT_ID * 0xc),
		(athrs17_reg_read(0x660 + (ATHEROS_CPU_PORT_ID * 0xc)) | (1 << 8)));
	athrs17_reg_write(0x420 + (ATHEROS_CPU_PORT_ID << 8),
		(athrs17_reg_read(0x420 + (ATHEROS_CPU_PORT_ID << 8)) & ~0x0fff0000) | (ETHSWITCH_AR8316_LAN_ID << 16));
	athrs17_reg_write(0x0048, athrs17_reg_read(0x0048) | (ATHEROS_ETHERTYPE_VLAN << 0));

	/* Setup VLAN */
	while (athrs17_reg_read(0x0614) & (1 << 31)) {}
	athrs17_reg_write(0x0610, (1 << 20) | (1 << 19) | ATHEROS_LAN_VLAN_MASK);
	athrs17_reg_write(0x0614, (ETHSWITCH_AR8316_LAN_ID << 16) | (1 << 31) | 2);
	while (athrs17_reg_read(0x0614) & (1 << 31)) {}
	athrs17_reg_write(0x0610, (1 << 20) | (1 << 19) | ATHEROS_WAN_VLAN_MASK);
	athrs17_reg_write(0x0614, (ETHSWITCH_AR8316_WAN_ID << 16) | (1 << 31) | 2);
#ifdef DEBUG_PKG
	while (athrs17_reg_read(0x0614) & (1 << 31)) {}
	athrs17_reg_write(0x0614, (0 << 16) | (1 << 31) | 5);
	while (athrs17_reg_read(0x0614) & (1 << 31)) {}
	printk("Wan VLAN entry = 0x%x - 0x%x", athrs17_reg_read(0x0614), athrs17_reg_read(0x0610));
	athrs17_reg_write(0x0614, athrs17_reg_read(0x0040) | (1 << 3) | 5);
	while (athrs17_reg_read(0x0614) & (1 << 31)) {}
	printk("Lan VLAN entry = 0x%x - 0x%x", athrs17_reg_read(0x0614), athrs17_reg_read(0x0610));
#endif

#endif//defined(ETHSWITCH_VIRTUAL_LAN_AND_WAN)

#if SUPPORT_JUMBO_FRAME
	BUG_ON((ETHERNET_JUMBO_FRAME >= (1 << 14)));
	athrs17_reg_write(0x0078, (athrs17_reg_read(0x0078) & 0xffff0000) | ETHERNET_JUMBO_FRAME);
#endif

	/* Enabel CPU port and forward broadcast/multicast/unicast to CPU */
	athrs17_reg_write(0x0620, athrs17_reg_read(0x0620) | (1 << 10));		// Enable CPU port
	printk("Atheros CPU control = 0x%x\n", athrs17_reg_read(0x0620));
	athrs17_reg_write(0x7c + (ATHEROS_CPU_PORT_ID << 2),
		/*(athrs17_reg_read(0x7c + (ATHEROS_CPU_PORT_ID << 2)) & ~(0 << 9)) |*/ 0x007e);
	printk("Atheros switch port[%d]: lookup control = 0x%x status = 0x%x port_vlan = 0x%x\n", ATHEROS_CPU_PORT_ID,
		athrs17_reg_read(0x660 + (ATHEROS_CPU_PORT_ID * 0xc)), athrs17_reg_read(0x7c + (ATHEROS_CPU_PORT_ID << 2)), athrs17_reg_read(0x420 + (ATHEROS_CPU_PORT_ID << 3)));

	/* Eanble IGMP snooping (to CPU port - port 0) */
	athrs17_reg_write(0x003c, athrs17_reg_read(0x003c) | (1 << 22));
	// Flood broadcast to CPU port Flood multicast/unicast to all port
	athrs17_reg_write(0x0624, athrs17_reg_read(0x0624) | (0x7f << 16) | (0x7f << 8) | (0x7f << 0));
	// Flood IGMP to cpu port
	//athrs17_reg_write(0x0624, athrs17_reg_read(0x0624) | (1 << 24) );
	// Flood IGMP to cpu port ans wan port
	athrs17_reg_write(0x0624, athrs17_reg_read(0x0624) | (1 << 24) | (1<<29));

#if defined(SWITCH_PORTS_STATISTICS)
	athrs17_reg_write(0x0034, athrs17_reg_read(0x0034) | (1 << 24));			// Flush all MIB counters
	athrs17_reg_write(0x0034, athrs17_reg_read(0x0034) | (0 << 24) | (1 << 20));			// Do not clear MIB counters after read
	athrs17_reg_write(0x0030, athrs17_reg_read(0x0030) | (1 << 0));			// Start MIB counters
#endif

//Print all register value
#if 0
	{
		for(phy = 0; phy < 0x79; phy++){
			printk("Atheros switch register[%x] = 0x%x\n",phy , athrs17_reg_read(phy));
		}	
	
		for (phy = 0; phy < 6; phy++) {
			int reg;
			printk("Atheros port[%x] ",phy);
			for(reg = 0x100; reg < 0x120; reg++){
				printk(" register[%x] = 0x%x\n", reg + (phy << 8) , athrs17_reg_read(reg + (phy << 8)));
			}			
		}		
	}
#endif

    /*
     * Reason: Prevent WAN packets go through to LAN. The script is provided by Atheros.
     * Modified: Yufa Huang
     * Date: 2011.01.18
     */
    athrs17_reg_write(0x10, 0x40000000);
    athrs17_reg_write(0x90, 0);
    athrs17_reg_write(0x94, 0);
    athrs17_reg_write(0x3c, 0xc09d0012);
    athrs17_reg_write(0x3c, 0xc09e4c0c);
    athrs17_reg_write(0x3c, 0xc09d0000);
    athrs17_reg_write(0x3c, 0xc09e82ee);
    athrs17_reg_write(0x3c, 0xc09d0005);
    athrs17_reg_write(0x3c, 0xc09e3d46);
    athrs17_reg_write(0x3c, 0xc09d000b);
    athrs17_reg_write(0x3c, 0xc09ebc40);
    athrs17_reg_write(0x3c, 0);

    /*
     * Reason: Because the Atheros script set REG 0x3C to 0, so re-enable IGMP.
     * Modified: Yufa Huang
     * Date: 2011.01.18
     */
	/* Eanble IGMP snooping (to CPU port - port 0) */
	athrs17_reg_write(0x003c, athrs17_reg_read(0x003c) | (1 << 22));

    printk("********** athrs17_reg_init complete. **********\n");

    athr16_init_flag = 1;
}

/******************************************************************************
*
* athrs17_phy_is_link_alive - test to see if the specified link is alive
*
* RETURNS:
*    TRUE  --> link is alive
*    FALSE --> link is down
*/
static BOOL
athrs17_phy_is_link_alive(int phyUnit)
{
    uint16_t phyHwStatus;
    uint32_t phyBase;
    uint32_t phyAddr;

    phyBase = ATHR_PHYBASE(phyUnit);
    phyAddr = ATHR_PHYADDR(phyUnit);

    phyHwStatus = phy_reg_read(phyBase, phyAddr, ATHR_PHY_SPEC_STATUS);

    if (phyHwStatus & ATHR_STATUS_LINK_PASS)
        return TRUE;

    return FALSE;
}

/******************************************************************************
*
* athrs17_phy_setup - reset and setup the PHY associated with
* the specified MAC unit number.
*
* Resets the associated PHY port.
*
* RETURNS:
*    TRUE  --> associated PHY is alive
*    FALSE --> no LINKs on this ethernet unit
*/

BOOL
athrs17_phy_setup(int ethUnit)
{
    int       phyUnit;
    uint16_t  phyHwStatus;
    uint16_t  timeout;
    int       liveLinks = 0;
    uint32_t  phyBase = 0;
    BOOL      foundPhy = FALSE;
    uint32_t  phyAddr = 0;
    

    /* See if there's any configuration data for this enet */
    /* start auto negogiation on each phy */
    for (phyUnit=0; phyUnit < ATHR_PHY_MAX; phyUnit++) {
        if (!ATHR_IS_ETHUNIT(phyUnit, ethUnit)) {
            continue;
        }

        foundPhy = TRUE;
        phyBase = ATHR_PHYBASE(phyUnit);
        phyAddr = ATHR_PHYADDR(phyUnit);
        
        phy_reg_write(phyBase, phyAddr, ATHR_AUTONEG_ADVERT, ATHR_ADVERTISE_ALL);

        phy_reg_write(phyBase, phyAddr, ATHR_1000BASET_CONTROL, ATHR_ADVERTISE_1000FULL);

        /* Reset PHYs*/
        phy_reg_write(phyBase, phyAddr, ATHR_PHY_CONTROL, ATHR_CTRL_AUTONEGOTIATION_ENABLE | ATHR_CTRL_SOFTWARE_RESET);

    }

    if (!foundPhy) {
        return FALSE; /* No PHY's configured for this ethUnit */
    }

    /*
     * After the phy is reset, it takes a little while before
     * it can respond properly.
     */
    mdelay(1000);


    /*
     * Wait up to 3 seconds for ALL associated PHYs to finish
     * autonegotiation.  The only way we get out of here sooner is
     * if ALL PHYs are connected AND finish autonegotiation.
     */
    for (phyUnit=0; (phyUnit < ATHR_PHY_MAX) /*&& (timeout > 0) */; phyUnit++) {
        if (!ATHR_IS_ETHUNIT(phyUnit, ethUnit)) {
            continue;
        }

        timeout=20;
        for (;;) {
            phyHwStatus = phy_reg_read(phyBase, phyAddr, ATHR_PHY_CONTROL);

            if (ATHR_RESET_DONE(phyHwStatus)) {
                DRV_PRINT(DRV_DEBUG_PHYSETUP,
                          ("Port %d, Neg Success\n", phyUnit));
                break;
            }
            if (timeout == 0) {
                DRV_PRINT(DRV_DEBUG_PHYSETUP,
                          ("Port %d, Negogiation timeout\n", phyUnit));
                break;
            }
            if (--timeout == 0) {
                DRV_PRINT(DRV_DEBUG_PHYSETUP,
                          ("Port %d, Negogiation timeout\n", phyUnit));
                break;
            }

            mdelay(150);
        }
    }

    /*
     * All PHYs have had adequate time to autonegotiate.
     * Now initialize software status.
     *
     * It's possible that some ports may take a bit longer
     * to autonegotiate; but we can't wait forever.  They'll
     * get noticed by mv_phyCheckStatusChange during regular
     * polling activities.
     */
    for (phyUnit=0; phyUnit < ATHR_PHY_MAX; phyUnit++) {
        if (!ATHR_IS_ETHUNIT(phyUnit, ethUnit)) {
            continue;
        }

        if (athrs17_phy_is_link_alive(phyUnit)) {
            liveLinks++;
            ATHR_IS_PHY_ALIVE(phyUnit) = TRUE;
        } else {
            ATHR_IS_PHY_ALIVE(phyUnit) = FALSE;
        }

        DRV_PRINT(DRV_DEBUG_PHYSETUP, ("eth%d: Phy Specific Status=%4.4x\n", ethUnit, phy_reg_read(ATHR_PHYBASE(phyUnit), ATHR_PHYADDR(phyUnit), ATHR_PHY_SPEC_STATUS)));
    }
    
    return (liveLinks > 0);
}

/******************************************************************************
*
* athrs17_phy_is_fdx - Determines whether the phy ports associated with the
* specified device are FULL or HALF duplex.
*
* RETURNS:
*    1 --> FULL
*    0 --> HALF
*/
int
athrs17_phy_is_fdx(int ethUnit)
{
    int       phyUnit;
    uint32_t  phyBase;
    uint32_t  phyAddr;
    uint16_t  phyHwStatus;
    int       ii = 200;

    if (ethUnit == ENET_UNIT_LAN)
        return TRUE;

    for (phyUnit=0; phyUnit < ATHR_PHY_MAX; phyUnit++) {
        if (!ATHR_IS_ETHUNIT(phyUnit, ethUnit)) {
            continue;
        }

        if (athrs17_phy_is_link_alive(phyUnit)) {

            phyBase = ATHR_PHYBASE(phyUnit);
            phyAddr = ATHR_PHYADDR(phyUnit);

            do {
                phyHwStatus = phy_reg_read(phyBase, phyAddr, ATHR_PHY_SPEC_STATUS);
                mdelay(10);
            } while((!(phyHwStatus & ATHR_STATUS_RESOVLED)) && --ii);
            
            if (phyHwStatus & ATHER_STATUS_FULL_DEPLEX)
                return TRUE;
        }
    }

    return FALSE;
}


/******************************************************************************
*
* athrs17_phy_speed - Determines the speed of phy ports associated with the
* specified device.
*
* RETURNS:
*               AG7100_PHY_SPEED_10T, AG7100_PHY_SPEED_100TX;
*               AG7100_PHY_SPEED_1000T;
*/

int
athrs17_phy_speed(int ethUnit)
{
    int       phyUnit;
    uint16_t  phyHwStatus;
    uint32_t  phyBase;
    uint32_t  phyAddr;
    int       ii = 200;

    if (ethUnit == ENET_UNIT_LAN)
        return AG7100_PHY_SPEED_1000T;
        
    for (phyUnit=0; phyUnit < ATHR_PHY_MAX; phyUnit++) {
        if (!ATHR_IS_ETHUNIT(phyUnit, ethUnit)) {
            continue;
        }

        if (athrs17_phy_is_link_alive(phyUnit)) {

            phyBase = ATHR_PHYBASE(phyUnit);
            phyAddr = ATHR_PHYADDR(phyUnit);
            do {
		phyHwStatus = phy_reg_read(phyBase, phyAddr, ATHR_PHY_SPEC_STATUS);
                mdelay(10);
            }while((!(phyHwStatus & ATHR_STATUS_RESOVLED)) && --ii);
            
            phyHwStatus = ((phyHwStatus & ATHER_STATUS_LINK_MASK) >> ATHER_STATUS_LINK_SHIFT);

            switch(phyHwStatus) {
            case 0:
                return AG7100_PHY_SPEED_10T;
            case 1:
                return AG7100_PHY_SPEED_100TX;
            case 2:
                return AG7100_PHY_SPEED_1000T;
            default:
                printk("Unkown speed read!\n");
            }
        }
    }

    return AG7100_PHY_SPEED_10T;
}

/*****************************************************************************
*
* athr_phy_is_up -- checks for significant changes in PHY state.
*
* A "significant change" is:
*     dropped link (e.g. ethernet cable unplugged) OR
*     autonegotiation completed + link (e.g. ethernet cable plugged in)
*
* When a PHY is plugged in, phyLinkGained is called.
* When a PHY is unplugged, phyLinkLost is called.
*/

int
athrs17_phy_is_up(int ethUnit)
{
    int           phyUnit;
    uint16_t      phyHwStatus, phyHwControl;
    athrPhyInfo_t *lastStatus;
    int           linkCount   = 0;
    int           lostLinks   = 0;
    int           gainedLinks = 0;
    uint32_t      phyBase;
    uint32_t      phyAddr;

    for (phyUnit=0; phyUnit < ATHR_PHY_MAX; phyUnit++) {
        if (!ATHR_IS_ETHUNIT(phyUnit, ethUnit)) {
            continue;
        }

        phyBase = ATHR_PHYBASE(phyUnit);
        phyAddr = ATHR_PHYADDR(phyUnit);

        lastStatus = &athrPhyInfo[phyUnit];

        if (lastStatus->isPhyAlive) { /* last known link status was ALIVE */
            phyHwStatus = phy_reg_read(phyBase, phyAddr, ATHR_PHY_SPEC_STATUS);

            /* See if we've lost link */
            if (phyHwStatus & ATHR_STATUS_LINK_PASS) {
                linkCount++;
            } else {
                lostLinks++;
                DRV_PRINT(DRV_DEBUG_PHYCHANGE,("\nenet%d port%d down\n",
                                               ethUnit, phyUnit));
                lastStatus->isPhyAlive = FALSE;
            }
        } else { /* last known link status was DEAD */
            /* Check for reset complete */
            phyHwStatus = phy_reg_read(phyBase, phyAddr, ATHR_PHY_STATUS);
            if (!ATHR_RESET_DONE(phyHwStatus))
                continue;

            phyHwControl = phy_reg_read(phyBase, phyAddr, ATHR_PHY_CONTROL);
            /* Check for AutoNegotiation complete */            
            if ((!(phyHwControl & ATHR_CTRL_AUTONEGOTIATION_ENABLE)) 
                 || ATHR_AUTONEG_DONE(phyHwStatus)) {
                phyHwStatus = phy_reg_read(phyBase, phyAddr, ATHR_PHY_SPEC_STATUS);

                if (phyHwStatus & ATHR_STATUS_LINK_PASS) {
                gainedLinks++;
                linkCount++;
                DRV_PRINT(DRV_DEBUG_PHYCHANGE,("\nenet%d port%d up\n",
                                               ethUnit, phyUnit));
                lastStatus->isPhyAlive = TRUE;
                }
            }
        }
    }

    return (linkCount);

}

uint32_t
athrs17_reg_read(uint32_t reg_addr)
{
    uint32_t reg_word_addr;
    uint32_t phy_addr, tmp_val, reg_val;
    uint16_t phy_val;
    uint8_t phy_reg;

    /* change reg_addr to 16-bit word address, 32-bit aligned */
    reg_word_addr = (reg_addr & 0xfffffffc) >> 1;

    /* configure register high address */
    phy_addr = 0x18;
    phy_reg = 0x0;
    phy_val = (uint16_t) ((reg_word_addr >> 8) & 0x3ff);  /* bit16-8 of reg address */
    phy_reg_write(0, phy_addr, phy_reg, phy_val);

    /* For some registers such as MIBs, since it is read/clear, we should */
    /* read the lower 16-bit register then the higher one */

    /* read register in lower address */
    phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
    phy_reg = (uint8_t) (reg_word_addr & 0x1f);   /* bit4-0 of reg address */
    reg_val = (uint32_t) phy_reg_read(0, phy_addr, phy_reg);

    /* read register in higher address */
    reg_word_addr++;
    phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
    phy_reg = (uint8_t) (reg_word_addr & 0x1f);   /* bit4-0 of reg address */
    tmp_val = (uint32_t) phy_reg_read(0, phy_addr, phy_reg);
    reg_val |= (tmp_val << 16);

    return reg_val;   
}

void
athrs17_reg_write(uint32_t reg_addr, uint32_t reg_val)
{
    uint32_t reg_word_addr;
    uint32_t phy_addr;
    uint16_t phy_val;
    uint8_t phy_reg;

    /* change reg_addr to 16-bit word address, 32-bit aligned */
    reg_word_addr = (reg_addr & 0xfffffffc) >> 1;

    /* configure register high address */
    phy_addr = 0x18;
    phy_reg = 0x0;
    phy_val = (uint16_t) ((reg_word_addr >> 8) & 0x3ff);  /* bit16-8 of reg address */
    phy_reg_write(0, phy_addr, phy_reg, phy_val);

    /* For some registers such as ARL and VLAN, since they include BUSY bit */
    /* in higher address, we should write the lower 16-bit register then the */
    /* higher one */

    /* write register in lower address */
    phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
    phy_reg = (uint8_t) (reg_word_addr & 0x1f);   /* bit4-0 of reg address */
    phy_val = (uint16_t) (reg_val & 0xffff);
    phy_reg_write(0, phy_addr, phy_reg, phy_val);

    /* write register in higher address */
    reg_word_addr++;
    phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
    phy_reg = (uint8_t) (reg_word_addr & 0x1f);   /* bit4-0 of reg address */
    phy_val = (uint16_t) ((reg_val >> 16) & 0xffff);
    phy_reg_write(0, phy_addr, phy_reg, phy_val); 
}

int
athr_ioctl(uint32_t *args, int cmd)
{
#ifdef FULL_FEATURE
    if (sw_ioctl(args, cmd))
        return -EOPNOTSUPP;

    return 0;
#else
    printk("EOPNOTSUPP\n");
    return -EOPNOTSUPP;
#endif
}
