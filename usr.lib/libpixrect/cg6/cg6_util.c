#ifndef lint
static char	sccsid[] = "@(#)cg6_util.c 1.1 94/10/31 SMI";
#endif lint

 /*
 * When fcolor == 1 we always use the passed in rop.
 */
int  cg6_rop_table[] = {
	/* 0 */	0x0000,	
	/* 1 */	0x1155,	
	/* 2 */	0x22AA,	
	/* 3 */	0x33FF,	
	/* 4 */	0x4400,	
	/* 5 */	0x5555,	
	/* 6 */	0x66AA,	
	/* 7 */	0x77FF,	
	/* 8 */	0x8800,	
	/* 9 */	0x9955,	
	/* A */	0xAAAA,	
	/* B */	0xBBFF,	
	/* C */	0xCC00,	
	/* D */	0xDD55,	
	/* E */	0xEEAA,	
	/* F */	0xFFFF	
};
