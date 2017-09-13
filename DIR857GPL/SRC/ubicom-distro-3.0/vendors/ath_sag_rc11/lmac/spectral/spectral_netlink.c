/*
 * Copyright (c) 2010, Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#if ATH_SUPPORT_SPECTRAL
#include <osdep.h>
#include "ah.h"
#include "spectral_data.h"
#include "spec_msg_proto.h"
#include "spectral.h"

#ifndef WIN32
struct sock *spectral_nl_sock;
static atomic_t spectral_nl_users = ATOMIC_INIT(0);
int spectral_init_netlink(struct ath_softc *sc)
{
    struct ath_spectral *spectral=sc->sc_spectral;
    
    if (spectral == NULL) {
		SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL, "%s: sc_spectral is NULL\n",
			__func__);
		return -EIO;
    }
    spectral->spectral_sent_msg=0;
    if (spectral_nl_sock == NULL) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
        spectral_nl_sock = (struct sock *)netlink_kernel_create(NETLINK_ATHEROS, 1,&spectral_nl_data_ready, THIS_MODULE);
#else
        extern struct net init_net;
        spectral_nl_sock = (struct sock *)netlink_kernel_create(&init_net,NETLINK_ATHEROS, 1,&spectral_nl_data_ready, NULL, THIS_MODULE);
#endif
        if (spectral_nl_sock == NULL) {
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL,"%s NETLINK_KERNEL_CREATE FAILED\n", __func__);
            return -ENODEV;
        }
    }
    atomic_inc(&spectral_nl_users);
    spectral->spectral_sock = spectral_nl_sock;

    if ((spectral==NULL) || (spectral->spectral_sock==NULL)) {
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"%s NULL pointers (spectral=%d) (sock=%d) \n", __func__, (spectral==NULL),(spectral->spectral_sock==NULL));
        return -ENODEV;
    }
   if (spectral->spectral_skb == NULL) {
             SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"%s %d NULL SKB\n", __func__, __LINE__);
    }
    
    return 0;
    
}

int spectral_destroy_netlink(struct ath_softc *sc)
{
    struct ath_spectral *spectral=sc->sc_spectral;

    spectral->spectral_sock = NULL;
    if (atomic_dec_and_test(&spectral_nl_users)) {
        sock_release(spectral_nl_sock->sk_socket);
        spectral_nl_sock = NULL;
    }
    return 0;
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
void spectral_nl_data_ready(struct sock *sk, int len)
{
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL3,"%s %d\n", __func__, __LINE__);
        return;
}

#else
void spectral_nl_data_ready(struct sk_buff *skb )
{
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL3,"%s %d\n", __func__, __LINE__);
        return;
}

#endif /* VERSION*/
#endif

