/*
 *	ipt_iftag - Netfilter module to match 'tag' value
 *
 *	(C) 2015 Vitaly Lavrov <vel21ripn@gnail.com>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/skbuff.h>

#include <net/dst.h>
#include <linux/sysctl.h>
#include <linux/inetdevice.h>
#include <linux/netfilter/x_tables.h>

// #include <linux/netfilter/ipt_iftag.h>
#include "ipt_iftag.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vitaly Lavrov <vel21ripn@gnail.com>");
MODULE_DESCRIPTION("Xtables: packet iftag operations");

# if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
# define xt_in(par)  par->in
# define xt_out(par)  par->out
#endif

static inline __u32 get_in_if(const struct net_device *dev) {
	if(!dev) return 0;
	else {
		struct in_device *in_dev = __in_dev_get_rcu(dev);
		return in_dev ? IN_DEV_IDTAG(in_dev) : 0;
	}
}

static bool
iftag_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_iftag_mtinfo *info = par->matchinfo;
	__u32 iiftag,oiftag,v1,v2;

	bool ret = false;
	iiftag = info->op & XT_IFTAG_IIF ? get_in_if(xt_in(par)):0;
	oiftag = info->op & XT_IFTAG_OIF ? get_in_if(xt_out(par)):0;
	v1 = 0; v2 = 0;
	if((info->op & XT_IFTAG_IF) == XT_IFTAG_IF) {
		v1 = iiftag; v2 = oiftag;
	} else {
		v1 = info->op & XT_IFTAG_IIF ? iiftag:oiftag;
		v2 = info->tag1;
	}
	if(info->op & XT_IFTAG_MASK) {
		v1 &= ~info->mask;
		if((info->op & XT_IFTAG_OPMASK) != XT_IFTAG_IN)
			v2 &= ~info->mask;
	}
	switch(info->op & XT_IFTAG_OPMASK) {
	  case XT_IFTAG_LT:
			ret = v1 < v2;
			break;
	  case XT_IFTAG_EQ:
			ret = v1 == v2;
			break;
	  case XT_IFTAG_GT:
			ret = v1 > v2;
			break;
	  case XT_IFTAG_IN:
			ret = v1 >= v2 && v1 <= info->tag2;
			break;
	}

	return ret ^ info->invert;
}

static int iftag_mt_errmsg(const char *msg) {
	if(msg)
		printk(KERN_ERR "iftag: %s\n",msg);
	return -EINVAL;
}

static int 
iftag_mt_check_v0(const struct xt_mtchk_param *par)
{
        const struct xt_iftag_mtinfo *info = par->matchinfo;

//	printk(KERN_INFO " op %x t1 %u t2 %u mask %u\n",
//			info->op,info->tag1,info->tag2,info->mask);
        if ( (info->op & XT_IFTAG_OPMASK) == XT_IFTAG_IN &&
             info->tag1 > info->tag2)
                return iftag_mt_errmsg("invalid range");
        if (!(info->op & XT_IFTAG_IF))
                return iftag_mt_errmsg("missing interface");
        if (!(info->op & XT_IFTAG_OP))
                return iftag_mt_errmsg("no op");
        if ((info->op & XT_IFTAG_MASK) && (info->op & XT_IFTAG_IF) != XT_IFTAG_IF) {
		if(info->tag1 & info->mask) return iftag_mt_errmsg("val & mask != 0");
		if((info->op & XT_IFTAG_OPMASK) == XT_IFTAG_IN &&
				(info->tag2 & info->mask)) return iftag_mt_errmsg("val2 & mask != 0");
	}
	switch(par->hook_mask) {
	case 1 << NF_INET_PRE_ROUTING:
	case 1 << NF_INET_LOCAL_IN:
		if(info->op & XT_IFTAG_OIF) return iftag_mt_errmsg("oif in prerouting/input");
		break;
	case 1 << NF_INET_POST_ROUTING:
	case 1 << NF_INET_LOCAL_OUT:
		if(info->op & XT_IFTAG_IIF) return iftag_mt_errmsg("iif in postrouting/output");
		break;
	}
        return 0;
}

static struct xt_match iftag_mt_reg __read_mostly = {
	.name           = "iftag",
	.family         = NFPROTO_IPV4,
	.match          = iftag_mt,
	.matchsize      = sizeof(struct xt_iftag_mtinfo),
	.checkentry	= iftag_mt_check_v0,
	.me             = THIS_MODULE
};

static int __init iftag_mt_init(void)
{
int err;
	printk(KERN_INFO "ipt_iftag loading..\n");
	err = xt_register_match(&iftag_mt_reg);
	if(err < 0) return err;

	printk(KERN_INFO "ipt_iftag OK.\n");
	return 0;
}

static void __exit iftag_mt_exit(void)
{
	printk(KERN_INFO "ipt_iftag unloading..\n");
	xt_unregister_match(&iftag_mt_reg);
	printk(KERN_INFO "ipt_iftag OK.\n");
}

module_init(iftag_mt_init);
module_exit(iftag_mt_exit);
