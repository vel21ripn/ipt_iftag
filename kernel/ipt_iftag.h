#ifndef _XT_IFTAG_H
#define _XT_IFTAG_H

#include <linux/types.h>
/*
 * -m iftag left op right
 *  left: iif|oif
 *  op: gt|eq|lt|in
 *  right: (iif|oif|digit[-digit])[/mask]
 */
#define XT_IFTAG_L_IIF	0x1
#define XT_IFTAG_L_OIF	0x2
#define XT_IFTAG_L	0x3

#define XT_IFTAG_R_IIF	0x4
#define XT_IFTAG_R_OIF	0x8
#define XT_IFTAG_R	0xC

#define XT_IFTAG_OPMASK	0x30
#define XT_IFTAG_LT	0x00
#define XT_IFTAG_EQ	0x10
#define XT_IFTAG_GT	0x20
#define XT_IFTAG_IN	0x30

#define XT_IFTAG_OP	0x40
#define XT_IFTAG_MASK	0x80


struct xt_iftag_mtinfo {
	__u32 tag1,tag2,mask; /* tag2 for range */
	__u8 op; /* iif, oif, lt, eq, gt, in, mask */
	__u8 invert;
};

#endif /*_XT_IFTAG_H*/
