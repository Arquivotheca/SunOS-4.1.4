#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)des_crypt.c 1.1 94/10/31 (C) 1986 Sun Micro";
#endif

/*
 * des_crypt.c, DES encryption library routines
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */


#include <sys/types.h>
#include <des/des_crypt.h>
#ifdef sun
#       include <sys/ioctl.h>
#       include <sys/des.h>
#       ifdef KERNEL
#               include <sys/conf.h>
#               define getdesfd() (cdevsw[11].d_open(0, 0) ? -1 : 0)
#               define ioctl(a, b, c) (cdevsw[11].d_ioctl(0, b, c, 0) ? -1 : 0)
#               ifndef CRYPT
#                       define _des_crypt(a, b, c) 0
#               endif
#       else
#               define getdesfd()       (open("/dev/des", 0, 0))
#       endif
#endif

/*
 * To see if chip is installed
 */
#define	UNOPENED (-2)
static int g_desfd = UNOPENED;



/*
 * Copy multiple of 8 bytes
 */

/*
 * CBC mode encryption
 */
cbc_crypt(key, buf, len, mode, ivec)
	char *key;
	char *buf;
	unsigned len;
	unsigned mode;
	char *ivec;
{
	return (1);
}


/*
 * ECB mode encryption
 */
ecb_crypt(key, buf, len, mode)
	char *key;
	char *buf;
	unsigned len;
	unsigned mode;
{

	return (1);
}



/*
 * Common code to cbc_crypt() & ecb_crypt()
 */
static
common_crypt(key, buf, len, mode, desp)
	char *key;
	char *buf;
	register unsigned len;
	unsigned mode;
	register struct desparams *desp;
{
	return (1);
}
