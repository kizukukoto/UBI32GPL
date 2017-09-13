/*
 *   Ubicom32 ethernet TIO interface driver.
 *
 * (C) Copyright 2009
 * Ubicom, Inc. www.ubicom.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <config.h>
#include <common.h>
#include <net.h>
#define extern
#include <asm/ip5000.h>
#undef extern
#include <asm/devtree.h>
#include <asm/errno.h>
#include <net.h>

#ifndef CONFIG_ETH_DEV
# define CONFIG_ETH_DEV 0
#endif

#include <asm/spinlock.h>
#define extern
#include "ubi32_eth.h"
#undef extern
#include "../sk98lin/u-boot_compat.h"

#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

void cleanup_skb(void);
struct devtree_node *lan_node = NULL;

#define NETDEV_TX_BUSY 1        /* driver tx path was busy*/
#define CONFIG_ZONE_DMA 1
#define wmb()  asm volatile (""   : : :"memory")
#define ENOMEM 2

extern unsigned long jiffies;

#define USE_POLLING 1

#define irqreturn_t int
#define IRQ_HANDLED 1
#define EINVAL 1
#define ETH_ALEN 6


#if 0
#define TX_DEBUG
#endif

#if 0
#define RX_DEBUG
#endif
extern unsigned char uip_buf[1502];
extern volatile unsigned short uip_len;

extern int eth_init(bd_t *bd);
extern void eth_halt(void);
extern int eth_rx(void);
extern int eth_send(volatile void *packet, int length);

static const char *eth_if_name[UBI32_ETH_NUM_OF_DEVICES] =
        {"eth_lan", "eth_wan", "eth_ultra"};

static struct eth_device *ubi32_eth_devices[UBI32_ETH_NUM_OF_DEVICES];

static u8_t mac_addr[UBI32_ETH_NUM_OF_DEVICES][ETH_ALEN] = {
        {0x00, 0x03, 0x64, 'l', 'a', 'n'},
        {0x00, 0x03, 0x64, 'w', 'a', 'n'},
        {0x00, 0x03, 0x64, 'd', 'b', 'g'}};

int eth_init_done;

struct ubi32_eth_private *netdev_priv(struct eth_device *dev) {
        return(dev->priv);
}

static inline struct sk_buff *ubi32_alloc_skb(struct eth_device *dev, unsigned int length)
{
        return alloc_skb(length, GFP_ATOMIC);
}

static inline void ubi32_eth_vp_int_set(struct eth_device *dev)
{
        struct ubi32_eth_private *priv = netdev_priv(dev);
        ubicom32_set_interrupt(priv->vp_int_bit);
}

static void ubi32_eth_vp_rxtx_enable(struct eth_device *dev)
{
        struct ubi32_eth_private *priv = netdev_priv(dev);
        priv->regs->command = UBI32_ETH_VP_CMD_RX_ENABLE | UBI32_ETH_VP_CMD_TX_ENABLE;
        priv->regs->int_mask = UBI32_ETH_VP_INT_RX | UBI32_ETH_VP_INT_TX;
        ubi32_eth_vp_int_set(dev);
}

static void ubi32_eth_vp_rxtx_stop(struct eth_device *dev)
{
        struct ubi32_eth_private *priv = netdev_priv(dev);
        priv->regs->command = 0;
        priv->regs->int_mask = 0;
        ubi32_eth_vp_int_set(dev);

        /* Wait for graceful shutdown */
        while (priv->regs->status & (UBI32_ETH_VP_STATUS_RX_STATE | UBI32_ETH_VP_STATUS_TX_STATE));
}

/*
 * ubi32_eth_tx_done()
 *      To avoid locking overhead, this is called by tasklet only.
 */
