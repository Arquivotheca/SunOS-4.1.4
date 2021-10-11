# @(#)cg4.mk 1.1 94/10/31 SMI
# pixrect library sub-makefile

MAKEFILE =	cg4.mk

HDRS =		cg4var.h

CNORM =		cg4.c \
		cg4_colormap.c \
		cg4_region.c	\
		cg4_ioctl.c

CDATA =		cg4_ops.c

include ../device.mk
