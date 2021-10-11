#
# @(#)Makefile	1.1	94/10/31 Sun Micro 1986
#

.KEEP_STATE:

#
# This makefile claims to know how to make two front-end processors
# of the C language:
#	1-phase C compiler called ccom, installed as /usr/lib/ccom.
#	    (This doubles as a front end for the global optimizer
#	    /usr/lib/iropt, by invoking it with the flag -XO)
#	1-phase variable counter called vcount
#
# In addition, it makes two flavors of code generators:
#	f1, which reads a binary postfix form of pcc trees
#	cg, which reads an iropt IR file (triples with a symbol table)
#
# The former is used by Pascal and Modula-2; the latter is used
# by FORTRAN, and in due course, a globally optimizing version of C.
#
# While these things can all be made with existing parts, they cannot all
# be made in the same directory.  To make vcount you must add "-DVCOUNT"
# to the CPPFLAGS argument.  Thus these programs must really be made in
# their own directories, with their own makefiles.
#

#
# Guy's changes from System V lint (12 Apr 88):
# "messages.o" added to list of objects, "messages.c" added to
# list of source files, "messages.h" added to list of headers.
#

all:	ccom f1 cg

TARGET  = SPARC
TESTDIR = .
DESTDIR = 
LDFLAGS =
DBX =
COPTS   =  -O
INSTALL = install -s -m 555
CPPFLAGS = -DTARGET=$(TARGET) -DMC68000=1 -DSPARC=2 -DI386=3
CFLAGS  = $(CPPFLAGS) $(COPTS) $(DBX)
CPP	= /lib/cpp
AWK	= /bin/awk
YACC	= /usr/bin/yacc
FFLAGS	= $(CFLAGS)  -DFORT # for Fortran only
LINT	= /usr/bin/lint
LFLAGS	= $(CPPFLAGS) -hpv 
MIP	= ../mip
IROPT	= ../../iropt/src
CGRDR	= ../../cgrdr/sparc
MKPRIV	= $(MIP)/mkpriv.awk

#
# Code Browser section
#
#	Remove the assignment to `BROWSER' to turn all the browsing stuff off
BROWSER = -DBROWSER
BROWSER_YACC = $(BY$(BROWSER))
BY-DBROWSER = -DBROWSER
BY = -UBROWSER
BROWSER_DIR = ../../browser/cblib
CB_LIB-DBROWSER = $(BROWSER_DIR)/sparc/libcb.a
CB_I-DBROWSER = -I$(BROWSER_DIR)/src -DBROWSER

$(BROWSER_DIR)/sparc/libcb.a: FRC
	cd $(@D) ; $(MAKE) $(@F)
FRC:

#
# headers
#
MHDR =  $(MIP)/mip.h $(MIP)/cpass1.h $(MIP)/cpass2.h \
	$(MIP)/pcc_types.h $(MIP)/pcc_ops.h $(MIP)/pcc_node.h \
	$(MIP)/pcc_symtab.h $(MIP)/pcc_sw.h $(MIP)/pcc_f1defs.h \
	$(MIP)/assert.h $(MIP)/messages.h

AWKHDR=	types.h ops.h node.h symtab.h sw.h f1defs.h

PHDR =	machdep2.h machdep.h align.h

IHDR =	$(MIP)/ir_misc.h $(MIP)/ir_c.h $(MIP)/ir_types.h \
	$(IROPT)/ir_common.h $(IROPT)/opdescr.h

HDRS =	$(MHDR) $(PHDR) $(IHDR) $(AWKHDR)

.INIT:	$(HDRS) $(MIP)/pragma.h

#
# sources
#
MSRC =	$(MIP)/xdefs.c $(MIP)/scan.c $(MIP)/pftn.c $(MIP)/trees.c \
	$(MIP)/optim.c $(MIP)/reader.c $(MIP)/match.c $(MIP)/yyerror.c \
	$(MIP)/messages.c $(MIP)/symtab.c $(MIP)/p1.c

PSRC =	code.c local.c regvars.c local2.c genswitch.c order.c \
	opmatch.c allo1.c allo2.c bound.c table.c \
	float2.c myflags2.c stab.c optim2.c util2.c su.c \
	gencall.c struct.c picode.c malign.c assign_field.c