void spectral_create_samp_msg(struct samp_msg_params *params)
{
    static SPECTRAL_SAMP_MSG spec_samp_msg; /* XXX non-reentrant - will be an issue with dual concurrent operation on multi-proc systems */
    SPECTRAL_SAMP_MSG *msg=NULL;
    SPECTRAL_SAMP_DATA *data=NULL;
    u_int8_t *bin_pwr_data = NULL;
    static int samp_msg_index=0, i=0;
    struct ath_softc *sc = params->sc;
    struct ath_spectral *spectral=sc->sc_spectral;
    SPECTRAL_CLASSIFIER_PARAMS *cp, *pcp;

    //8-13-2008 Why are the last few bytes missing? Check if our samp_msg_len is screwing up.
    int samp_msg_len = sizeof(SPECTRAL_SAMP_MSG);
    int temp_samp_msg_len = sizeof(SPECTRAL_SAMP_MSG) - (MAX_NUM_BINS * sizeof(u_int8_t));
    temp_samp_msg_len += (params->pwr_count * sizeof(u_int8_t));

    bin_pwr_data = *(params->bin_pwr_data);

    memset(&spec_samp_msg, 0, sizeof(SPECTRAL_SAMP_MSG));

    data = &spec_samp_msg.samp_data;

        spec_samp_msg.freq = params->freq;
        spec_samp_msg.freq_loading = params->freq_loading;
        spec_samp_msg.samp_data.spectral_data_len = params->datalen;
        spec_samp_msg.samp_data.spectral_rssi = params->rssi;
        spec_samp_msg.samp_data.spectral_combined_rssi = (u_int8_t)params->rssi;
        spec_samp_msg.samp_data.spectral_upper_rssi = params->upper_rssi;
        spec_samp_msg.samp_data.spectral_lower_rssi = params->lower_rssi;
        spec_samp_msg.samp_data.spectral_bwinfo = params->bwinfo;
        spec_samp_msg.samp_data.spectral_tstamp = params->tstamp;
        spec_samp_msg.samp_data.spectral_max_index = params->max_index;

        /* Classifier in user space needs access to these */
        spec_samp_msg.samp_data.spectral_lower_max_index = params->max_lower_index;
        spec_samp_msg.samp_data.spectral_upper_max_index = params->max_upper_index;
        spec_samp_msg.samp_data.spectral_nb_lower= params->nb_lower;
        spec_samp_msg.samp_data.spectral_nb_upper = params->nb_upper;
        spec_samp_msg.samp_data.spectral_last_tstamp = params->last_tstamp;


        spec_samp_msg.samp_data.spectral_max_mag = params->max_mag;
        spec_samp_msg.samp_data.bin_pwr_count = params->pwr_count;

        spec_samp_msg.samp_data.spectral_combined_rssi = params->rssi;
        //Reusing max_exp as max_scale
        spec_samp_msg.samp_data.spectral_max_scale = params->max_exp;

        /* Classifier in user space needs access to these */
        cp = &(spec_samp_msg.samp_data.classifier_params);
        pcp = &(params->classifier_params);

        OS_MEMCPY(cp, pcp, sizeof(SPECTRAL_CLASSIFIER_PARAMS));

        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"bin_pwr_data=%p\n", bin_pwr_data);

#ifdef REVERSE_ORDER
        OS_MEMCPY(&data->bin_pwr[0], (bin_pwr_data+64), 64);
        OS_MEMCPY(&data->bin_pwr[64], (bin_pwr_data), 64);
#else
        /* 8-22 Reverse the order of the bytes being passed up */
        OS_MEMCPY(&data->bin_pwr[0], bin_pwr_data, params->pwr_count);
#endif
        for (i=0; i < params->pwr_count; i++) {
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"(%d)[%x:%x] ",i,*(bin_pwr_data+i), data->bin_pwr[i]);
        }
#ifdef SPECTRAL_CLASSIFIER_IN_KERNEL        
        if(params->interf_list.count) {
            OS_MEMCPY(&data->interf_list, &params->interf_list, sizeof(struct INTERF_SRC_RSP));
        } else 
#endif
        data->interf_list.count = 0;
#ifndef WIN32
        spectral_prep_skb(sc);
        if (spectral->spectral_skb != NULL) {
            spectral->spectral_nlh = (struct nlmsghdr*)spectral->spectral_skb->data;
            memcpy(NLMSG_DATA(spectral->spectral_nlh), &spec_samp_msg, NLMSG_SPACE(samp_msg_len));
            msg = (SPECTRAL_SAMP_MSG*) NLMSG_DATA(spectral->spectral_nlh);
            print_samp_msg (msg, sc);
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"nlmsg_len=%d skb->len=%d\n", spectral->spectral_nlh->nlmsg_len, spectral->spectral_skb->len);
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,"%s", ".");
            spectral_bcast_msg(sc);
            samp_msg_index++;
        }
#else 
        /* call the indicate function to pass the data to Windows spectral service*/
		msg = (SPECTRAL_SAMP_MSG *)
            OS_MALLOC(sc->sc_osdev, sizeof(SPECTRAL_SAMP_MSG), GFP_KERNEL); 
        if (msg) {
            OS_MEMCPY(msg, &spec_samp_msg, sizeof(SPECTRAL_SAMP_MSG));
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,
                " datalen=%d tstamp=%x last_tstamp=%x "
                "rssi=%d nb_lower=%d peak=%d\n",
                spec_samp_msg.samp_data.spectral_data_len,
                spec_samp_msg.samp_data.spectral_tstamp,
                spec_samp_msg.samp_data.spectral_last_tstamp,
                spec_samp_msg.samp_data.spectral_lower_rssi,
                spec_samp_msg.samp_data.spectral_nb_lower,
                spec_samp_msg.samp_data.spectral_lower_max_index);
            SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL1,
                "datalen=%d sizeof(SPECTRAL_SAMP_MSG)=%d\n",
                params->datalen, sizeof(SPECTRAL_SAMP_MSG));
            ath_spectral_indicate(
                params->sc, (void*)msg, sizeof(SPECTRAL_SAMP_MSG));
            OS_FREE(msg);
            msg = NULL;
        } else {
            SPECTRAL_DPRINTK(sc, 0 /* unfiltered */,
                "Couldn't allocate msg buffer!\n");
        }
