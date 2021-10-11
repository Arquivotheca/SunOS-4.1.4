#   @(#)Makefile 1.1 94/10/31 SMI
#
# Makefile for the IR=>pcc backend interface packages
# (cgrdr.a and onepass.a).
#
# cgrdr.a is for a standalone code generator that reads an
# IR archive, builds pcc trees, and hands them off to the pcc
# backend routines.
#
# onepass.a is linked into a frontend such as f77pass1 to make
# it into a onepass compiler.  It transforms the iropt IR directly
# into pcc trees and calls pcc backend routines to generate code.
#

TARGET=	SPARC
PCC =	../../pcc/sparc
MIP =	../../pcc/mip
IR =	../../iropt/src
SRC =	../src

DBX =
CPP =	/lib/cpp	# 'make depend' assumes -M feature of version 3.2!
GREP = egrep
TARGETS = -DMC68000=1 -DSPARC=2 -DI386=3
CPPFLAGS = -I$(IR) -I$(MIP) -I$(PCC) -DTARGET=$(TARGET) $(TARGETS)
COPTS=	-O
CFLAGS=	$(COPTS) $(DBX) $(CPPFLAGS)
LINT= /usr/bin/lint
LINTFLAGS = -hbx $(CPPFLAGS)
LTAIL=  ${GREP} -v 'struct/union .* never defined' | \
	${GREP} -v 'possible pointer alignment problem' ; true
LTAIL2= ${GREP} -v 'returns value which is always ignored' ; true
SED= /bin/sed

.DEFAULT:
	sccs get -G$@ $@

MHDR =  $(MIP)/mip.h $(MIP)/cpass2.h \
	$(MIP)/pcc_types.h $(MIP)/pcc_ops.h $(MIP)/pcc_node.h \
	$(MIP)/pcc_sw.h  $(MIP)/pcc_f1defs.h

AWKHDR=	$(PCC)/types.h $(PCC)/ops.h $(PCC)/node.h $(PCC)/sw.h

PHDR =	$(PCC)/machdep2.h $(PCC)/machdep.h $(PCC)/align.h

IHDR =	$(MIP)/ir_types.h $(IR)/ir_common.h $(IR)/opdescr.h

HDRS =	$(SRC)/cg_ir.h $(MHDR) $(PHDR) $(IHDR) $(AWKHDR)

SRCS =	$(SRC)/do_ir_archive.c $(SRC)/read_ir.c $(SRC)/debug.c \
	$(SRC)/misc.c $(SRC)/pcc.c $(SRC)/pccfmt.c $(SRC)/cg_main.c \
	$(SRC)/intr_map.c  $(SRC)/rewrite.c $(SRC)/onepass_init.c \
	$(SRC)/onepass_proc.c $(SRC)/cgflags.c

OBJS =	do_ir_archive.o read_ir.o debug.o \
	misc.o pcc.o pccfmt.o cg_main.o \
	intr_map.o  rewrite.o onepass_init.o \
	onepass_proc.o cgflags.o

CGRDR_OBJS = do_ir_archive.o read_ir.o debug.o \
	misc.o pcc.o pccfmt.o cg_main.o \
	intr_map.o rewrite.o cgflags.o

ONEPASS_OBJS = misc.o pcc.o pccfmt.o intr_map.o rewrite.o \
	onepass_init.o onepass_proc.o cgflags.o

PCC_OBJS = $(PCC)/cg.a

LINTOBJS = $(OBJS:.o=.ln)

CGRDR_LINTOBJS = $(CGRDR_OBJS:.o=.ln)

ONEPASS_LINTOBJS = $(ONEPASS_OBJS:.o=.ln)

cgrdr.a:   $(CGRDR_OBJS) FRC
	rm -f $@
	ar cru $@ $(CGRDR_OBJS)
	ranlib $@

onepass.a: $(ONEPASS_OBJS) FRC
	rm -f $@
	ar cru $@ $(ONEPASS_OBJS)
	ranlib $@

cgrdr.ln:	$(CGRDR_LINTOBJS) FRC
	@echo $@:
	@cat $(CGRDR_LINTOBJS) > $@
	@-(${LINT} -u $@) 2>&1 | ${LTAIL2}

$(OBJS):
	$(CC) -c $(CFLAGS) $(SRC)/`basename $@ .o`.c

$(LINTOBJS):
	@echo $(SRC)/`basename $@ .ln`.c :
	@-(${LINT} -i $(LINTFLAGS) $(SRC)/`basename $@ .ln`.c) 2>&1 | ${LTAIL}

$(PCC_OBJS): FRC
	cd $(PCC); make $(@F)

$(AWKHDR):
	cd $(PCC); make $(@F)

clean:
	rm -f $(OBJS) $(LINTOBJS) cgrdr.a onepass.a cgrdr.ln

FRC:

depend: $(SRCS) $(HDRS)
	rm -f makedep
	for i in $(SRCS); do \
	    ( $(CPP) -M $(CPPFLAGS) $$i | \
		sed -e 's,^\(.*\)\.o:,\1.o \1.ln:,' \
		    -e 's,$(IR),$$(IR),g' \
		    -e 's,$(MIP),$$(MIP),g' \
		    -e 's,$(PCC),$$(PCC),g' \
		>> makedep ); \
	done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile
	rm -f eddep makedep

