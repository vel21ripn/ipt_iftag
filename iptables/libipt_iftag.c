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

static const char *op_m1[4] = { "<", "==",">","in" };

static const char *op_m2[4] = { "lt", "eq","gt","in" };

static void 
__iftag_mt4_save(const void *entry, const struct xt_entry_match *match,int mode)
{
	const struct xt_iftag_mtinfo *info = (const void *)match->data;
	const char *vop;
	char *v1,*v2,b[32],mask[32];

	b[0] = '\0';
	mask[0] = '\0';

	v1 = (info->op & XT_IFTAG_L_IIF) ? "iif":"oif";
	if(info->op & XT_IFTAG_R) {
		v2 = (info->op & XT_IFTAG_R_IIF) ? "iif":"oif";
	} else {
		if((info->op & XT_IFTAG_OPMASK) == XT_IFTAG_IN)
			snprintf(b,sizeof(b)-1,"%u-%u",info->tag1,info->tag2);
		    else
			snprintf(b,sizeof(b)-1,info->op & XT_IFTAG_MASK ?
					"0x%x":"%u",info->tag1);
		v2 = b;
	}
	vop = mode ? op_m1[(info->op & XT_IFTAG_OPMASK) >> 4]:
		     op_m2[(info->op & XT_IFTAG_OPMASK) >> 4];

	if(mode) {
	  char v1m[32],v2m[32];
	  if(info->op & XT_IFTAG_MASK) {
		snprintf(v1m,sizeof(v1m)-1,"(%s & ~0x%x)",v1,info->mask);
		v1 = v1m;
		snprintf(v2m,sizeof(v2m)-1,"(%s & ~0x%x)",v2,info->mask);
		if(info->op & XT_IFTAG_R) v2 = v2m;
	  }
	  printf(" %stag %s %s %s ",info->invert ? "! ":"" , v1,vop,v2);
	} else {
	  if(info->op & XT_IFTAG_MASK)
		snprintf(mask,sizeof(mask)-1,"/0x%x",info->mask);
	  printf(" %s--tag %s %s %s%s ",info->invert ? "! ":"" ,v1,vop,v2,mask);
	}
}

static void 
iftag_mt4_save(const void *entry, const struct xt_entry_match *match)
{
__iftag_mt4_save(entry,match,0);
}

static void 
iftag_mt4_print(const void *entry, const struct xt_entry_match *match,
                  int numeric)
{
__iftag_mt4_save(entry,match,1);
}

#define STREQ(a,b) !strcmp(a,b)

#ifndef xtables_error
#define xtables_error exit_error
#endif
static void parse_lpart(char *optarg,
		    struct xt_iftag_mtinfo *info,
		    unsigned int *flags) {
    if(optarg) {
	if(STREQ(optarg,"iif"))
		info->op |= XT_IFTAG_L_IIF;
	if(STREQ(optarg,"oif"))
		info->op |= XT_IFTAG_L_OIF;
    }
    *flags   |= info->op;
    if(!(info->op & XT_IFTAG_L))
		xtables_error(PARAMETER_PROBLEM,"iftag: missing iif or oif");
 }

static void parse_op(char *vop,
		    struct xt_iftag_mtinfo *info,
		    unsigned int *flags) {
    if(vop) {
	if(STREQ(vop,"lt") || STREQ(vop,"<")) 
		info->op |= XT_IFTAG_LT|XT_IFTAG_OP;
	if(STREQ(vop,"eq") || STREQ(vop,"=") || STREQ(vop,"=="))
		info->op |= XT_IFTAG_EQ|XT_IFTAG_OP;
	if(STREQ(vop,"gt") || STREQ(vop,">"))
		info->op |= XT_IFTAG_GT|XT_IFTAG_OP;
	if(STREQ(vop,"in"))
		info->op |= XT_IFTAG_IN|XT_IFTAG_OP;
    }
    *flags   |= info->op;
    if(!(info->op & XT_IFTAG_OP))
		xtables_error(PARAMETER_PROBLEM,"iftag: missing operator");
}
static void parse_rpart(char *v2,
		    struct xt_iftag_mtinfo *info,
		    unsigned int *flags) {
    if(v2) {
	char *v2c = strchr(v2,'/');
	if(v2c) {
		*v2c++ = '\0';
		if(sscanf(v2c,"%i",&info->mask) != 1)
			xtables_error(PARAMETER_PROBLEM,"iftag: invalid mask");
		info->op |= XT_IFTAG_MASK;
	}
	if(STREQ(v2,"iif")) {
		info->op |= XT_IFTAG_R_IIF;
		if(info->op & XT_IFTAG_L_IIF)
			xtables_error(PARAMETER_PROBLEM,"iftag: iif and iif");
	} else if(STREQ(v2,"oif")) {
		info->op |= XT_IFTAG_R_OIF;
		if(info->op & XT_IFTAG_L_OIF)
			xtables_error(PARAMETER_PROBLEM,"iftag: oif and oif");
	} else {
		if(sscanf(v2,"%i-%i",&info->tag1,&info->tag2) == 2 ||
		   sscanf(v2,"%i:%i",&info->tag1,&info->tag2) == 2) {
			if((info->op & XT_IFTAG_OPMASK) != XT_IFTAG_IN)
				xtables_error(PARAMETER_PROBLEM,"iftag: range without operator 'in'");
			if(info->op & XT_IFTAG_MASK)
				xtables_error(PARAMETER_PROBLEM,"iftag: can't range and mask");
			if(info->tag1 == info->tag2) 
				xtables_error(PARAMETER_PROBLEM,"iftag: empty range");
			if(info->tag1 > info->tag2) {
				uint32_t t = info->tag1;
				info->tag1 = info->tag2;
				info->tag2 = t;
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
    } else xtables_error(PARAMETER_PROBLEM,"iftag: missing secondary operand");
    *flags   |= info->op;
}

static int 
iftag_mt4_parse(int c, char **argv, int invert, unsigned int *flags,
                  const void *entry, struct xt_entry_match **match)
{
	struct xt_iftag_mtinfo *info = (void *)(*match)->data;
	char args[64];
	static const char *delim=", \t";
	char *v1,*vop,*v2,*sr;

        *flags = 0;
	info->invert = invert;

	strncpy(args,optarg,sizeof(args)-1);
	v1 = strtok_r(args,delim,&sr);
	if(!v1)
		xtables_error(PARAMETER_PROBLEM,"iftag: missing args");
	parse_lpart(v1,info,flags);
	vop = strtok_r(NULL,delim,&sr);
	if(vop) {
		parse_op(vop,info,flags);
		v2 = strtok_r(NULL,delim,&sr);
		parse_rpart(v2,info,flags);
	} else {
		vop = argv[optind];
		parse_op(vop,info,flags);
		optind++;
		v2 = argv[optind];
		parse_rpart(v2,info,flags);
		optind++;
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
	"  -m iftag --tag left,op,right\n or\n  -m iftag --tag left op right\n"
	"   left:  iif|oif\n"
	"   op:    gt|eq|lt|in\n"
	"   right: iif[/mask]|oif[/mask]|digits[-digits]\n");
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