#endif
}


#ifndef WIN32

void spectral_prep_skb(struct ath_softc *sc)
{
    struct ath_spectral *spectral=sc->sc_spectral;

    spectral->spectral_skb=dev_alloc_skb(MAX_SPECTRAL_PAYLOAD);

    if (spectral->spectral_skb != NULL) {

        skb_put(spectral->spectral_skb, MAX_SPECTRAL_PAYLOAD);
        spectral->spectral_nlh = (struct nlmsghdr*)spectral->spectral_skb->data;
        // Possible bug that size of  SPECTRAL_SAMP_MSG and SPECTRAL_MSG differ by 3 bytes  so we miss 3 bytes        
        spectral->spectral_nlh->nlmsg_len = NLMSG_SPACE(sizeof(SPECTRAL_SAMP_MSG));
        spectral->spectral_nlh->nlmsg_pid = 0;
        spectral->spectral_nlh->nlmsg_flags = 0;
    } else {
        spectral->spectral_skb = NULL;
        spectral->spectral_nlh = NULL;
    }
}

void spectral_unicast_msg(struct ath_softc *sc)
{
    struct ath_spectral *spectral=sc->sc_spectral;

    if ((spectral==NULL) || (spectral->spectral_sock==NULL)) {
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"%s NULL pointers (spectral=%d) (sock=%d) (skb=%d)\n", __func__, (spectral==NULL),(spectral->spectral_sock==NULL),(spectral->spectral_skb==NULL));
        dev_kfree_skb(spectral->spectral_skb);
        spectral->spectral_skb = NULL;
        return;
    }
    

    if (spectral->spectral_skb != NULL) {

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
        NETLINK_CB(spectral->spectral_skb).pid = 0;  /* from kernel */
        NETLINK_CB(spectral->spectral_skb).dst_pid = spectral->spectral_pid;
        NETLINK_CB(spectral->spectral_skb).pid = 0;  /* from kernel */
#endif /* VERSION - field depracated by newer kernel */
        NETLINK_CB(spectral->spectral_skb).pid = 0;  /* from kernel */
     /* to mcast group 1<<0 */
        NETLINK_CB(spectral->spectral_skb).dst_group=0;;

        netlink_unicast(spectral->spectral_sock, spectral->spectral_skb, spectral->spectral_pid, MSG_DONTWAIT);
    }
}

void spectral_bcast_msg(struct ath_softc *sc)
{
    struct ath_spectral *spectral=sc->sc_spectral;
    SPECTRAL_SAMP_MSG *msg=NULL;
    fd_set write_set; 
    struct nlmsghdr *nlh=NULL;
    int status;
    FD_ZERO(&write_set);

    if ((spectral==NULL) || (spectral->spectral_sock==NULL)) {
        SPECTRAL_DPRINTK(sc, ATH_DEBUG_SPECTRAL2,"%s NULL pointers (spectral=%d) (sock=%d) (skb=%d)\n", __func__, (spectral==NULL),(spectral->spectral_sock==NULL),(spectral->spectral_skb==NULL));
        dev_kfree_skb(spectral->spectral_skb);
        spectral->spectral_skb = NULL;
        return;
    }
    
        nlh = (struct nlmsghdr*)spectral->spectral_skb->data;
        msg = (SPECTRAL_SAMP_MSG*) NLMSG_DATA(spectral->spectral_nlh);
        print_samp_msg (msg, sc);
        
        status = netlink_broadcast(spectral->spectral_sock, spectral->spectral_skb, 0,1, GFP_ATOMIC);
        if (status == 0) {
            spectral->spectral_sent_msg++;
            //if (spectral->spectral_sent_msg % 10000 == 0) printk("sent 10000 netlink msgs\n");
        }

        /* netlink will have freed the skb */
        if (spectral->spectral_skb) {
            spectral->spectral_skb=NULL;
        }
}

#endif
#endif
