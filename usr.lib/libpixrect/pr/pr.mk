# @(#)pr.mk 1.1 94/10/31 SMI
# pixrect library sub-makefile

MAKEFILE =	pr.mk

HDRS =		pixfont.h \
		pixrect.h \
		pixrect_hs.h \
		pr_dblbuf.h \
		pr_impl_make.h \
		pr_impl_util.h \
		pr_io.h \
		pr_line.h \
		pr_planegroups.h \
		pr_shlib_stub.h \
		pr_texmacs.h \
		pr_util.h \
		traprop.h

CNORM =		pf.c \
		pf_default.c \
		pf_text.c \
		pf_ttext.c \
		pr_clip.c \
		pr_gen_batch.c \
		pr_dblbuf.c \
		pr_io.c \
		pr_line.c \
		pr_make.c \
		pr_open.c \
		pr_plngrp.c \
		pr_polygon2.c \
		pr_polyline.c \
		pr_polypoint.c \
		pr_replrop.c \
		pr_reverse.c \
		pr_text.c \
		pr_texvec.c \
		pr_traprop.c \
		$(CSHSA1)

CDATA =		pr_makefun.c

CSHNORO =	pr_io.c \
		pr_polypoint.c

MORESRC =

CSHSA1 =	pr_texpat.c
CSHSA2 =	GP1_ROP_sa.c MEM_ROP_sa.c

# list of fonts to be compiled into pf_defdata.o
FONTLIST      = pf_fontlist
FONTDIR       = fixedwidthfonts
FONTLIB       = ..

include ../device.mk

$(OSHSA2:%=$(SHOBJ_DIR)/%) $(OSHSA2:%=$(LIB_sa)(%)) := OPTIM = -O1

HOST_CC =	cc
HOST_ARCH:sh =	echo -n "-" ; arch
MAKEDEFDIR =	$(TARGET_DIR:%$(TARGET_ARCH)=%$(HOST_ARCH))
MAKEDEF =	$(MAKEDEFDIR)/pf_makedef

# we don't want this in CNOOPT because populate would make pf_makedef
ONOOPT +=	pf_defdata.o
OBJ1 +=		pf_defdata.o

pf_defdata.c: ../$(FONTDIR) $(FONTLIST) $(MAKEDEF)
	@$(RM) $@
	$(MAKEDEF) < $(FONTLIST) > $@ || { $(RM) $@ ; exit 1 ; }

../$(FONTDIR):
	@test -d $@ || ln -s $(FONTLIB)/$(FONTDIR) $@

$(MAKEDEF): $$(@F).c
	@test -d $(@D) || mkdir -p $(@D)
	$(HOST_CC) -o $@ $(@F).c
