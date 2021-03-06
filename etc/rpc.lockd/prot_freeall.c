#ifndef lint
static char sccsid[] = "@(#)prot_freeall.c 1.1 94/10/31 SMI";
#endif

	/*
	 * Copyright (c) 1988 by Sun Microsystems, Inc.
	 */

	/*
	 * prot_freeall.c consists of subroutines that implement the
	 * DOS-compatible file sharing services for PC-NFS
	 */

#include <stdio.h>
#include <sys/file.h>
#include <rpc/rpc.h>
#include <rpcsvc/sm_inter.h>
#include "prot_lock.h"

extern int debug;
extern int grace_period;
extern char *xmalloc();
extern void xfree();
extern bool_t obj_cmp();
extern void proc_priv_crash();

char *malloc();

void *
proc_nlm_freeall(Rqstp, Transp)
	struct svc_req *Rqstp;
	SVCXPRT *Transp;
{
	nlm_notify	req;
        struct status   stat;
/*
 * Allocate space for arguments and decode them
 */

	req.name = NULL;
	if (!svc_getargs(Transp, xdr_nlm_notify, &req)) {
		svcerr_decode(Transp);
		return;
	}

	if (debug) {
		printf("proc_nlm_freeall from %s\n",
			req.name);
	}
	destroy_client_shares(req.name);

        /*
         * Free up all Unix style locks
         */
        bzero((char *)&stat, sizeof(struct status));
        stat.mon_name = req.name;
        stat.state = req.state;
        proc_priv_crash(&stat);

	free(req.name);
	svc_sendreply(Transp, xdr_void, NULL);
}
