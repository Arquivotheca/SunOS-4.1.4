# @(#)Makefile 1.1 94/10/31 SMI
# pixrect library main makefile
#
#	Pixrect Makefile User's Guide
#
#	User must at least create the top level directory,
#	make symbolic link for SCCS, and check out the top level Makefile.
#
#	Make commands and operations:
#	make populate => 
#		make necessary sub-directories, check out necessary files
#	make => 
#		create libpixrect.a, assuming existence of sub-directories and
#		corresponding SCCS directories.
#	make kernel =>
#		create files necessary for kernel build
#	make install => 
#		install libpixrect.a, llib-lpixrect.ln and header
#		files (create them if necessary) to destination directory
#		(make all necessary directories if necessary).
#	make lib_p =>
#		create libpixrect_p.a  for prof
#	make lib_g =>
#		create libpixrect_g.a  for dbx
#	make lib TARGET_ARCH=-sun4 =>
#		create libpixrect.a  for sun4 architecture
#	make lib_p TARGET_ARCH=-mc68010 =>
#		create libpixrect_p.a for mc68010 architecture
#	make lib_sh
#		create libpixrect.so, .sa
#
# ------------------------------------------------
#	Directory Tree:
#		obj-`mach`: target directory for `arch`
#		obj-`mach`/pic: shared objects
#		pixrect    : header files
#		bw* cg* gp* mem pr: source directories
# ------------------------------------------------
#
PIXINC	      =
INCLUDE	      = .
INCDIR =	$(INCLUDE)/pixrect
DESTDIR	      = 
HDRDIR =	usr/include
LIBDIR  =	usr/lib
LINTDIR =	$(LIBDIR)/lint

OPTIM =		-O

SHLDFLAGS     = -assert pure-text
SHLDLIBSDIR   =
SHLDLIBS      = -ldl
SHLD	      = $(LD)

MAKEFILE      = Makefile
SCCSDIR	      = ../../SCCS_DIRECTORIES/usr.lib/libpixrect
OWNER	      = bin

# commands
INSTALLX =	install -c -o $(OWNER)
INSTALLD =	install -d -o $(OWNER) -m 755
INSTALL =	$(INSTALLX) -m 644
INSTALLH =	$(INSTALLX) -m 444
RANLIB =	ranlib

TARGET_DIR    = obj$(TARGET_ARCH)
SHOBJ_DIR     = $(TARGET_DIR)/pic

SHLIB_PREFIX =	# +
SHLIB_VER =	$(SHLIB_VER_CMD:sh)
SHLIB_VER_CMD =	sh version
SHLIB_SA_DIRS =	pr
INS_sh_so =	$(SHLIB_PREFIX)libpixrect.so.$(SHLIB_VER)
INS_sh_sa =	$(SHLIB_PREFIX)libpixrect.sa.$(SHLIB_VER)

# flags passed to inferior make
PASSFLAGS= TARGET_ARCH="$(TARGET_ARCH)" TARGET_DIR="../$(TARGET_DIR)" \
	CC="$(CC)" PIXINC="$(PIXINC)" OPTIM="$(OPTIM)"

# subdirectories
DEVDIR = mem bw2 cg2 cg3 cg4 cg6 cg8 cg9 cg12 gp1 tv1 gt
SUBDIR = pr $(DEVDIR)

LINTLIB	= 	$(TARGET_DIR)/llib-lpixrect
LINTLIB_LN =	$(LINTLIB).ln
LIB	      = $(TARGET_DIR)/libpixrect.a
LIB_g	      = $(TARGET_DIR)/libpixrect_g.a
LIB_p	      = $(TARGET_DIR)/libpixrect_p.a
LIB_pg	      = $(TARGET_DIR)/libpixrect_pg.a
LIB_sh_so =	$(TARGET_DIR)/libpixrect.so
LIB_sh_sa =	$(TARGET_DIR)/libpixrect.sa

SID_WHAT	= what
SID_FILE	= libpixrect.sid.$(SHLIB_VER)
TAGS_CMD	= ctags -ta
TAGS_SORT	= sort
TAGS_FILE	= tags

