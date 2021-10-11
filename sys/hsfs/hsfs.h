/*	@(#)hsfs.h 1.1 94/10/31 SMI  */

/*
 * Interface definitions for High Sierra filesystem
 * Copyright (c) 1989 by Sun Microsystem, Inc.
 */

struct hsfs_args {
	char *fspec;	/* name of filesystem to mount */
	int norrip;
};
