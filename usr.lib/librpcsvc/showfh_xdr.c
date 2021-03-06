#ifndef lint
static	char sccsid[] = "@(#)showfh_xdr.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Please do not edit this file.
 * It was generated partially using rpcgen.
 */
#include <rpc/rpc.h>
#include <rpcsvc/showfh.h>

bool_t
xdr_res_handle(xdrs, objp)
	XDR *xdrs;
	res_handle *objp;
{
	if (!xdr_string(xdrs, &objp->file_there, MAXNAMELEN)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->result)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_nfs_handle(xdrs, objp)
	XDR *xdrs;
	nfs_handle *objp;
{
	if (!xdr_array(xdrs, (char **)&objp->cookie.cookie_val, (u_int *)&objp->cookie.cookie_len, MAXNAMELEN, sizeof(u_int), xdr_u_int)) {
		return (FALSE);
	}
	return (TRUE);
}
