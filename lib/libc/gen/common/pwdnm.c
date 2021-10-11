#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)pwdnm.c 1.1 94/10/31 Copyr 1987 Sun Micro"; /* c2 secure */
#endif

#include <rpc/rpc.h>
#include <rpcsvc/pwdnm.h>


bool_t
xdr_pwdnm(xdrs,objp)
	XDR *xdrs;
	pwdnm *objp;
{
	if (! xdr_wrapstring(xdrs, &objp->name)) {
		return(FALSE);
	}
	if (! xdr_wrapstring(xdrs, &objp->password)) {
		return(FALSE);
	}
	return(TRUE);
}


