# @(#)@(#)gt.mk	1.1 94/10/31 SMI
# pixrect library sub-makefile
 

MAKEFILE =	gt.mk

HDRS =		gtvar.h

CNORM =		gt.c \
		gt_region.c \
		gt_colormap.c \
		gt_rop.c \
		gt_vector.c \
		gt_getput.c \
		gt_stencil.c \
		gt_batch.c

CDATA =		gt_ops.c

CNORO=	gt_ioctl.c

include		../device.mk

