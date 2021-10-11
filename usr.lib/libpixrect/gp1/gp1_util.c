#ifndef lint
static char sccsid[] = "@(#)gp1_util.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * GP ioctl wrappers
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sun/gpio.h>

gp1_kern_gp_restart(fd, flag)
        int fd, flag;
{
	return ioctl(fd, GP1IO_CHK_GP, &flag);
}

gp1_get_static_block(fd)
	int fd;
{
	int block;

	return ioctl(fd, GP1IO_GET_STATIC_BLOCK, &block) >= 0 ? block : -1;
}


gp1_free_static_block(fd, block)
	int fd, block;
{
	return ioctl(fd, GP1IO_FREE_STATIC_BLOCK, &block);
}

gp1_get_restart_count(fd)
	int fd;
{
	int count;

	return ioctl(fd, GP1IO_GET_RESTART_COUNT, &count) >= 0 ? count : -1;
}
