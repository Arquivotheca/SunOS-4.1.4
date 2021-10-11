/*
* Copyright 1988 by Sun Microsystems, Inc.
*/
/* @(#)vxvar.h 1.1 94/10/31 Sun Microsystems Visualization Products */

#include <sys/types.h>
#include <sundev/vxseg.h>

/* per node info
*/
struct vx_nodeinfo { 
	int 		vn_flag;
	struct	proc	*vn_procp;
	int 		vn_lockflag;
	long		vn_kmp;  /* kernal map for data */
	int		vn_datap; /* pointer to data page */
	/* these added for DVMA */
	int		vn_mbinfo;
	struct buf	*vn_bp;
	struct vx_mseg	vn_mseg;
	struct vx_mseg	vn_omseg;
};

/* per process info
*/

struct vx_procinfo {
	u_int		vp_flag;
	u_int		vp_lockmask;
	u_int		vp_intrmask;
	struct proc	*vp_procp;
};

/* all driver info
*/
#define VX_MAXBOARDS	5	/* 1 sfb + 4 jpp's for now */
#define VX_MAXNODES    20  	/* 4 nodes per board */
#define VX_MAXPROCS    10  	/* arbitrary               */