static int ubi32_eth_tx_done(struct eth_device *dev)
{
        struct ubi32_eth_private *priv;
        unsigned short tx_out;
        struct sk_buff *skb;
        volatile void *pdata;
        struct ubi32_eth_dma_desc *desc;

        priv = netdev_priv(dev);
        tx_out = priv->regs->tx_out;

        while (priv->tx_tail != tx_out) {
                pdata = priv->regs->tx_dma_ring[priv->tx_tail];
                BUG_ON(pdata == NULL);

                skb = container_of((void *)pdata, struct sk_buff, cb);
                desc = (struct ubi32_eth_dma_desc *)pdata;
                if (desc->status != UBI32_ETH_VP_TX_OK) {
                        atomic_inc((atomic_t *)&priv->stats.tx_errors);
                } else {
                        priv->stats.tx_packets++;
                        priv->stats.tx_bytes += skb->len;
                }
                dev_kfree_skb_any(skb);
                priv->regs->tx_dma_ring[priv->tx_tail] = NULL;
                priv->tx_tail = (priv->tx_tail + 1) & TX_DMA_RING_MASK;
        }

        if (priv->status & UBI32_ETH_VP_STATE_TX_Q_FULL) {
                ubi32_spin_lock(&priv->lock);
                priv->status &= ~UBI32_ETH_VP_STATE_TX_Q_FULL;
                ubi32_eth_vp_int_set(dev);
                ubi32_spin_unlock(&priv->lock);
        }
#ifdef TX_DEBUG
        printf("TX completed\n");
#endif
        return 0;
}

/*
 * ubi32_eth_receive()
 *      To avoid locking overhead, this is called only
 *      by tasklet when not using NAPI, or
 *      by NAPI poll when using NAPI.
 */
static int ubi32_eth_receive(struct eth_device *dev, int quota)
{
        struct ubi32_eth_private *priv = netdev_priv(dev);
        unsigned short rx_in = priv->regs->rx_in;
        unsigned short rx_out = priv->regs->rx_out;
        struct sk_buff *skb;
        struct ubi32_eth_dma_desc *desc = NULL;
        volatile void *pdata;

        int i, replenish_cnt, count = 0;
        int replenish_max = RX_DMA_MAX_QUEUE_SIZE;
        if (CONFIG_ZONE_DMA && (dev == ubi32_eth_devices[0]))
                replenish_max = 21;

        while (priv->rx_tail != rx_out) {
                if (count == quota) {
                        /* There is still frame pending to be processed */
                        priv->vp_stats.rx_throttle++;
                        break;
                }

                pdata = priv->regs->rx_dma_ring[priv->rx_tail];
                BUG_ON(pdata == NULL);

                desc = (struct ubi32_eth_dma_desc *)pdata;
                skb = container_of((void *)pdata, struct sk_buff, cb);

                count++;
                priv->regs->rx_dma_ring[priv->rx_tail] = NULL;
                priv->rx_tail = ((priv->rx_tail + 1) & RX_DMA_RING_MASK);

                if (!(desc->status & UBI32_ETH_VP_RX_OK)) {
                        priv->stats.rx_errors++;
                        dev_kfree_skb_any(skb);
                        continue;
                }

                skb_put(skb, desc->data_len);

                NetReceive(skb->data, skb->len);

                memset(uip_buf,0,sizeof(uip_buf));

                memcpy(uip_buf,skb->data,sizeof(uip_buf));
                //memmove(uip_buf+12,uip_buf+16,sizeof(uip_buf)-12);
                uip_len=sizeof(uip_buf);

                /*int i=0;
                for(i=0;i<30;i++)
                        printf("uip_buf[%d]=0x%x\n",i,uip_buf[i]);
                */

                dev_kfree_skb_any(skb);
        }

        /* fill in more descripor for VP*/
        replenish_cnt =  replenish_max -
                ((RX_DMA_RING_SIZE + rx_in - priv->rx_tail) & RX_DMA_RING_MASK);
        if (replenish_cnt > 0) {
                for (i = 0; i < replenish_cnt; i++) {
                        skb = ubi32_alloc_skb(dev, RX_BUF_SIZE + CACHE_LINE_SIZE);
                        if (!skb) {
                                priv->vp_stats.rx_alloc_err++;
                                break;
                        }
                        /* set up dma descriptor */
                        desc = (struct ubi32_eth_dma_desc *)skb->cb;
                        BUG_ON((CACHE_LINE_SIZE - NET_IP_ALIGN) < ((int)skb->data & (CACHE_LINE_SIZE - 1)));
                        int extra_reserve = CACHE_LINE_SIZE - NET_IP_ALIGN - ((int)skb->data & (CACHE_LINE_SIZE - 1)); /* align the IP header */
                        skb_reserve(skb, extra_reserve);
                        desc->data_pointer = skb->data;
                        desc->buffer_len = RX_BUF_SIZE + CACHE_LINE_SIZE - extra_reserve;
                        desc->data_len = 0;
                        desc->status = 0;

                        priv->regs->rx_dma_ring[rx_in] = desc;
                        rx_in = (rx_in + 1) & RX_DMA_RING_MASK;
                }
                wmb();
                priv->regs->rx_in = rx_in;
                ubi32_eth_vp_int_set(dev);
        }

        if (count > 0) {
                priv->last_rx = jiffies;
        }

#ifdef RX_DEBUG
        printf("rx poll\n");
#endif

        /* return TRUE when all RX frames are processed */
        return (priv->rx_tail == rx_out);
}