IRSRC =	$(MIP)/ir_wf.c $(MIP)/ir_alloc.c $(MIP)/ir_misc.c $(MIP)/ir_debug.c \
	$(MIP)/ir_map.c $(MIP)/ir_map_expr.c

CSRC =	$(MSRC) $(PSRC) $(IRSRC) $(MIP)/cgram.y $(MIP)/common.c $(MIP)/fort.c

SRCS =	$(CSRC) $(HDRS) Makefile

#
# objects
#
MOBJ =	xdefs.o scan.o pftn.o trees.o optim.o \
	reader.o match.o yyerror.o \
	symtab.o p1.o

POBJ =	code.o local.o regvars.o local2.o genswitch.o order.o \
	opmatch.o allo1.o allo2.o bound.o table.o \
	float2.o myflags2.o stab.o optim2.o util2.o su.o \
	gencall.o struct.o picode.o malign.o assign_field.o

COBJ =	cgram.o comm1.o $(MOBJ) messages.o $(POBJ) 

IROBJ =	ir_wf.o ir_alloc.o ir_misc.o ir_debug.o ir_map.o ir_map_expr.o

FOBJ =	freader.o fallo2.o fopmatch.o fmatch.o ftable.o forder.o \
	fbound.o flocal2.o fgenswitch.o fcomm2.o ffloat2.o fmyflags2.o foptim2.o \
	futil2.o fsu.o fgencall.o fstruct.o fpicode.o fmalign.o fassign_field.o

VOBJ =  cgram.o xdefs.o scan.o pftn.o trees.o optim.o code.o \
	local.o regvars.o comm1.o stab.o yyerror.o p1stubs.o allo1.o

# C0OBJ = cgram.o xdefs.o scan.o pftn.o trees.o optim.o code.o \
#	local.o regvars.o comm1.o stab.o yyerror.o p1stubs.o allo1.o

# C1OBJ = reader.o local2.o genswitch.o order.o opmatch.o match.o allo2.o comm2.o \
#	table.o optim2.o util2.o su.o gencall.o struct.o float2.o myflags2.o

#
# c compiler (ccom)
#

ccom:	$(AWKHDR) $(CB_LIB$(BROWSER)) $(COBJ) $(IROBJ)
	$(CC) $(LDFLAGS) $(COBJ) $(IROBJ) $(CB_LIB$(BROWSER)) -o $@

$(IROBJ): $(MIP)/$$(@:.o=.c)
	$(CC) -c $(CFLAGS) -I$(MIP) -I. -I$(IROPT) $(CB_I$(BROWSER)) $(MIP)/$(@:.o=.c)

$(MOBJ): $(MIP)/$$(@:.o=.c)
	$(CC) -c $(CFLAGS) -I$(MIP) -I.  $(MIP)/$(@:.o=.c) \
	$(CB_I$(BROWSER))

$(POBJ): $$(@:.o=.c)
	$(CC) -c $(CFLAGS) -I$(MIP) -I.  $*.c $(CB_I$(BROWSER))

vcount:	$(VOBJ)
	$(CC) $(LDFLAGS) $(VOBJ)
	mv a.out $(TESTDIR)/vcount
#
# defunct versions of a two-pass C compiler
# c0: $(C0OBJ)
#	$(CC) $(LDFLAGS) $(C0OBJ) -o c0
# c1: $(C1OBJ)
#	$(CC) $(LDFLAGS) $(C1OBJ) -o c1
#

messages.o: $(MIP)/messages.h $(MIP)/messages.c
	$(CC) $(CFLAGS) -c -R $(MIP)/messages.c

cgram.o: $(MIP)/mip.h $(MIP)/cpass1.h machdep.h cgram.c
	$(CC) -c -O $(CPPFLAGS) -I$(MIP) -I. cgram.c $(CB_I$(BROWSER))

cgram.c: $(MIP)/cgram.y
	rm -f gram.in
	-unifdef $(BROWSER_YACC) < $(MIP)/cgram.y > gram.in
	$(YACC) gram.in
	mv y.tab.c cgram.c

