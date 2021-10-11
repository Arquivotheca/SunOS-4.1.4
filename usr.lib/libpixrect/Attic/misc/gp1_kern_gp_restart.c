#ifndef lint
static char sccsid[] = "@(#)gp1_kern_gp_restart.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sun/gpio.h>

/* this is just a wrapper for an ioctl to the gp driver to restart the gp */
/* called by assembler routines and therefore cannot do ioctl directly */

gp1_kern_gp_restart(fd, flag)
	int fd;
	int flag;
{
	ioctl(fd, GP1IO_CHK_GP, &flag);
}
