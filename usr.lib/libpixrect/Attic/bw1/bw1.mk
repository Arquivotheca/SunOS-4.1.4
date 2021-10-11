# @(#)bw1.mk 1.1 94/10/31 SMI
# pixrect library sub-makefile

MAKEFILE =	bw1.mk

HDRS =		bw1reg.h \
		bw1var.h

CNORM =		bw1.c \
		bw1_batch.c \
		bw1_colormap.c \
		bw1_getput.c \
		bw1_polypoint.c \
		bw1_region.c \
		bw1_rop.c

CDATA =		bw1_ops.c

CNOOPT =	bw1_vec.c

CSHNORO =	bw1_vec.c

CSTAND =	bw1_stand.c

ASRC =		bw1_vecbres.S

include ../device.mk
