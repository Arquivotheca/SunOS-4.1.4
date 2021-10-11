# @(#)cg2.mk 1.1 94/10/31 SMI
# pixrect library sub-makefile

MAKEFILE =	cg2.mk

HDRS =		cg2reg.h \
		cg2var.h


CNORM =		cg2.c \
		cg2_batch.c \
		cg2_colormap.c \
		cg2_getput.c \
		cg2_polypoint.c \
		cg2_region.c \
		cg2_rop.c \
		cg2_stencil.c \
		cg2_vec.c

CDATA =		cg2_ops.c

include ../device.mk