LINT_SRCS =	pr/pr_lint.c \
		bw2/bw2_lint.c \
		cg2/cg2_lint.c \
		cg3/cg3_lint.c \
		cg4/cg4_lint.c \
		cg6/cg6_lint.c \
		cg8/cg8_lint.c \
		cg9/cg9_lint.c \
		cg12/cg12_lint.c \
		gp1/gp1_lint.c \
		mem/mem_lint.c \
		tv1/tv1_lint.c \
		gt/gt_lint.c

#	No, these are not HDRS_INSTALL files
HDRS   =	bw2/bw2var.h \
		cg2/cg2reg.h \
		cg2/cg2var.h \
		cg3/cg3var.h \
		cg4/cg4var.h \
		cg8/cg8var.h \
		cg9/cg9var.h \
		cg12/cg12_var.h \
		gp1/gp1cmds.h \
		gp1/gp1reg.h \
		gp1/gp1var.h \
		mem/mem_rop_impl_ops.h \
		mem/mem_rop_impl_util.h \
		mem/memreg.h \
		mem/memvar.h \
		mem/mem32_var.h \
		pr/pixfont.h \
		pr/pixrect.h \
		pr/pixrect_hs.h \
		pr/pr_dblbuf.h \
		pr/pr_impl_make.h \
		pr/pr_impl_util.h \
		pr/pr_io.h \
		pr/pr_line.h \
		pr/pr_planegroups.h \
		pr/pr_util.h \
		pr/traprop.h \
		tv1/tv1var.h \
		gt/gtvar.h  \
		gt/gt_rp.h \
		gt/gt_fbi.h 

#	regular installed header files
HDRS_INSTALL   =	$(HDRS)

HDRS_EXTRA =	cg6/cg6fbc.h \
		cg6/cg6tec.h \
		cg6/cg6thc.h \
		cg6/cg6var.h \
		pr/pr_shlib_stub.h \
		pr/pr_texmacs.h

# all header files, in subdirectories
PIXHDRS	      = $(HDRS) $(HDRS_EXTRA)

.PRECIOUS: $(LIB) $(LIB_g) $(LIB_p) $(LIB_pg)

target:	lib lib_pg lib_sh lintlib

all: target lib_g

lib:		$(LIB)
lib_g:		$(LIB_g)
lib_p:		$(LIB_p)
lib_pg:		$(LIB_pg)
lib_sh:		$(LIB_sh_so)
lintlib:	$(LINTLIB) $(LINTLIB_LN)

clean: FRC
	$(RM) -rf $(SHOBJ_DIR) $(TARGET_DIR) $(INCDIR)
	@for d in $(SUBDIR); do \
		(cd $$d && $(MAKE) -f $$d.mk $(MFLAGS) $(PASSFLAGS) $@); \
	done

Clean: clean
	@-for d in $(SUBDIR); do (cd $$d && sccs clean); done
	@-sccs clean
	
$(LIB) \
$(LIB_g) \
$(LIB_p) \
$(LIB_pg) : device.mk incdir $(TARGET_DIR) FRC
	@for d in $(SUBDIR); do \
		(cd $$d && $(MAKE) -f $$d.mk $(MFLAGS) $(PASSFLAGS) $(@F)); \
	done
	$(RANLIB) $@

$(LIB_sh_so): $(LIB_sh_sa) liborder liblist FRC
	@for d in $(SUBDIR); do \
	(cd $$d && $(MAKE) -f $$d.mk $(MFLAGS) $(PASSFLAGS) \
		SHOBJ_DIR=../$(SHOBJ_DIR) $(@F) ); \
	done
	$(SHLD) -o $@ $(SHLDFLAGS) `sh liborder liblist $(SHOBJ_DIR)` \
		$(SHLDLIBSDIR) $(SHLDLIBS)

$(LIB_sh_sa): device.mk incdir $(TARGET_DIR) $(SHOBJ_DIR) FRC
	@for d in $(SHLIB_SA_DIRS); do \
	(cd $$d && $(MAKE) -f $$d.mk $(MFLAGS) $(PASSFLAGS) \
		SHOBJ_DIR=../$(SHOBJ_DIR) $(@F) ); \
	done
	$(RANLIB) $@

$(LINTLIB): incdir $(TARGET_DIR) $$(LINT_SRCS)
	@$(RM) $@
	echo "/* GENERATED FILE, DO NOT EDIT */" > $(LINTLIB)
	cat $(LINT_SRCS) >> $(LINTLIB)

