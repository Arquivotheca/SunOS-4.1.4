# @(#)@(#)cg8.mk	1.1 10/31/94, SMI
# pixrect library sub-makefile

MAKEFILE =	cg8.mk

HDRS = 		cg8var.h

CNORM =		cg8.c			\
		cg8_region.c		\
		cg8_colormap.c		\
		cg8_rop.c		\
		cg8_vector.c		\
		cg8_getput.c

CDATA = 	cg8_ops.c

CNORO = 	cg8_ioctl.c

include ../device.mk
