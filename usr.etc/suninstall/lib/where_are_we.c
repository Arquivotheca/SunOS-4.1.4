/*      @(#)where_are_we.c 1.1 94/10/31 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "stdio.h"

/*
 *	are we in MINIROOT ?
 */

char  *
where_are_we()
{
	FILE *fd;

	if((fd = fopen("/.MINIROOT","r")) == NULL) {
                return("");
        } else {
                (void) fclose(fd);
                return("/a");
        }
}