static void ubi32_eth_do_tasklet(unsigned long arg)
{
        struct eth_device *dev = (struct eth_device *)arg;
        struct ubi32_eth_private *priv = netdev_priv(dev);

        priv->vp_stats.tasklet_run_cnt++;
        if (priv->regs->rx_in == priv->regs->rx_out)
                priv->vp_stats.rx_q_full_cnt++;

        priv->regs->int_mask |= UBI32_ETH_VP_INT_TX;
        priv->regs->int_status &= ~UBI32_ETH_VP_INT_TX;
        if (priv->tx_tail != priv->regs->tx_out) {
                ubi32_eth_tx_done(dev);
        }

        /* always call receive to process new RX frame as well as replenish RX buffers */
        priv->regs->int_mask |= UBI32_ETH_VP_INT_RX;
        priv->regs->int_status &= ~UBI32_ETH_VP_INT_RX;
        if (!ubi32_eth_receive(dev, RX_BOUND)) {
                priv->regs->int_mask &= ~UBI32_ETH_VP_INT_RX;
                priv->vp_stats.tasklet_resch_cnt++;
        }
        return;
}

#if defined(USE_POLLING)

int eth_rx(void)
{
        struct eth_device *dev;
//      int i;

//      for (i = 0; i < UBI32_ETH_NUM_OF_DEVICES; i++) {
                dev = ubi32_eth_devices[CONFIG_ETH_DEV];        //i
                if (dev) {      // && (dev->flags & IFF_UP)) {
                        ubi32_eth_do_tasklet((unsigned long)dev);
                }
//      }

        return 1;
}
#endif

static int ubi32_eth_start_xmit(struct sk_buff *skb, struct eth_device *dev)
{
        struct ubi32_eth_private *priv = netdev_priv(dev);
        struct ubi32_eth_dma_desc *desc = NULL;
        unsigned short space, tx_in;
        tx_in = priv->regs->tx_in;

//      dev->trans_start = jiffies; /* save the timestamp */
        space = TX_DMA_RING_MASK - ((TX_DMA_RING_SIZE + tx_in - priv->tx_tail) & TX_DMA_RING_MASK);
        if (space == 0) {
                atomic_inc((atomic_t *)&priv->stats.tx_errors);
                if (!(priv->status & UBI32_ETH_VP_STATE_TX_Q_FULL)) {
                        ubi32_spin_lock(&priv->lock);
                        priv->status |= UBI32_ETH_VP_STATE_TX_Q_FULL;
                        priv->vp_stats.tx_q_full_cnt++;
                        ubi32_spin_unlock(&priv->lock);
                }
#ifdef TX_DEBUG
                printf("tx not started busy\n");
#endif
                return NETDEV_TX_BUSY;
        }

        /*still have room */
        desc = (struct ubi32_eth_dma_desc *)skb->cb;
        desc->data_pointer = skb->data;
        desc->data_len = skb->len;
        priv->regs->tx_dma_ring[tx_in] = desc;
        tx_in = ((tx_in + 1) & TX_DMA_RING_MASK);
        //wmb();
        priv->regs->tx_in = tx_in;
        /* kick the HRT */
        ubi32_eth_vp_int_set(dev);

#ifdef TX_DEBUG
        printf("tx started\n");
#endif
        return 0;
}

/*
 * ubi32_eth_cleanup: unload the module
 */
void ubi32_eth_cleanup(void)
{
        struct ubi32_eth_private *priv;
        struct eth_device *dev;
        int i;

        for (i = 0; i < UBI32_ETH_NUM_OF_DEVICES; i++) {
                dev = ubi32_eth_devices[i];
                if (dev) {
                        ubi32_eth_vp_rxtx_stop(dev);
                        priv = netdev_priv(dev);
                        if (priv) {
                                if (priv->regs->tx_dma_ring) {
                                        free(priv->regs->tx_dma_ring);
                                }
                                free(priv);
                        }
                        free(dev);
                        ubi32_eth_devices[i] = NULL;
                }
        }

        cleanup_skb();
}

