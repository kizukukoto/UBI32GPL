/*
 *	Generic parts
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
/*
 * Reason: Ubicom patch for wlan TP
 * Modified: John Huang
 * Date: 2010.04.01
 */
#include <linux/proc_fs.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/llc.h>
#include <net/llc.h>
#include <net/stp.h>

#include "br_private.h"

int (*br_should_route_hook)(struct sk_buff *skb);

static const struct stp_proto br_stp_proto = {
	.rcv	= br_stp_rcv,
};

static struct pernet_operations br_net_ops = {
	.exit	= br_net_exit,
};

/*
 * Reason: Ubicom patch for wlan TP
 * Modified: John Huang
 * Date: 2010.04.01
 */
#ifdef CONFIG_UBICOM32
#ifdef CONFIG_PROC_FS
static int nf_br_wish_enable_proc_help(char *buf, char **start, off_t offset, int count,
                int *eof, void *data)
{
     int len = 0;

     if (count > 128)
             len = sprintf(buf,
               "bridge netfilter for wish: write 0 to disable; write 1 to enable\n");

     return len;
}

static int nf_br_wish_enable_proc_write(struct file *file, const char __user *buffer,
                           unsigned long count, void *data)
{
#ifdef CONFIG_BRIDGE_NETFILTER
     char v;

     if (copy_from_user(&v, buffer, 1))
                return -EFAULT;

     if (v == '0') {
             br_netfilter_fini();
             printk(KERN_INFO "Disable bridge netfilter\n");
     } else if (v == '1') {
             int err;
             err = br_netfilter_init();
             printk(KERN_INFO "Enable bridge netfilter (erno = %d)\n", err);
             if (err)
                     return err;
     } else {
             return -EINVAL;
     }
#endif
     return count;
}

static int __init br_nf_wish_proc_init(void)
{
     struct proc_dir_entry *ptr;
     ptr = create_proc_entry("br_nf_wish", S_IFREG | S_IRUGO, NULL);
     if (!ptr) {
             printk(KERN_WARNING "unable to create /proc/br_nf_wish\n");
             return -1;
     }
     ptr->owner = THIS_MODULE;
     ptr->read_proc = nf_br_wish_enable_proc_help;
     ptr->write_proc = nf_br_wish_enable_proc_write;
     return 0;
}
#endif       // PROC_FS
#endif       // UBICOM32

static int __init br_init(void)
{
	int err;

	err = stp_proto_register(&br_stp_proto);
	if (err < 0) {
		printk(KERN_ERR "bridge: can't register sap for STP\n");
		return err;
	}

	err = br_fdb_init();
	if (err)
		goto err_out;

	err = register_pernet_subsys(&br_net_ops);
	if (err)
		goto err_out1;

/*
 * Reason: Ubicom patch for wlan TP
 * Modified: John Huang
 * Date: 2010.04.01
 */
#ifdef CONFIG_UBICOM32
#ifdef CONFIG_PROC_FS
	err = br_nf_wish_proc_init();
	if (!err)
#endif
#endif
	err = br_netfilter_init();
	if (err)
		goto err_out2;

	err = register_netdevice_notifier(&br_device_notifier);
	if (err)
		goto err_out3;

	err = br_netlink_init();
	if (err)
		goto err_out4;

	brioctl_set(br_ioctl_deviceless_stub);
	br_handle_frame_hook = br_handle_frame;

	br_fdb_get_hook = br_fdb_get;
	br_fdb_put_hook = br_fdb_put;

	return 0;
err_out4:
	unregister_netdevice_notifier(&br_device_notifier);
err_out3:
	br_netfilter_fini();
err_out2:
	unregister_pernet_subsys(&br_net_ops);
err_out1:
	br_fdb_fini();
err_out:
	stp_proto_unregister(&br_stp_proto);
	return err;
}

static void __exit br_deinit(void)
{
	stp_proto_unregister(&br_stp_proto);

	br_netlink_fini();
	unregister_netdevice_notifier(&br_device_notifier);
	brioctl_set(NULL);

	unregister_pernet_subsys(&br_net_ops);

	synchronize_net();

	br_netfilter_fini();
	br_fdb_get_hook = NULL;
	br_fdb_put_hook = NULL;

	br_handle_frame_hook = NULL;
	br_fdb_fini();
}

EXPORT_SYMBOL(br_should_route_hook);

module_init(br_init)
module_exit(br_deinit)
MODULE_LICENSE("GPL");
MODULE_VERSION(BR_VERSION);
