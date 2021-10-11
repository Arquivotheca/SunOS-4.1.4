/* @(#)vxseg.h 1.1 94/10/31 Sun Microsystems Visualization Products */

#ifndef _vm_seg_vx_h
#define _vm_seg_vx_h

struct vx_mseg { caddr_t      vms_addr;
                  int          vms_size;
                  struct proc *vms_pp;
		};

#ifdef KERNEL
int	segvx_create();
#endif KERNEL
#endif !_vm_seg_vx_h
