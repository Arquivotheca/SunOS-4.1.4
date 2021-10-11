# @(#)cg1.mk 1.1 94/10/31 SMI
# pixrect library sub-makefile

MAKEFILE =	cg1.mk

HDRS =		cg1reg.h \
		cg1var.h

CNORM =		cg1.c \
		cg1_batch.c \
		cg1_colormap.c \
		cg1_getput.c \
		cg1_polypoint.c \
		cg1_region.c \
		cg1_rop.c \
		cg1_stencil.c

CDATA =		cg1_ops.c

CNOOPT =	cg1_vec.c

CSHNORO =	cg1_vec.c

ASRC =		cg1_vecbres.S

include ../device.mk
