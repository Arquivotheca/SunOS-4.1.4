# @(#)cg6.mk 1.1 94/10/31 SMI
# pixrect library sub-makefile

MAKEFILE =	cg6.mk

HDRS =		cg6var.h \
		cg6fbc.h \
		cg6thc.h \
		cg6tec.h

CNORM =		cg6.c \
		cg6_batch.c \
		cg6_colormap.c \
		cg6_getput.c \
		cg6_line.c \
		cg6_polygon2.c \
		cg6_polyline.c \
		cg6_polyline0.c \
		cg6_polypoint.c \
		cg6_region.c \
		cg6_replrop.c \
		cg6_rop.c \
		cg6_stencil.c \
		cg6_util.c \
		cg6_vec.c \
		cg6_vec0.c

CDATA =		cg6_ops.c

include ../device.mk
