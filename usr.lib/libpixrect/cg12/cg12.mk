#	@(#)cg12.mk	1.1 94/10/31 SMI
MAKEFILE =	cg12.mk

HDRS =		cg12_var.h

CNORM =		cg12.c		\
		cg12_batch.c	\
		cg12_colormap.c	\
		cg12_getput.c	\
		cg12_region.c	\
		cg12_replrop.c	\
		cg12_rop.c	\
		cg12_stencil.c	\
		cg12_vector.c

CDATA =		cg12_ops.c

CNORO =		cg12_ioctl.c

include		../device.mk