$(LINTLIB_LN): $(LINTLIB)
	cd $(@D) && $(LINT) -Cpixrect $(LINTLIB:$(TARGET_DIR)/%=%)=.c

install: ins_lib ins_lint install_h

# Note: libpixrect_pg.a is installed as libpixrect_p.a to match
# the window people's practices.
ins_lib: lib lib_pg lib_sh version FRC
	$(INSTALLD) $(DESTDIR)/$(LIBDIR)
	$(INSTALL) $(LIB) \
		$(DESTDIR)/$(LIBDIR)/$(LIB:$(TARGET_DIR)/%=%)
	$(RANLIB) $(DESTDIR)/$(LIBDIR)/$(LIB:$(TARGET_DIR)/%=%)
	$(INSTALL) $(LIB_pg) \
		$(DESTDIR)/$(LIBDIR)/$(LIB_p:$(TARGET_DIR)/%=%)
	$(RANLIB) $(DESTDIR)/$(LIBDIR)/$(LIB_p:$(TARGET_DIR)/%=%)
	$(INSTALLX) -m 755 $(LIB_sh_so) $(DESTDIR)/$(LIBDIR)/$(INS_sh_so)
	$(INSTALL) $(LIB_sh_sa) $(DESTDIR)/$(LIBDIR)/$(INS_sh_sa)
	$(RANLIB) $(DESTDIR)/$(LIBDIR)/$(INS_sh_sa)

ins_lint: $(LINTLIB) $(LINTLIB_LN) FRC
	$(INSTALLD) $(DESTDIR)/$(LINTDIR)
	$(INSTALL) $(LINTLIB) $(LINTLIB_LN) $(DESTDIR)/$(LINTDIR)

install_h: $$(HDRS_INSTALL) FRC
	$(INSTALLD) $(DESTDIR)/$(HDRDIR)/pixrect
	$(INSTALLH) $(HDRS_INSTALL) $(DESTDIR)/$(HDRDIR)/pixrect

incdir: subdirs $(PIXHDRS)

$(PIXHDRS): $(INCDIR) $(INCDIR)/$$(@F)
	@test -f $@ || sccs get -s $@ -G$@
	$(RM) $(INCDIR)/$(@F)
	ln -s ../$@ $(INCDIR)/$(@F)

$(INCDIR)/%.h: FRC

$(SUBDIR) $(INCDIR) $(TARGET_DIR) $(SHOBJ_DIR):
	mkdir -p $@

populate: subdirs incdir FRC
	@for d in $(SUBDIR); do \
		(cd $$d && $(MAKE) $(MFLAGS) -f $$d.mk $(PASSFLAGS) $@) \
	done;

subdirs: $(SUBDIR) $(SUBDIR:%=%/SCCS) $(INCDIR)

$(SUBDIR:%=%/SCCS):
	test -h $@ || ln -s ../SCCS/../$@ $@

kernel: populate

cleandirs: FRC
	$(RM) -rf $(SUBDIR) $(INCDIR) $(TARGET_DIR)

sid:	version Makefile device.mk
	$(RM) $(SID_FILE); touch $(SID_FILE); \
	echo PWP PEP | tr P '\045' > $(SID_FILE);
	$(SID_WHAT) Makefile device.mk >> $(SID_FILE)
	@for d in $(SUBDIR); do \
		(cd $$d && \
		$(MAKE) $(MFLAGS) -f $$d.mk $(PASSFLAGS) \
		SID_WHAT="$(SID_WHAT)" SID_FILE=../$(SID_FILE) $@); \
	done;

tags:	FRC
	$(RM) $(TAGS_FILE); touch $(TAGS_FILE);
	@for d in $(SUBDIR); do \
		(cd $$d && \
		$(MAKE) $(MFLAGS) -f $$d.mk $(PASSFLAGS) \
		TAGS_CMD="$(TAGS_CMD)" TAGS_FILE=../$(TAGS_FILE) \
		DEVDIR=../$$d $@); \
	done;
	$(TAGS_SORT) $(TAGS_FILE) > $(TAGS_FILE).tmp && \
		mv $(TAGS_FILE).tmp $(TAGS_FILE)
	

FRC:
