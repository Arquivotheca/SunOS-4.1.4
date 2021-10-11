#ifndef lint
static	char sccsid[] = "@(#)dcp_probe.c 1.1 94/10/31 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <ctype.h>
#include <nlist.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/buf.h>
#include <sys/vmmac.h>
#include <sys/dkbad.h>
#include <machine/param.h>
#include <machine/pte.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <sundev/mbvar.h>
#include <sundev/screg.h>
#include <sundev/sireg.h>
#include <sundev/scsi.h>
#include "sdrtns.h"

struct nlist nl[] = {
        { "_des_addr" },
        "",
};
 
/*
 *      probe() - returns TRUE if there is des else FALSE if there is no des
 *      or there are errors
 */
probe()
{
        int value = 0, kmem;

        if (nlist("/vmunix", nl) == -1) {
	   send_message(0, CONSOLE, "error in reading namelist\n"); 
           return(FALSE);
        }
        if (nl[0].n_value == 0) {
	   send_message(0, CONSOLE, "Variables missing from namelist\n");
           return(FALSE);
        }
        if ((kmem = open("/dev/kmem", 0)) < 0) {
	   send_message(0, CONSOLE, "can't open kmem\n");
           return(FALSE);
        }
        if (lseek(kmem, nl[0].n_value, 0) != -1) {
            if (read(kmem, &value, sizeof(int)) != sizeof (int)) {
	       send_message(0, CONSOLE, "can't read des_exists from kmem\n");
               return(FALSE);
            }
	}
        return(value);
}
