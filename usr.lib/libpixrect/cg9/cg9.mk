#	@(#)cg9.mk	1.1 94/10/31 SMI
MAKEFILE =	cg9.mk

HDRS =		cg9var.h

CNORM =		cg9.c \
		cg9_region.c \
		cg9_colormap.c \
		cg9_rop.c \
		cg9_vector.c \
		cg9_getput.c

CDATA =		cg9_ops.c

CNORO=	cg9_ioctl.c

include		../device.mk