comm1.o: $(MIP)/mip.h $(MIP)/cpass1.h machdep.h $(MIP)/common.c
	$(CC) -c $(CFLAGS) -DPASS1 -I$(MIP) -I. $(MIP)/common.c -o $@

#
# local headers -- these include the public headers and add local
#	aliases for the public names -- a poor man's version of the
#	modula-2 IMPORT facility.
#
types.h: $(MIP)/pcc_types.h $(MKPRIV)
	$(AWK) -f $(MKPRIV) < $(MIP)/pcc_types.h > $@

ops.h: $(MIP)/pcc_ops.h $(MKPRIV)
	$(AWK) -f $(MKPRIV) < $(MIP)/pcc_ops.h > $@

node.h: $(MIP)/pcc_node.h $(MKPRIV)
	$(AWK) -f $(MKPRIV) < $(MIP)/pcc_node.h > $@

symtab.h: $(MIP)/pcc_symtab.h $(MKPRIV)
	$(AWK) -f $(MKPRIV) < $(MIP)/pcc_symtab.h > $@

sw.h: $(MIP)/pcc_sw.h $(MKPRIV)
	$(AWK) -f $(MKPRIV) < $(MIP)/pcc_sw.h > $@

f1defs.h: $(MIP)/pcc_f1defs.h $(MKPRIV)
	$(AWK) -f $(MKPRIV) < $(MIP)/pcc_f1defs.h > $@

shrink:
	-rm *.o ccom

clean:
	rm -f *.o ccom f1 cg pcc.a cgram.c gram.in $(AWKHDR)
	cd $(CGRDR); make clean

lintall: $(CSRC) $(HDRS) cgram.c 
	for i in $(MSRC) $(PSRC) ; do \
	    ( $(LINT) -i $(LFLAGS) -I. -I$(MIP) $$i ); \
	done
	for i in $(IRSRC); do \
	    ( $(LINT) -i $(LFLAGS) -I. -I$(MIP) -I$(IROPT) $$i ); \
	done
	$(LINT) -i $(LFLAGS) -I. -I$(MIP) cgram.c
	$(LINT) -i $(LFLAGS) -I. -I$(MIP) -DPASS1 $(MIP)/common.c
	$(LINT) -i $(LFLAGS) -I. -I$(MIP) -DFORT $(MIP)/fort.c
	$(LINT) $(LFLAGS) *.ln

#
# code generators (f1 and cg)
#
f1: $(FOBJ) fort.o 
	$(CC)  $(LDFLAGS) fort.o $(FOBJ) -o $@

cg: $(FOBJ) cgrdr
	$(CC)  $(LDFLAGS) $(CGRDR)/cgrdr.a $(FOBJ) -o $@

pcc.a: $(FOBJ)
	rm -f $@
	ar cru $@ $(FOBJ)
	ranlib $@

fort.o: $(MIP)/fort.c
	$(CC) -c $(FFLAGS) -I$(MIP) -I. $(MIP)/fort.c

# this line should ALWAYS be executed before building cg
cgrdr:
	cd $(CGRDR); make $(MFLAGS) COPTS="$(COPTS)"

freader.o: reader.o
	$(CC) -c $(FFLAGS) -DNOMAIN -I$(MIP) -I. $(MIP)/reader.c -o $@
fallo2.o: allo2.o
	$(CC) -c $(FFLAGS) -I$(MIP) -I. allo2.c -o $@
fmatch.o: match.o
	$(CC) -c $(FFLAGS) -I$(MIP) -I. $(MIP)/match.c -o $@
ftable.o: table.o
	$(CC) -c $(FFLAGS) -I$(MIP) -I. table.c -o $@
forder.o: order.o
	$(CC) -c $(FFLAGS) -I$(MIP) -I. order.c -o $@
fsu.o: su.o
	$(CC) -c $(FFLAGS) -I$(MIP) -I. su.c -o $@
flocal2.o: local2.o
	$(CC) -c $(FFLAGS) -I$(MIP) -I. local2.c -o $@
fgenswitch.o: genswitch.o
	$(CC) -c $(FFLAGS) -I$(MIP) -I. genswitch.c -o $@
fbound.o: bound.o
	$(CC) -c $(FFLAGS) -I$(MIP) -I. bound.c -o $@
