/* 
 * libxt_iftag.c
 * Copyright (C) 2015 Vitaly Lavrov <vel21ripn@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <xtables.h>

#include <linux/version.h>

#include "ipt_iftag.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
#endif

static void 
__iftag_mt4_save(const void *entry, const struct xt_entry_match *match,char *part)
{
	const struct xt_iftag_mtinfo *info = (const void *)match->data;
	char *v1,*v2,*vop,b1[32],mask[32];

	b1[0] = '\0';
	mask[0] = '\0';

	if((info->op & XT_IFTAG_IF) == XT_IFTAG_IF) {
		v1 = "iif";
		v2 = "oif";
	} else {
		v1 = info->op & XT_IFTAG_IIF ? "iif":"oif";
		v2 = &b1[0];
		if((info->op & XT_IFTAG_OPMASK) == XT_IFTAG_IN)
			snprintf(b1,sizeof(b1)-1,"0x%x-0x%x",info->tag1,info->tag2);
		    else
			snprintf(b1,sizeof(b1)-1,"0x%x",info->tag1);
	}
	if(info->op & XT_IFTAG_MASK)
		snprintf(mask,sizeof(mask)-1,"/0x%x",info->mask);

        switch(info->op & XT_IFTAG_OPMASK) {
          case XT_IFTAG_LT: vop = "lt";  break;
          case XT_IFTAG_EQ: vop = "eq";  break;
          case XT_IFTAG_GT: vop = "gt";  break;
          case XT_IFTAG_IN: vop = "in";  break;
	  default: vop = "?";
        }

	printf(" %s%s %s %s %s%s ",info->invert ? "! ":"" , part,v1,vop,v2,mask);
}

static void 
iftag_mt4_save(const void *entry, const struct xt_entry_match *match)
{
__iftag_mt4_save(entry,match,"--tag");
}

static void 
iftag_mt4_print(const void *entry, const struct xt_entry_match *match,
                  int numeric)
{
__iftag_mt4_save(entry,match,"tag");
}

#define STREQ(a,b) !strcmp(a,b)

#ifndef xtables_error
#define xtables_error exit_error
#endif


static int 
iftag_mt4_parse(int c, char **argv, int invert, unsigned int *flags,
                  const void *entry, struct xt_entry_match **match)
{
	struct xt_iftag_mtinfo *info = (void *)(*match)->data;
	char *vop,*v2,*v2c;

        *flags = 0;
	info->invert = invert;
	
	vop = strchr(optarg,',');
	if(vop) *vop++ = '\0';
	  else xtables_error(PARAMETER_PROBLEM,"iftag: missing operator");

	v2 = strchr(vop,',');
	if(v2) *v2++ = '\0';
	  else xtables_error(PARAMETER_PROBLEM,"iftag: missing right operand");


	if(STREQ(optarg,"iif")) {
		info->op |= XT_IFTAG_IIF;
		*flags   |= XT_IFTAG_IIF;
	}
	if(STREQ(optarg,"oif")) {
		info->op |= XT_IFTAG_OIF;
		*flags   |= XT_IFTAG_OIF;
	}
	if(!(info->op & XT_IFTAG_IF))
		xtables_error(PARAMETER_PROBLEM,"iftag: missing iif or oif");

	if(STREQ(vop,"lt")) {
		info->op |= XT_IFTAG_LT|XT_IFTAG_OP;
		*flags   |= XT_IFTAG_LT|XT_IFTAG_OP;
	}
	if(STREQ(vop,"eq")) {
		info->op |= XT_IFTAG_EQ|XT_IFTAG_OP;
		*flags   |= XT_IFTAG_EQ|XT_IFTAG_OP;
	}
	if(STREQ(vop,"gt")) {
		info->op |= XT_IFTAG_GT|XT_IFTAG_OP;
		*flags   |= XT_IFTAG_GT|XT_IFTAG_OP;
	}
	if(STREQ(vop,"in")) {
		info->op |= XT_IFTAG_IN|XT_IFTAG_OP;
		*flags   |= XT_IFTAG_IN|XT_IFTAG_OP;
	}
	if(!(info->op & XT_IFTAG_OP))
		xtables_error(PARAMETER_PROBLEM,"iftag: missing operator");

	v2c = strchr(v2,'/');
	if(v2c) {
		*v2c++ = '\0';
		if(sscanf(v2c,"%i",&info->mask) != 1)
			xtables_error(PARAMETER_PROBLEM,"iftag: invalid mask");
		info->op |= XT_IFTAG_MASK;
		*flags   |= XT_IFTAG_MASK;
	}
	if(STREQ(v2,"oif")) {
		if(info->op & XT_IFTAG_OIF)
			xtables_error(PARAMETER_PROBLEM,"iftag: double oif");
		info->op |= XT_IFTAG_OIF;
		*flags   |= XT_IFTAG_OIF;
	} else {
		if(STREQ(v2,"iif")) {
			if(info->op & XT_IFTAG_IIF)
				xtables_error(PARAMETER_PROBLEM,"iftag: double iif");
			xtables_error(PARAMETER_PROBLEM,"iftag: bad use iif");
		}
		if(sscanf(v2,"%i-%i",&info->tag1,&info->tag2) == 2) {
			if((info->op & XT_IFTAG_OPMASK) != XT_IFTAG_IN)
				xtables_error(PARAMETER_PROBLEM,"iftag: range without operator 'in'");
			if(info->op & XT_IFTAG_MASK) {
				if(info->tag1 & info->mask)
					xtables_error(PARAMETER_PROBLEM,"iftag: lower_value & mask != 0");
				if(info->tag2 & info->mask)
					xtables_error(PARAMETER_PROBLEM,"iftag: upper_value & mask != 0");
			}
		} else {
			if(sscanf(v2,"%i",&info->tag1) == 1) {
				if((info->op & XT_IFTAG_OPMASK) == XT_IFTAG_IN)
					xtables_error(PARAMETER_PROBLEM,"iftag: missing range value");
				if(info->op & XT_IFTAG_MASK) {
					if(info->tag1 & info->mask)
						xtables_error(PARAMETER_PROBLEM,"iftag: value & mask != 0");
				}
			} else xtables_error(PARAMETER_PROBLEM,"iftag: missing secondary operand"); 
		}
	}
//	fprintf(stderr,"tag1 %u tag2 %u mask 0x%x op 0x%x\n",
//			info->tag1,info->tag2,info->mask,info->op);
	return true;
}

static void
iftag_mt_check (unsigned int flags)
{
	if (flags == 0){
		xtables_error(PARAMETER_PROBLEM, "xt_iftag: You need to "
                              "specify at least one protocol");
	}
}


static void
iftag_mt_help(void)
{
 printf("iftag match options:\n"
	"  -m iftag --tag left,op,right\n"
	"   left:  iif|oif\n"
	"   op:    gt|eq|lt|in\n"
	"   right: (digits[(-digits]|oif)[/mask]\n");
}

static const struct option iftag_mt_opts[] = {
	{.name = "tag", .has_arg = true, .val = '1'},
	{.name = NULL},

};


static void iftag_mt_init (struct xt_entry_match *match) {
//	fprintf(stderr,"%s\n",__func__);
}

static struct xtables_match
iftag_mt4_reg = {
	.version = XTABLES_VERSION,
	.name = "iftag",
	.revision = 0,
	.family = NFPROTO_IPV4,
	.size = XT_ALIGN(sizeof(struct xt_iftag_mtinfo)),
	.userspacesize = XT_ALIGN(sizeof(struct xt_iftag_mtinfo)),
	.help = iftag_mt_help,
	.init = iftag_mt_init,
	.parse = iftag_mt4_parse,
	.final_check = iftag_mt_check,
	.print = iftag_mt4_print,
	.save = iftag_mt4_save,
	.extra_opts = iftag_mt_opts,
};

void _init(void)
{
	xtables_register_match(&iftag_mt4_reg);
}
