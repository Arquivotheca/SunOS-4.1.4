# @(#)mem.mk 1.1 94/10/31 SMI
# pixrect library sub-makefile

MAKEFILE =	mem.mk

HDRS =		mem_rop_impl_ops.h \
		mem_rop_impl_util.h \
		memreg.h \
		memvar.h \
		mem32_var.h

CNORM =		mem.c \
		mem_batch.c \
		mem_colormap.c \
		mem_getput.c \
		mem_polypoint.c \
		mem_prs.c \
		mem_region.c \
		mem_rop.c \
		mem_stencil.c \
		mem_vec.c \
		mem32_vec.c \
		mem32_cmap.c \
		mem32_rop.c

CDATA =		mem_ops.c \
		mem32_ops.c

MORESRC =	mem_kern.c

include ../device.mk

mem_rop.o $(SHOBJ_DIR)/mem_rop.o $(TARGET_DIR)/mem_rop.o \
$(LIB)(mem_rop.o) $(LIB_p)(mem_rop.o) $(LIB_pg)(mem_rop.o) := OPTIM=-O1
