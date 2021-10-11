# @(#)gp1.mk 1.1 94/10/31 SMI
# pixrect library sub-makefile

MAKEFILE =	gp1.mk

HDRS =		gp1cmds.h \
		gp1reg.h \
		gp1var.h

CNORM =		gp1.c \
		gp1_line.c \
		gp1_loadtex.c \
		gp1_polygon2.c \
		gp1_polyline.c \
		gp1_replrop.c \
		gp1_shmem.c \
		gp1_util.c \
		gp1_vec.c \
		gp1_rop.c \
		gp1_region.c \
		gp1_colormap.c
		

CDATA =		gp1_ops.c \
		gp1_ioctl.c

MOREOBJ =	

include ../device.mk

$(TARGET_DIR)/gp1_shmem.o:	$(TARGET_DIR)/gp1_shmem.il gp1_shmem.c
	$(COMPILE.c) -o $@ gp1_shmem.c $(TARGET_DIR)/gp1_shmem.il

gp1_shmem.o \
$(SHOBJ_DIR)/gp1_shmem.o: $(TARGET_DIR)/gp1_shmem.il gp1_shmem.c
	$(COMPILE.c) -o $@ gp1_shmem.c $(TARGET_DIR)/gp1_shmem.il

$(LIB)(gp1_shmem.o) \
$(LIB_p)(gp1_shmem.o) \
$(LIB_pg)(gp1_shmem.o) \
$(LIB_g)(gp1_shmem.o):	$(TARGET_DIR)/gp1_shmem.il gp1_shmem.c
	$(COMPILE.c) -o $% gp1_shmem.c $(TARGET_DIR)/gp1_shmem.il
	$(AR) $(ARFLAGS) $@ gp1_shmem.o
	$(RM) $%