ffloat2.o: float2.o 
	$(CC) -c $(FFLAGS) -I$(MIP) -I. float2.c -o $@
fmyflags2.o: myflags2.o 
	$(CC) -c $(FFLAGS) -I$(MIP) -I. myflags2.c -o $@
foptim2.o: optim2.o 
	$(CC) -c $(FFLAGS) -I$(MIP) -I. optim2.c -o $@
fopmatch.o: opmatch.o 
	$(CC) -c $(FFLAGS) -I$(MIP) -I. opmatch.c -o $@
futil2.o: util2.o 
	$(CC) -c $(FFLAGS) -I$(MIP) -I. util2.c -o $@
fgencall.o: gencall.o 
	$(CC) -c $(FFLAGS) -I$(MIP) -I. gencall.c -o $@
fstruct.o: struct.o 
	$(CC) -c $(FFLAGS) -I$(MIP) -I. struct.c -o $@
fpicode.o: picode.o
	$(CC) -c $(FFLAGS) -I$(MIP) -I. picode.c -o $@
fmalign.o: malign.o
	$(CC) -c $(FFLAGS) -I$(MIP) -I. malign.c -o $@
fassign_field.o: assign_field.o
	$(CC) -c $(FFLAGS) -I$(MIP) -I. assign_field.c -o $@
fcomm2.o: comm1.o
	$(CC) -c $(FFLAGS) -DPASS2 -I$(MIP) -I. $(MIP)/common.c -o $@

install:	all misalign.il
	$(INSTALL) ccom $(DESTDIR)/usr/lib/ccom
	$(INSTALL) cg $(DESTDIR)/usr/lib/cg
	install -m 444 misalign.il $(DESTDIR)/usr/lib/misalign.il

install_unbundled: cg
	install -d $(DESTDIR)/usr/lib/$(LANGUAGE)
	$(INSTALL) cg $(DESTDIR)/usr/lib/$(LANGUAGE)/cg
#	install -m 444 misalign.il $(DESTDIR)/usr/lib/misalign.il

version: $(SRCS)
	rm -f $@
	echo '#!/bin/csh' >> $@
	echo '# xWx (Sun) xGx' | sed 's/x/%/g'  >> $@
	echo '# Usage:' >> $@
	echo '#	version diffs' >> $@
	echo '# produces a list of diffs since xGx' | sed 's/x/%/g' >> $@
	echo '#	version get' >> $@
	echo '# retrieves the versions of the files in this' >> $@
	echo '# directory as of xGx' | sed 's/x/%/g' >> $@
	echo '#' >> $@
	echo 'if ($$#argv != 1) then' >> $@
	echo '    echo usage: version "{diffs|get}"' >> $@
	echo '    exit(1)' >> $@
	echo 'else' >> $@
	echo '    set cmd = $$argv[1]' >> $@
	echo '    if ($$cmd != "diffs" && $$cmd != "get") then' >> $@
	echo '	echo usage: version "{diffs|get}"' >> $@
	echo '	exit(1)' >> $@
	echo '    endif' >> $@
	echo 'endif' >> $@
	echo 'set list = ( \' >> $@
	for i in $(SRCS); do \
	    (/usr/ucb/what  $$i | grep "^	" | \
		$(AWK) '{ printf("    %s ", $$2) }' >> $@ ;\
		    echo $$i '\' >> $@ ) \
	done
	echo '    0 )' >> $@
	echo 'while ( $$list[1] != 0 )' >> $@
	echo '    set vers = $$list[1]' >> $@
	echo '    shift list' >> $@
	echo '    set file = $$list[1]' >> $@
	echo '    shift list' >> $@
	echo '    if ($$cmd == "get") then' >> $@
	echo '	sccs get -r$$vers -G$$file $$file' >> $@
	echo '    else' >> $@
	echo '	sccs diffs -r$$vers $$file' >> $@
	echo '    endif' >> $@
	echo 'end' >> $@
	chmod +x $@

print:  $(CSRC)
	pr -f Makefile 
	/usr/ucb/ctags -x $(CSRC) | pr -f -h XREF
	pr -f $(CSRC)

tags:	$(CSRC) $(HDRS) 
	ctags -t $(CSRC) $(HDRS)

