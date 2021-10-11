
/*	@(#)vxioctl.h 1.1 94/10/31 SMI/VPG	
*/

/*
* Copyright (c) 1990 by Sun Microsystems, Inc.
*/

#define VXTEST     	_IOW(v, 1, int)
#define VXDEBUG    	_IOW(v, 2, int)
#define VXENINTR   	_IOW(v, 3, struct vx_parms)
#define VXLOCK     	_IOW(v, 4, struct vx_parms)
#define VXUNLOCK   	_IOW(v, 5, struct vx_parms)
#define VXEXPORTSEG	_IOW(v, 6, struct vx_parms)
#define VXUNEXPORTSEG	_IOW(v, 7, struct vx_parms)
#define VXNDMAP    	_IOR(v, 8, int)
#define	VXALLOCKEY	_IOW(v, 9, int)
#define	VXFREEKEY	_IOW(v, 10, int)
#define	VXFREELOCK	_IOW(v, 11, int)

struct vx_parms { int 		vip_unit; 
		  caddr_t 	vip_addr;
	 	  int     	vip_size;
	        };
