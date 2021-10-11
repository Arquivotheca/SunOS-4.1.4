# @(#)cg3.mk 1.1 94/10/31 SMI
# pixrect library sub-makefile

MAKEFILE =	cg3.mk

HDRS =		cg3var.h

CNORM =		cg3.c \
		cg3_colormap.c \
		cg3_region.c

CDATA =		cg3_ops.c

include ../device.mk
