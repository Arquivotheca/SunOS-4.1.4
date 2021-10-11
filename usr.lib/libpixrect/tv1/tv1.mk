# @(#)tv1.mk	1.1 94/10/31 SMI
# pixrect library sub-makefile

MAKEFILE =	tv1.mk

HDRS = 		tv1var.h

CNORM =		tv1.c \
		tv1_region.c \
		tv1_colormap.c \
		tv1_ops.c \
		tv1_en_rop.c \
		tv1_ioctl.c

CDATA = 	tv1_ops.c

include ../device.mk
