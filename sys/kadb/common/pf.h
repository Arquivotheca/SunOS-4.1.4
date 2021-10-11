/*      @(#)pf.h 1.1 94/10/31 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

struct pseudo_file {
	char *pf_name;		/* name of the psuedo file */
	char *pf_string;	/* start of body of psuedo file */
};

#ifdef KADB
extern struct pseudo_file pf[];
extern int npf;
#endif