# DO NOT DELETE THIS LINE -- make depend uses it

do_ir_archive.o do_ir_archive.ln: ../src/do_ir_archive.c
do_ir_archive.o do_ir_archive.ln: ../src/cg_ir.h
do_ir_archive.o do_ir_archive.ln: $(PCC)/machdep.h
do_ir_archive.o do_ir_archive.ln: $(PCC)/align.h
do_ir_archive.o do_ir_archive.ln: $(IR)/ir_common.h
do_ir_archive.o do_ir_archive.ln: $(MIP)/ir_types.h
do_ir_archive.o do_ir_archive.ln: $(MIP)/pcc_types.h
do_ir_archive.o do_ir_archive.ln: $(IR)/opdescr.h
do_ir_archive.o do_ir_archive.ln: /usr/include/stdio.h
do_ir_archive.o do_ir_archive.ln: /usr/include/sys/file.h
do_ir_archive.o do_ir_archive.ln: /usr/include/sys/fcntl.h
read_ir.o read_ir.ln: ../src/read_ir.c
read_ir.o read_ir.ln: ../src/cg_ir.h
read_ir.o read_ir.ln: $(PCC)/machdep.h
read_ir.o read_ir.ln: $(PCC)/align.h
read_ir.o read_ir.ln: $(IR)/ir_common.h
read_ir.o read_ir.ln: $(MIP)/ir_types.h
read_ir.o read_ir.ln: $(MIP)/pcc_types.h
read_ir.o read_ir.ln: /usr/include/stdio.h
read_ir.o read_ir.ln: /usr/include/ctype.h
read_ir.o read_ir.ln: /usr/include/sys/types.h
read_ir.o read_ir.ln: /usr/include/sys/sysmacros.h
read_ir.o read_ir.ln: /usr/include/sys/stat.h
read_ir.o read_ir.ln: /usr/include/sys/uio.h
debug.o debug.ln: ../src/debug.c
debug.o debug.ln: ../src/cg_ir.h
debug.o debug.ln: $(PCC)/machdep.h
debug.o debug.ln: $(PCC)/align.h
debug.o debug.ln: $(IR)/ir_common.h
debug.o debug.ln: $(MIP)/ir_types.h
debug.o debug.ln: $(MIP)/pcc_types.h
debug.o debug.ln: /usr/include/stdio.h
debug.o debug.ln: /usr/include/ctype.h
misc.o misc.ln: ../src/misc.c
misc.o misc.ln: ../src/cg_ir.h
misc.o misc.ln: $(PCC)/machdep.h
misc.o misc.ln: $(PCC)/align.h
misc.o misc.ln: $(IR)/ir_common.h
misc.o misc.ln: $(MIP)/ir_types.h
misc.o misc.ln: $(MIP)/pcc_types.h
misc.o misc.ln: /usr/include/stdio.h
pcc.o pcc.ln: ../src/pcc.c
pcc.o pcc.ln: ../src/cg_ir.h
pcc.o pcc.ln: $(PCC)/machdep.h
pcc.o pcc.ln: $(PCC)/align.h
pcc.o pcc.ln: $(IR)/ir_common.h
pcc.o pcc.ln: $(MIP)/ir_types.h
pcc.o pcc.ln: $(MIP)/pcc_types.h
pcc.o pcc.ln: $(MIP)/pcc_ops.h
pcc.o pcc.ln: $(MIP)/pcc_f1defs.h
pcc.o pcc.ln: $(MIP)/pcc_node.h
pcc.o pcc.ln: $(PCC)/machdep.h
pcc.o pcc.ln: $(MIP)/pcc_types.h
pcc.o pcc.ln: $(MIP)/pcc_sw.h
pcc.o pcc.ln: /usr/include/stdio.h
pcc.o pcc.ln: /usr/include/ctype.h
pccfmt.o pccfmt.ln: ../src/pccfmt.c
pccfmt.o pccfmt.ln: $(MIP)/cpass2.h
pccfmt.o pccfmt.ln: $(MIP)/mip.h
pccfmt.o pccfmt.ln: /usr/include/stdio.h
pccfmt.o pccfmt.ln: $(PCC)/ops.h
pccfmt.o pccfmt.ln: $(MIP)/pcc_ops.h
pccfmt.o pccfmt.ln: $(PCC)/types.h
pccfmt.o pccfmt.ln: $(MIP)/pcc_types.h
pccfmt.o pccfmt.ln: $(PCC)/node.h
pccfmt.o pccfmt.ln: $(MIP)/pcc_node.h
pccfmt.o pccfmt.ln: $(PCC)/machdep.h
pccfmt.o pccfmt.ln: $(PCC)/align.h
pccfmt.o pccfmt.ln: $(MIP)/pcc_types.h
pccfmt.o pccfmt.ln: $(PCC)/machdep.h
pccfmt.o pccfmt.ln: $(PCC)/machdep2.h
pccfmt.o pccfmt.ln: $(MIP)/pcc_sw.h
cg_main.o cg_main.ln: ../src/cg_main.c
cg_main.o cg_main.ln: $(MIP)/cpass2.h
cg_main.o cg_main.ln: $(MIP)/mip.h
cg_main.o cg_main.ln: /usr/include/stdio.h
cg_main.o cg_main.ln: $(PCC)/ops.h
cg_main.o cg_main.ln: $(MIP)/pcc_ops.h
cg_main.o cg_main.ln: $(PCC)/types.h
cg_main.o cg_main.ln: $(MIP)/pcc_types.h
cg_main.o cg_main.ln: $(PCC)/node.h
cg_main.o cg_main.ln: $(MIP)/pcc_node.h
cg_main.o cg_main.ln: $(PCC)/machdep.h
cg_main.o cg_main.ln: $(PCC)/align.h
cg_main.o cg_main.ln: $(MIP)/pcc_types.h
cg_main.o cg_main.ln: $(PCC)/machdep.h
cg_main.o cg_main.ln: $(PCC)/machdep2.h
intr_map.o intr_map.ln: ../src/intr_map.c
intr_map.o intr_map.ln: $(MIP)/pcc_ops.h
rewrite.o rewrite.ln: ../src/rewrite.c
rewrite.o rewrite.ln: ../src/cg_ir.h
rewrite.o rewrite.ln: $(PCC)/machdep.h
rewrite.o rewrite.ln: $(PCC)/align.h
rewrite.o rewrite.ln: $(IR)/ir_common.h
rewrite.o rewrite.ln: $(MIP)/ir_types.h
rewrite.o rewrite.ln: $(MIP)/pcc_types.h
rewrite.o rewrite.ln: $(MIP)/pcc_ops.h
rewrite.o rewrite.ln: $(MIP)/pcc_types.h
rewrite.o rewrite.ln: $(MIP)/pcc_node.h
rewrite.o rewrite.ln: $(PCC)/machdep.h
rewrite.o rewrite.ln: $(MIP)/pcc_types.h
rewrite.o rewrite.ln: /usr/include/stdio.h
onepass_init.o onepass_init.ln: ../src/onepass_init.c
onepass_init.o onepass_init.ln: $(MIP)/cpass2.h
onepass_init.o onepass_init.ln: $(MIP)/mip.h
onepass_init.o onepass_init.ln: /usr/include/stdio.h
onepass_init.o onepass_init.ln: $(PCC)/ops.h
onepass_init.o onepass_init.ln: $(MIP)/pcc_ops.h
onepass_init.o onepass_init.ln: $(PCC)/types.h
onepass_init.o onepass_init.ln: $(MIP)/pcc_types.h
onepass_init.o onepass_init.ln: $(PCC)/node.h
onepass_init.o onepass_init.ln: $(MIP)/pcc_node.h
onepass_init.o onepass_init.ln: $(PCC)/machdep.h
onepass_init.o onepass_init.ln: $(PCC)/align.h
onepass_init.o onepass_init.ln: $(MIP)/pcc_types.h
onepass_init.o onepass_init.ln: $(PCC)/machdep.h
onepass_init.o onepass_init.ln: $(PCC)/machdep2.h
onepass_proc.o onepass_proc.ln: ../src/onepass_proc.c
onepass_proc.o onepass_proc.ln: ../src/cg_ir.h
onepass_proc.o onepass_proc.ln: $(PCC)/machdep.h
onepass_proc.o onepass_proc.ln: $(PCC)/align.h
onepass_proc.o onepass_proc.ln: $(IR)/ir_common.h
onepass_proc.o onepass_proc.ln: $(MIP)/ir_types.h
onepass_proc.o onepass_proc.ln: $(MIP)/pcc_types.h
onepass_proc.o onepass_proc.ln: $(IR)/opdescr.h
onepass_proc.o onepass_proc.ln: /usr/include/stdio.h
onepass_proc.o onepass_proc.ln: /usr/include/sys/file.h
onepass_proc.o onepass_proc.ln: /usr/include/sys/fcntl.h
cgflags.o cgflags.ln: ../src/cgflags.c
cgflags.o cgflags.ln: $(MIP)/cpass2.h
cgflags.o cgflags.ln: $(MIP)/mip.h
cgflags.o cgflags.ln: /usr/include/stdio.h
cgflags.o cgflags.ln: $(PCC)/ops.h
cgflags.o cgflags.ln: $(MIP)/pcc_ops.h
cgflags.o cgflags.ln: $(PCC)/types.h
cgflags.o cgflags.ln: $(MIP)/pcc_types.h
cgflags.o cgflags.ln: $(PCC)/node.h
cgflags.o cgflags.ln: $(MIP)/pcc_node.h
cgflags.o cgflags.ln: $(PCC)/machdep.h
cgflags.o cgflags.ln: $(PCC)/align.h
cgflags.o cgflags.ln: $(MIP)/pcc_types.h
cgflags.o cgflags.ln: $(PCC)/machdep.h
cgflags.o cgflags.ln: $(PCC)/machdep2.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
