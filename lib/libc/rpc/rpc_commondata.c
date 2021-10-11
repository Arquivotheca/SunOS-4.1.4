/*	@(#)rpc_commondata.c 1.1 94/10/31 SMI	*/
#include <rpc/rpc.h>
/*
 * This file should only contain common data (global data) that is exported
 * by public interfaces
 */
struct opaque_auth _null_auth;
fd_set svc_fdset;
struct rpc_createerr rpc_createerr;
int	h_errno;	/* used by gethostbyname -- probably goes elsewhere */
