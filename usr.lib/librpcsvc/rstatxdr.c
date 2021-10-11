#ifndef lint
static  char sccsid[] = "@(#)rstatxdr.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * XDR routines for the rstat daemon, rup and perfmeter.
 */

/* 
 * Note that all the portions of the file are not generated by rpcgen
 * due to some inconsistencies. So if regenerating with rpcgen make
 * sure that those portions are unaffected.
 */

#include <rpc/rpc.h>
#include <rpcsvc/rstat.h>

rstat(host, statp)
	char *host;
	struct statstime *statp;
{
	return (callrpc(host, RSTATPROG, RSTATVERS_TIME, RSTATPROC_STATS,
			xdr_void, (char *)NULL, xdr_statstime, (char *)statp));
}

havedisk(host)
	char *host;
{
	long have;
	
	if (callrpc(host, RSTATPROG, RSTATVERS_SWTCH, RSTATPROC_HAVEDISK,
			xdr_void, (char *)NULL, xdr_long, (char *)&have) != 0)
		return (-1);
	else
		return (have);
}

bool_t
xdr_stats(xdrs, statp)
	XDR *xdrs;
	struct stats *statp;
{
	int i;
	
	for (i = 0; i < 4; i++)
		if (xdr_int(xdrs, (int *) &statp->cp_time[i]) == 0)
			return (0);
	for (i = 0; i < 4; i++)
		if (xdr_int(xdrs, (int *) &statp->dk_xfer[i]) == 0)
			return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pgpgin) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pgpgout) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pswpin) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pswpout) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_intr) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ipackets) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ierrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_oerrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_collisions) == 0)
		return (0);
	return (1);
}

bool_t
xdr_statsswtch(xdrs, statp)		/* version 2 */
	XDR *xdrs;
	struct statsswtch *statp;
{
	int i;
	
	for (i = 0; i < 4; i++)
		if (xdr_int(xdrs, (int *) &statp->cp_time[i]) == 0)
			return (0);
	for (i = 0; i < 4; i++)
		if (xdr_int(xdrs, (int *) &statp->dk_xfer[i]) == 0)
			return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pgpgin) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pgpgout) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pswpin) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_pswpout) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_intr) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ipackets) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ierrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_oerrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_collisions) == 0)
		return (0);
	if (xdr_int(xdrs, (int *) &statp->v_swtch) == 0)
		return (0);
	for (i = 0; i < 3; i++)
		if (xdr_long(xdrs, &statp->avenrun[i]) == 0)
			return (0);
	if (xdr_timeval(xdrs, &statp->boottime) == 0)
		return (0);
	return (1);
}

/*
 * This code has been changed from the one generated by rpcgen as there
 * are some incompatibilities with the actual structure and the way the
 * xdr routine is written in the previous verisions.
 */

bool_t
xdr_statstime(xdrs, objp)
	XDR *xdrs;
	statstime *objp;
{
	if (!xdr_vector(xdrs, (char *)objp->cp_time, 4, sizeof(int), xdr_int)) {
		return (FALSE);
	}
	if (!xdr_vector(xdrs, (char *)objp->dk_xfer, 4, sizeof(int), xdr_int)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->v_pgpgin)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->v_pgpgout)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->v_pswpin)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->v_pswpout)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->v_intr)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->if_ipackets)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->if_ierrors)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->if_oerrors)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->if_collisions)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->v_swtch)) {
		return (FALSE);
	}
	if (!xdr_vector(xdrs, (char *)objp->avenrun, 3, sizeof(long), xdr_long)) {
		return (FALSE);
	}
	if (!xdr_timeval(xdrs, &objp->boottime)) {
		return (FALSE);
	}
	if (!xdr_timeval(xdrs, &objp->curtime)) {
		return (FALSE);
	}
	/* 
	 * Many implementations of this protocol incorrectly left out
	 * if_opackets. This value is included here for compatibility 
	 * with these buggy implementations. This is not generated by 
	 * rpcgen.
	 */
	(void) xdr_int (xdrs, &objp->if_opackets);
	return (TRUE);
}

bool_t
xdr_statsvar(xdrs, objp)
	XDR *xdrs;
	statsvar *objp;
{
	if (!xdr_array(xdrs, (char **)&objp->cp_time.cp_time_val, (u_int *)&objp->cp_time.cp_time_len, ~0, sizeof(int), xdr_int)) {
		return (FALSE);
	}
	if (!xdr_array(xdrs, (char **)&objp->dk_xfer.dk_xfer_val, (u_int *)&objp->dk_xfer.dk_xfer_len, ~0, sizeof(int), xdr_int)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->v_pgpgin)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->v_pgpgout)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->v_pswpin)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->v_pswpout)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->v_intr)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->if_ipackets)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->if_ierrors)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->if_opackets)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->if_oerrors)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->if_collisions)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->v_swtch)) {
		return (FALSE);
	}
	if (!xdr_vector(xdrs, (char *)objp->avenrun, 3, sizeof(long), xdr_long)) {
		return (FALSE);
	}
	if (!xdr_timeval(xdrs, &objp->boottime)) {
		return (FALSE);
	}
	if (!xdr_timeval(xdrs, &objp->curtime)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_timeval(xdrs, tvp)
	XDR *xdrs;
	struct timeval *tvp;
{
	if (xdr_long(xdrs, &tvp->tv_sec) == 0)
		return (FALSE);
	if (xdr_long(xdrs, &tvp->tv_usec) == 0)
		return (FALSE);
	return (TRUE);
}