int ubi32_eth_init_module(void)
{
        struct ethtionode *eth_node;
        struct eth_device *dev;
        struct ubi32_eth_private *priv;
        int i, err;

        cleanup_skb();

        /*
         * Device allocation.
         */
        err = 0;
        for (i = 0; i < UBI32_ETH_NUM_OF_DEVICES; i++) {
                /*
                 * See if the eth_vp is in the device tree.
                 */
                eth_node = (struct ethtionode *)devtree_find_node(eth_if_name[i]);
                if (!eth_node) {
                        printk("%s Ethernet device does not exist\n", eth_if_name[i]);
                        continue;
                }

                eth_node->tx_dma_ring = (struct ubi32_eth_dma_desc **)malloc(
                                sizeof(struct ubi32_eth_dma_desc *) *
                                (TX_DMA_RING_SIZE + RX_DMA_RING_SIZE));
                if (!eth_node->tx_dma_ring) {
                        err = -ENOMEM;
                        break;
                }
                eth_node->rx_dma_ring = eth_node->tx_dma_ring + TX_DMA_RING_SIZE;
                eth_node->tx_sz = TX_DMA_RING_SIZE - 1;
                eth_node->rx_sz = RX_DMA_RING_SIZE - 1;

                /*
                 * Initialize eth devices
                 */
                dev = malloc(sizeof(struct eth_device));
                if (!dev) {
                        err = -ENOMEM;
                        break;
                }

                priv = malloc(sizeof(struct ubi32_eth_private));
                if (!dev) {
                        err = -ENOMEM;
                        break;
                }

                memcpy(dev->name, eth_if_name[i], sizeof dev->name - 1);
                dev->priv = priv;
                priv->dev = dev;

                /*
                 * FIX IT! get proper mac address
                 */
                memcpy(priv->mac_addr, mac_addr[i], ETH_ALEN);

                priv->regs = eth_node;
                priv->regs->command = 0;
                priv->regs->int_mask = 0;
                priv->regs->int_status = 0;
                priv->regs->tx_out = 0;
                priv->regs->rx_out = 0;
                priv->regs->tx_in = 0;
                priv->regs->rx_in = 0;
                priv->rx_tail = 0;
                priv->tx_tail = 0;

                priv->vp_int_bit = eth_node->dn.sendirq;
                priv->irq = eth_node->dn.recvirq;

                ubi32_spin_lock_init(&priv->lock);

                ubi32_eth_devices[i] = dev;
                printk(KERN_INFO "%s Ethernet device found. vp_base:0x%p, tio_int:%d irq:%d\n",
                        dev->name, priv->regs, eth_node->dn.sendirq, eth_node->dn.recvirq);

                /* XXX missing stop atm */
                ubi32_eth_vp_rxtx_enable(dev);
        }

        if (err) {
                ubi32_eth_cleanup();
                return err;
        }

        if (!ubi32_eth_devices[0] && !ubi32_eth_devices[1] && !ubi32_eth_devices[2]) {
                return -ENODEV;
        }

        printf("ubi32_eth_init_module completed\n");

        return 0;
}

void eth_halt(void)
{

#if 0
        /*
         * Don't cleanup, the TIO gets in a bad state apparently which we
         * need to fix but not for the first release.
         */
        ubi32_eth_cleanup();
#endif
}

int eth_send(volatile void *packet, int len)
{
        /* use the WAN interface until we can configure the switch */
        struct sk_buff *skb = ubi32_alloc_skb(ubi32_eth_devices[CONFIG_ETH_DEV], len);
        if (skb) {
#ifdef TX_DEBUG
                printf("%s %x:%x\n", ubi32_eth_devices[CONFIG_ETH_DEV]->name, (u32)ubi32_eth_devices[CONFIG_ETH_DEV], (u32)ubi32_eth_devices[CONFIG_ETH_DEV]->priv);
#endif
                memcpy(skb->data, (void *)packet, len);
                ubi32_eth_start_xmit(skb, ubi32_eth_devices[CONFIG_ETH_DEV]);
                return 0;
        } else {
                return 1;
        }
}

int eth_init(bd_t *bd)
{
        if (!eth_init_done) {
                ubi32_eth_init_module();
                eth_init_done = 1;
        }

        return 0;
}

