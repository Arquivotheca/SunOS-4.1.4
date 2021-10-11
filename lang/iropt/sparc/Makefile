#	@(#)Makefile 1.1 94/10/31 SMI	

TARGET=	SPARC
PCC =	../../pcc/sparc
MIP =	../../pcc/mip

HOST=	`mach`
SRC =	../src
DBX= 
AS=	as
CPP =	/lib/cpp        # 'make depend' assumes -M feature of version 3.2!
CPPFLAGS =  -DIROPT -DTARGET=$(TARGET) -DMC68000=1 -DSPARC=2 -DI386=3 \
	    -I$(PCC) -I$(MIP)
IROPT=iropt
COPTS= -O2
CFLAGS= $(COPTS) $(DBX) $(CPPFLAGS)
LDFLAGS=
LIBDEBUG= /usr/lib/debug

GREP=	/usr/bin/egrep
LINT= /usr/bin/lint
LINTFLAGS = -bx $(CPPFLAGS)
LTAIL=  ${GREP} -v 'struct/union .* never defined' | \
	${GREP} -v 'possible pointer alignment problem' ; true
LTAIL2= ${GREP} -v 'printf returns value which is always ignored' ; true

.DEFAULT:
	sccs get -G$@ $@

DESTDIR=

ASM_SRCS = \
	$(SRC)/bit_util.s

C_SRCS = \
	$(SRC)/iropt.c $(SRC)/misc.c $(SRC)/read_ir.c $(SRC)/setup.c \
	$(SRC)/aliaser.c $(SRC)/var_df.c $(SRC)/cf.c $(SRC)/expr_df.c \
	$(SRC)/recordrefs.c $(SRC)/page.c $(SRC)/loop.c $(SRC)/li.c \
	$(SRC)/ln.c $(SRC)/iv.c $(SRC)/reg.c $(SRC)/reg_alloc.c $(SRC)/cse.c \
	$(SRC)/copy_ppg.c $(SRC)/copy_df.c $(SRC)/implicit.c \
	$(SRC)/make_expr.c $(SRC)/debug.c $(SRC)/ir_wf.c $(SRC)/lu.c\
	$(SRC)/bit_util.c $(SRC)/tail_recursion.c $(SRC)/auto_alloc.c \
	$(SRC)/live.c $(SRC)/tprev_seq.c $(SRC)/vprev_seq.c \
	$(SRC)/cost.c $(SRC)/resequence.c \
	$(SRC)/complex_expansion.c

HDRS=	$(SRC)/ir_common.h $(SRC)/opdescr.h \
	$(PCC)/align.h $(MIP)/ir_types.h $(MIP)/pcc_types.h \
	$(C_SRCS:.c=.h)

SRCS =	$(C_SRCS) $(ASM_SRCS)

SCCS_FILES = Makefile $(SRCS) $(HDRS)

C_OBJS= \
	iropt.o misc.o read_ir.o setup.o aliaser.o var_df.o cf.o expr_df.o \
	recordrefs.o page.o loop.o li.o ln.o iv.o reg.o reg_alloc.o cse.o \
	copy_ppg.o copy_df.o implicit.o make_expr.o \
	debug.o ir_wf.o lu.o tail_recursion.o auto_alloc.o live.o \
	tprev_seq.o vprev_seq.o cost.o resequence.o \
	complex_expansion.o

O3_OBJS= expr_df.o var_df.o copy_df.o live.o loop.o

ASM_OBJS = \
	bit_util.o

INLINE_FILES = bit_util.il
ALL_INLINE_FILES= $(SRC)/bit_util.il.sparc

OBJS =	$(C_OBJS) $(ASM_OBJS)

LINTOBJS = $(C_OBJS:.o=.ln)

iropt:	$(OBJS)
	$(CC) $(DBX) $(LDFLAGS) -o $(IROPT) $(OBJS) \
		${LIBDEBUG}/malloc.o ${LIBDEBUG}/mallocmap.o

$(O3_OBJS) := COPTS = -O3

# for bit_util.s, define -DPROF if profiling

$(ASM_OBJS):
	case "$(HOST)" in \
	mc68030 | mc68020 | mc68010 ) \
	    /lib/cpp $(SRC)/`basename $@ .o`.s > tmp$$$$.s; \
		$(AS) -o $@ tmp$$$$.s; rm tmp$$$$.s \
	    ;;\
	sparc | i386 ) \
	    $(CC) -c $(CFLAGS) $(SRC)/`basename $@ .o`.c \
	    ;;\
	* ) \
	    echo >&2 "Unknown machine type \"$(HOST)\".  Stop." ;\
		exit 1 \
	    ;;\
	esac

$(C_OBJS): $(INLINE_FILES)
	$(CC) -c $(CFLAGS) $(INLINE_FILES) $(SRC)/`basename $@ .o`.c

$(INLINE_FILES):  $(ALL_INLINE_FILES)
	case "$(HOST)" in \
	sparc ) \
		rm -f $@ ; ln $(SRC)/$@.sparc $@ \
		;; \
	esac

$(LINTOBJS):
	@echo $(SRC)/`basename $@ .ln`.c :
	@-(${LINT} -i $(LINTFLAGS) $(SRC)/`basename $@ .ln`.c) 2>&1 | ${LTAIL}

install : iropt
	install -m 555 -s $(IROPT) $(DESTDIR)/usr/lib/$(IROPT)

lintall:  $(LINTOBJS) FRC
	@echo $@:
	@cat $(LINTOBJS) > $@.ln
	@-(${LINT} -u $@.ln) 2>&1 | ${LTAIL2}

install_unbundled: iropt
	install -d $(DESTDIR)/usr/lang/$(LANGUAGE)
	install -m 555 -s $(IROPT) $(DESTDIR)/usr/lang/$(LANGUAGE)/$(IROPT)

clean:
	rm -f $(OBJS) $(LINTOBJS) lintall.ln iropt $(INLINE_FILES)

FRC:

depend: $(SRCS) $(HDRS)
	rm -f makedep
	for i in $(SRCS); do \
	    ( $(CPP) -M $(CPPFLAGS) $$i  | \
		sed -e 's,^\(.*\)\.o:,\1.o \1.ln:,' \
		    -e '/sysmacros/d' \
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
	rm -f makedep eddep

# DO NOT DELETE THIS LINE -- make depend uses it

iropt.o iropt.ln: ../src/iropt.c
iropt.o iropt.ln: ../src/iropt.h
iropt.o iropt.ln: ../src/ir_common.h
iropt.o iropt.ln: $(MIP)/ir_types.h
iropt.o iropt.ln: $(MIP)/pcc_types.h
iropt.o iropt.ln: $(PCC)/align.h
iropt.o iropt.ln: ../src/recordrefs.h
iropt.o iropt.ln: ../src/debug.h
iropt.o iropt.ln: ../src/page.h
iropt.o iropt.ln: ../src/cf.h
iropt.o iropt.ln: ../src/setup.h
iropt.o iropt.ln: ../src/aliaser.h
iropt.o iropt.ln: ../src/implicit.h
iropt.o iropt.ln: ../src/var_df.h
iropt.o iropt.ln: ../src/make_expr.h
iropt.o iropt.ln: ../src/expr_df.h
iropt.o iropt.ln: ../src/loop.h
iropt.o iropt.ln: ../src/bit_util.h
iropt.o iropt.ln: ../src/ln.h
iropt.o iropt.ln: ../src/li.h
iropt.o iropt.ln: ../src/iv.h
iropt.o iropt.ln: ../src/resequence.h
iropt.o iropt.ln: ../src/misc.h
iropt.o iropt.ln: ../src/cse.h
iropt.o iropt.ln: ../src/copy_ppg.h
iropt.o iropt.ln: ../src/reg.h
iropt.o iropt.ln: ../src/cost.h
iropt.o iropt.ln: ../src/reg_alloc.h
iropt.o iropt.ln: ../src/auto_alloc.h
iropt.o iropt.ln: ../src/lu.h
iropt.o iropt.ln: ../src/tail_recursion.h
iropt.o iropt.ln: ../src/read_ir.h
iropt.o iropt.ln: ../src/ir_wf.h
iropt.o iropt.ln: ../src/complex_expansion.h
iropt.o iropt.ln: /usr/include/stdio.h
iropt.o iropt.ln: /usr/include/ctype.h
iropt.o iropt.ln: /usr/include/sys/file.h
iropt.o iropt.ln: /usr/include/sys/fcntlcom.h
iropt.o iropt.ln: /usr/include/sys/time.h
iropt.o iropt.ln: /usr/include/time.h
iropt.o iropt.ln: /usr/include/sys/resource.h
iropt.o iropt.ln: ../src/opdescr.h
misc.o misc.ln: ../src/misc.c
misc.o misc.ln: ../src/iropt.h
misc.o misc.ln: ../src/ir_common.h
misc.o misc.ln: $(MIP)/ir_types.h
misc.o misc.ln: $(MIP)/pcc_types.h
misc.o misc.ln: $(PCC)/align.h
misc.o misc.ln: ../src/page.h
misc.o misc.ln: ../src/misc.h
misc.o misc.ln: ../src/recordrefs.h
misc.o misc.ln: /usr/include/stdio.h
read_ir.o read_ir.ln: ../src/read_ir.c
read_ir.o read_ir.ln: ../src/iropt.h
read_ir.o read_ir.ln: ../src/ir_common.h
read_ir.o read_ir.ln: $(MIP)/ir_types.h
read_ir.o read_ir.ln: $(MIP)/pcc_types.h
read_ir.o read_ir.ln: $(PCC)/align.h
read_ir.o read_ir.ln: ../src/page.h
read_ir.o read_ir.ln: ../src/misc.h
read_ir.o read_ir.ln: ../src/cf.h
read_ir.o read_ir.ln: ../src/read_ir.h
read_ir.o read_ir.ln: ../src/complex_expansion.h
read_ir.o read_ir.ln: /usr/include/stdio.h
read_ir.o read_ir.ln: /usr/include/ctype.h
read_ir.o read_ir.ln: /usr/include/sys/types.h
read_ir.o read_ir.ln: /usr/include/sys/uio.h
setup.o setup.ln: ../src/setup.c
setup.o setup.ln: ../src/iropt.h
setup.o setup.ln: ../src/ir_common.h
setup.o setup.ln: $(MIP)/ir_types.h
setup.o setup.ln: $(MIP)/pcc_types.h
setup.o setup.ln: $(PCC)/align.h
setup.o setup.ln: ../src/page.h
setup.o setup.ln: ../src/misc.h
setup.o setup.ln: ../src/loop.h
setup.o setup.ln: ../src/bit_util.h
setup.o setup.ln: ../src/setup.h
setup.o setup.ln: /usr/include/stdio.h
aliaser.o aliaser.ln: ../src/aliaser.c
aliaser.o aliaser.ln: /usr/include/stdio.h
aliaser.o aliaser.ln: ../src/iropt.h
aliaser.o aliaser.ln: ../src/ir_common.h
aliaser.o aliaser.ln: $(MIP)/ir_types.h
aliaser.o aliaser.ln: $(MIP)/pcc_types.h
aliaser.o aliaser.ln: $(PCC)/align.h
aliaser.o aliaser.ln: ../src/page.h
aliaser.o aliaser.ln: ../src/misc.h
aliaser.o aliaser.ln: ../src/bit_util.h
aliaser.o aliaser.ln: ../src/aliaser.h
var_df.o var_df.ln: ../src/var_df.c
var_df.o var_df.ln: ../src/iropt.h
var_df.o var_df.ln: ../src/ir_common.h
var_df.o var_df.ln: $(MIP)/ir_types.h
var_df.o var_df.ln: $(MIP)/pcc_types.h
var_df.o var_df.ln: $(PCC)/align.h
var_df.o var_df.ln: ../src/recordrefs.h
var_df.o var_df.ln: ../src/page.h
var_df.o var_df.ln: ../src/misc.h
var_df.o var_df.ln: ../src/bit_util.h
var_df.o var_df.ln: ../src/var_df.h
var_df.o var_df.ln: ../src/make_expr.h
var_df.o var_df.ln: ../src/expr_df.h
var_df.o var_df.ln: ../src/live.h
var_df.o var_df.ln: ../src/debug.h
var_df.o var_df.ln: /usr/include/stdio.h
cf.o cf.ln: ../src/cf.c
cf.o cf.ln: ../src/iropt.h
cf.o cf.ln: ../src/ir_common.h
cf.o cf.ln: $(MIP)/ir_types.h
cf.o cf.ln: $(MIP)/pcc_types.h
cf.o cf.ln: $(PCC)/align.h
cf.o cf.ln: ../src/page.h
cf.o cf.ln: ../src/misc.h
cf.o cf.ln: ../src/read_ir.h
cf.o cf.ln: ../src/cf.h
cf.o cf.ln: /usr/include/stdio.h
cf.o cf.ln: /usr/include/ctype.h
expr_df.o expr_df.ln: ../src/expr_df.c
expr_df.o expr_df.ln: ../src/iropt.h
expr_df.o expr_df.ln: ../src/ir_common.h
expr_df.o expr_df.ln: $(MIP)/ir_types.h
expr_df.o expr_df.ln: $(MIP)/pcc_types.h
expr_df.o expr_df.ln: $(PCC)/align.h
expr_df.o expr_df.ln: ../src/page.h
expr_df.o expr_df.ln: ../src/misc.h
expr_df.o expr_df.ln: ../src/bit_util.h
expr_df.o expr_df.ln: ../src/recordrefs.h
expr_df.o expr_df.ln: ../src/make_expr.h
expr_df.o expr_df.ln: ../src/expr_df.h
expr_df.o expr_df.ln: ../src/var_df.h
expr_df.o expr_df.ln: ../src/debug.h
expr_df.o expr_df.ln: /usr/include/stdio.h
recordrefs.o recordrefs.ln: ../src/recordrefs.c
recordrefs.o recordrefs.ln: /usr/include/stdio.h
recordrefs.o recordrefs.ln: ../src/iropt.h
recordrefs.o recordrefs.ln: ../src/ir_common.h
recordrefs.o recordrefs.ln: $(MIP)/ir_types.h
recordrefs.o recordrefs.ln: $(MIP)/pcc_types.h
recordrefs.o recordrefs.ln: $(PCC)/align.h
recordrefs.o recordrefs.ln: ../src/recordrefs.h
recordrefs.o recordrefs.ln: ../src/page.h
recordrefs.o recordrefs.ln: ../src/misc.h
recordrefs.o recordrefs.ln: ../src/make_expr.h
page.o page.ln: ../src/page.c
page.o page.ln: ../src/iropt.h
page.o page.ln: ../src/ir_common.h
page.o page.ln: $(MIP)/ir_types.h
page.o page.ln: $(MIP)/pcc_types.h
page.o page.ln: $(PCC)/align.h
page.o page.ln: ../src/recordrefs.h
page.o page.ln: ../src/bit_util.h
page.o page.ln: ../src/make_expr.h
page.o page.ln: ../src/misc.h
page.o page.ln: ../src/page.h
page.o page.ln: ../src/loop.h
page.o page.ln: ../src/bit_util.h
page.o page.ln: ../src/reg.h
page.o page.ln: ../src/cost.h
page.o page.ln: ../src/iv.h
page.o page.ln: ../src/cf.h
page.o page.ln: /usr/include/stdio.h
page.o page.ln: /usr/include/sys/time.h
page.o page.ln: /usr/include/time.h
page.o page.ln: /usr/include/sys/resource.h
loop.o loop.ln: ../src/loop.c
loop.o loop.ln: ../src/iropt.h
loop.o loop.ln: ../src/ir_common.h
loop.o loop.ln: $(MIP)/ir_types.h
loop.o loop.ln: $(MIP)/pcc_types.h
loop.o loop.ln: $(PCC)/align.h
loop.o loop.ln: ../src/page.h
loop.o loop.ln: ../src/bit_util.h
loop.o loop.ln: ../src/misc.h
loop.o loop.ln: ../src/setup.h
loop.o loop.ln: ../src/loop.h
loop.o loop.ln: ../src/bit_util.h
loop.o loop.ln: ../src/debug.h
loop.o loop.ln: /usr/include/stdio.h
li.o li.ln: ../src/li.c
li.o li.ln: ../src/iropt.h
li.o li.ln: ../src/ir_common.h
li.o li.ln: $(MIP)/ir_types.h
li.o li.ln: $(MIP)/pcc_types.h
li.o li.ln: $(PCC)/align.h
li.o li.ln: ../src/page.h
li.o li.ln: ../src/misc.h
li.o li.ln: ../src/loop.h
li.o li.ln: ../src/bit_util.h
li.o li.ln: ../src/iv.h
li.o li.ln: ../src/ln.h
li.o li.ln: ../src/make_expr.h
li.o li.ln: ../src/recordrefs.h
li.o li.ln: ../src/lu.h
li.o li.ln: ../src/li.h
li.o li.ln: ../src/bit_util.h
li.o li.ln: /usr/include/stdio.h
ln.o ln.ln: ../src/ln.c
ln.o ln.ln: ../src/iropt.h
ln.o ln.ln: ../src/ir_common.h
ln.o ln.ln: $(MIP)/ir_types.h
ln.o ln.ln: $(MIP)/pcc_types.h
ln.o ln.ln: $(PCC)/align.h
ln.o ln.ln: ../src/page.h
ln.o ln.ln: ../src/misc.h
ln.o ln.ln: ../src/cost.h
ln.o ln.ln: ../src/loop.h
ln.o ln.ln: ../src/bit_util.h
ln.o ln.ln: ../src/ln.h
ln.o ln.ln: /usr/include/stdio.h
iv.o iv.ln: ../src/iv.c
iv.o iv.ln: ../src/iropt.h
iv.o iv.ln: ../src/ir_common.h
iv.o iv.ln: $(MIP)/ir_types.h
iv.o iv.ln: $(MIP)/pcc_types.h
iv.o iv.ln: $(PCC)/align.h
iv.o iv.ln: ../src/page.h
iv.o iv.ln: ../src/misc.h
iv.o iv.ln: ../src/loop.h
iv.o iv.ln: ../src/bit_util.h
iv.o iv.ln: ../src/make_expr.h
iv.o iv.ln: ../src/recordrefs.h
iv.o iv.ln: ../src/iv.h
iv.o iv.ln: /usr/include/stdio.h
reg.o reg.ln: ../src/reg.c
reg.o reg.ln: ../src/iropt.h
reg.o reg.ln: ../src/ir_common.h
reg.o reg.ln: $(MIP)/ir_types.h
reg.o reg.ln: $(MIP)/pcc_types.h
reg.o reg.ln: $(PCC)/align.h
reg.o reg.ln: ../src/page.h
reg.o reg.ln: ../src/misc.h
reg.o reg.ln: ../src/loop.h
reg.o reg.ln: ../src/bit_util.h
reg.o reg.ln: ../src/bit_util.h
reg.o reg.ln: /usr/include/stdio.h
reg.o reg.ln: /usr/include/ctype.h
reg.o reg.ln: ../src/reg.h
reg.o reg.ln: ../src/cost.h
reg.o reg.ln: ../src/reg_alloc.h
reg.o reg.ln: ../src/recordrefs.h
reg.o reg.ln: ../src/cost.h
reg_alloc.o reg_alloc.ln: ../src/reg_alloc.c
reg_alloc.o reg_alloc.ln: ../src/iropt.h
reg_alloc.o reg_alloc.ln: ../src/ir_common.h
reg_alloc.o reg_alloc.ln: $(MIP)/ir_types.h
reg_alloc.o reg_alloc.ln: $(MIP)/pcc_types.h
reg_alloc.o reg_alloc.ln: $(PCC)/align.h
reg_alloc.o reg_alloc.ln: ../src/recordrefs.h
reg_alloc.o reg_alloc.ln: ../src/page.h
reg_alloc.o reg_alloc.ln: ../src/misc.h
reg_alloc.o reg_alloc.ln: ../src/var_df.h
reg_alloc.o reg_alloc.ln: ../src/bit_util.h
reg_alloc.o reg_alloc.ln: ../src/loop.h
reg_alloc.o reg_alloc.ln: ../src/bit_util.h
reg_alloc.o reg_alloc.ln: ../src/cost.h
reg_alloc.o reg_alloc.ln: ../src/live.h
reg_alloc.o reg_alloc.ln: ../src/reg.h
reg_alloc.o reg_alloc.ln: ../src/cost.h
reg_alloc.o reg_alloc.ln: ../src/reg_alloc.h
reg_alloc.o reg_alloc.ln: /usr/include/stdio.h
reg_alloc.o reg_alloc.ln: /usr/include/ctype.h
cse.o cse.ln: ../src/cse.c
cse.o cse.ln: ../src/iropt.h
cse.o cse.ln: ../src/ir_common.h
cse.o cse.ln: $(MIP)/ir_types.h
cse.o cse.ln: $(MIP)/pcc_types.h
cse.o cse.ln: $(PCC)/align.h
cse.o cse.ln: ../src/page.h
cse.o cse.ln: ../src/misc.h
cse.o cse.ln: ../src/make_expr.h
cse.o cse.ln: ../src/recordrefs.h
cse.o cse.ln: ../src/cse.h
cse.o cse.ln: /usr/include/stdio.h
cse.o cse.ln: /usr/include/ctype.h
copy_ppg.o copy_ppg.ln: ../src/copy_ppg.c
copy_ppg.o copy_ppg.ln: ../src/iropt.h
copy_ppg.o copy_ppg.ln: ../src/ir_common.h
copy_ppg.o copy_ppg.ln: $(MIP)/ir_types.h
copy_ppg.o copy_ppg.ln: $(MIP)/pcc_types.h
copy_ppg.o copy_ppg.ln: $(PCC)/align.h
copy_ppg.o copy_ppg.ln: ../src/page.h
copy_ppg.o copy_ppg.ln: ../src/misc.h
copy_ppg.o copy_ppg.ln: ../src/bit_util.h
copy_ppg.o copy_ppg.ln: ../src/recordrefs.h
copy_ppg.o copy_ppg.ln: ../src/copy_ppg.h
copy_ppg.o copy_ppg.ln: ../src/copy_df.h
copy_ppg.o copy_ppg.ln: /usr/include/stdio.h
copy_df.o copy_df.ln: ../src/copy_df.c
copy_df.o copy_df.ln: ../src/iropt.h
copy_df.o copy_df.ln: ../src/ir_common.h
copy_df.o copy_df.ln: $(MIP)/ir_types.h
copy_df.o copy_df.ln: $(MIP)/pcc_types.h
copy_df.o copy_df.ln: $(PCC)/align.h
copy_df.o copy_df.ln: ../src/recordrefs.h
copy_df.o copy_df.ln: ../src/page.h
copy_df.o copy_df.ln: ../src/misc.h
copy_df.o copy_df.ln: ../src/bit_util.h
copy_df.o copy_df.ln: ../src/copy_ppg.h
copy_df.o copy_df.ln: ../src/copy_df.h
copy_df.o copy_df.ln: /usr/include/stdio.h
implicit.o implicit.ln: ../src/implicit.c
implicit.o implicit.ln: ../src/iropt.h
implicit.o implicit.ln: ../src/ir_common.h
implicit.o implicit.ln: $(MIP)/ir_types.h
implicit.o implicit.ln: $(MIP)/pcc_types.h
implicit.o implicit.ln: $(PCC)/align.h
implicit.o implicit.ln: ../src/recordrefs.h
implicit.o implicit.ln: ../src/page.h
implicit.o implicit.ln: ../src/misc.h
implicit.o implicit.ln: ../src/implicit.h
implicit.o implicit.ln: /usr/include/stdio.h
implicit.o implicit.ln: /usr/include/ctype.h
make_expr.o make_expr.ln: ../src/make_expr.c
make_expr.o make_expr.ln: ../src/iropt.h
make_expr.o make_expr.ln: ../src/ir_common.h
make_expr.o make_expr.ln: $(MIP)/ir_types.h
make_expr.o make_expr.ln: $(MIP)/pcc_types.h
make_expr.o make_expr.ln: $(PCC)/align.h
make_expr.o make_expr.ln: ../src/page.h
make_expr.o make_expr.ln: ../src/misc.h
make_expr.o make_expr.ln: ../src/recordrefs.h
make_expr.o make_expr.ln: ../src/make_expr.h
make_expr.o make_expr.ln: /usr/include/stdio.h
debug.o debug.ln: ../src/debug.c
debug.o debug.ln: /usr/include/stdio.h
debug.o debug.ln: ../src/iropt.h
debug.o debug.ln: ../src/ir_common.h
debug.o debug.ln: $(MIP)/ir_types.h
debug.o debug.ln: $(MIP)/pcc_types.h
debug.o debug.ln: $(PCC)/align.h
debug.o debug.ln: ../src/recordrefs.h
debug.o debug.ln: ../src/make_expr.h
debug.o debug.ln: ../src/page.h
debug.o debug.ln: ../src/misc.h
debug.o debug.ln: ../src/setup.h
debug.o debug.ln: ../src/reg.h
debug.o debug.ln: ../src/cost.h
debug.o debug.ln: ../src/loop.h
debug.o debug.ln: ../src/bit_util.h
debug.o debug.ln: ../src/live.h
debug.o debug.ln: ../src/bit_util.h
debug.o debug.ln: ../src/debug.h
ir_wf.o ir_wf.ln: ../src/ir_wf.c
ir_wf.o ir_wf.ln: ../src/iropt.h
ir_wf.o ir_wf.ln: ../src/ir_common.h
ir_wf.o ir_wf.ln: $(MIP)/ir_types.h
ir_wf.o ir_wf.ln: $(MIP)/pcc_types.h
ir_wf.o ir_wf.ln: $(PCC)/align.h
ir_wf.o ir_wf.ln: ../src/page.h
ir_wf.o ir_wf.ln: ../src/misc.h
ir_wf.o ir_wf.ln: ../src/reg.h
ir_wf.o ir_wf.ln: ../src/cost.h
ir_wf.o ir_wf.ln: ../src/auto_alloc.h
ir_wf.o ir_wf.ln: ../src/ir_wf.h
ir_wf.o ir_wf.ln: /usr/include/stdio.h
lu.o lu.ln: ../src/lu.c
lu.o lu.ln: ../src/iropt.h
lu.o lu.ln: ../src/ir_common.h
lu.o lu.ln: $(MIP)/ir_types.h
lu.o lu.ln: $(MIP)/pcc_types.h
lu.o lu.ln: $(PCC)/align.h
lu.o lu.ln: ../src/page.h
lu.o lu.ln: ../src/misc.h
lu.o lu.ln: ../src/loop.h
lu.o lu.ln: ../src/bit_util.h
lu.o lu.ln: ../src/recordrefs.h
lu.o lu.ln: ../src/iv.h
lu.o lu.ln: ../src/lu.h
lu.o lu.ln: /usr/include/stdio.h
bit_util.o bit_util.ln: ../src/bit_util.c
bit_util.o bit_util.ln: ../src/iropt.h
bit_util.o bit_util.ln: ../src/ir_common.h
bit_util.o bit_util.ln: $(MIP)/ir_types.h
bit_util.o bit_util.ln: $(MIP)/pcc_types.h
bit_util.o bit_util.ln: $(PCC)/align.h
bit_util.o bit_util.ln: ../src/bit_util.h
tail_recursion.o tail_recursion.ln: ../src/tail_recursion.c
tail_recursion.o tail_recursion.ln: /usr/include/stdio.h
tail_recursion.o tail_recursion.ln: ../src/iropt.h
tail_recursion.o tail_recursion.ln: ../src/ir_common.h
tail_recursion.o tail_recursion.ln: $(MIP)/ir_types.h
tail_recursion.o tail_recursion.ln: $(MIP)/pcc_types.h
tail_recursion.o tail_recursion.ln: $(PCC)/align.h
tail_recursion.o tail_recursion.ln: ../src/page.h
tail_recursion.o tail_recursion.ln: ../src/misc.h
tail_recursion.o tail_recursion.ln: ../src/lu.h
tail_recursion.o tail_recursion.ln: ../src/tail_recursion.h
auto_alloc.o auto_alloc.ln: ../src/auto_alloc.c
auto_alloc.o auto_alloc.ln: ../src/iropt.h
auto_alloc.o auto_alloc.ln: ../src/ir_common.h
auto_alloc.o auto_alloc.ln: $(MIP)/ir_types.h
auto_alloc.o auto_alloc.ln: $(MIP)/pcc_types.h
auto_alloc.o auto_alloc.ln: $(PCC)/align.h
auto_alloc.o auto_alloc.ln: ../src/page.h
auto_alloc.o auto_alloc.ln: ../src/misc.h
auto_alloc.o auto_alloc.ln: ../src/auto_alloc.h
auto_alloc.o auto_alloc.ln: /usr/include/stdio.h
live.o live.ln: ../src/live.c
live.o live.ln: /usr/include/stdio.h
live.o live.ln: ../src/iropt.h
live.o live.ln: ../src/ir_common.h
live.o live.ln: $(MIP)/ir_types.h
live.o live.ln: $(MIP)/pcc_types.h
live.o live.ln: $(PCC)/align.h
live.o live.ln: ../src/recordrefs.h
live.o live.ln: ../src/var_df.h
live.o live.ln: ../src/page.h
live.o live.ln: ../src/misc.h
live.o live.ln: ../src/setup.h
live.o live.ln: ../src/tprev_seq.h
live.o live.ln: ../src/vprev_seq.h
live.o live.ln: ../src/bit_util.h
live.o live.ln: ../src/live.h
tprev_seq.o tprev_seq.ln: ../src/tprev_seq.c
tprev_seq.o tprev_seq.ln: /usr/include/stdio.h
tprev_seq.o tprev_seq.ln: ../src/ir_common.h
tprev_seq.o tprev_seq.ln: ../src/tprev_seq.h
vprev_seq.o vprev_seq.ln: ../src/vprev_seq.c
vprev_seq.o vprev_seq.ln: /usr/include/stdio.h
vprev_seq.o vprev_seq.ln: ../src/iropt.h
vprev_seq.o vprev_seq.ln: ../src/ir_common.h
vprev_seq.o vprev_seq.ln: $(MIP)/ir_types.h
vprev_seq.o vprev_seq.ln: $(MIP)/pcc_types.h
vprev_seq.o vprev_seq.ln: $(PCC)/align.h
vprev_seq.o vprev_seq.ln: ../src/misc.h
vprev_seq.o vprev_seq.ln: ../src/recordrefs.h
vprev_seq.o vprev_seq.ln: ../src/vprev_seq.h
cost.o cost.ln: ../src/cost.c
cost.o cost.ln: /usr/include/stdio.h
cost.o cost.ln: ../src/iropt.h
cost.o cost.ln: ../src/ir_common.h
cost.o cost.ln: $(MIP)/ir_types.h
cost.o cost.ln: $(MIP)/pcc_types.h
cost.o cost.ln: $(PCC)/align.h
cost.o cost.ln: ../src/cost.h
resequence.o resequence.ln: ../src/resequence.c
resequence.o resequence.ln: ../src/iropt.h
resequence.o resequence.ln: ../src/ir_common.h
resequence.o resequence.ln: $(MIP)/ir_types.h
resequence.o resequence.ln: $(MIP)/pcc_types.h
resequence.o resequence.ln: $(PCC)/align.h
resequence.o resequence.ln: ../src/debug.h
resequence.o resequence.ln: ../src/resequence.h
resequence.o resequence.ln: ../src/misc.h
resequence.o resequence.ln: /usr/include/stdio.h
complex_expansion.o complex_expansion.ln: ../src/complex_expansion.c
complex_expansion.o complex_expansion.ln: ../src/iropt.h
complex_expansion.o complex_expansion.ln: ../src/ir_common.h
complex_expansion.o complex_expansion.ln: $(MIP)/ir_types.h
complex_expansion.o complex_expansion.ln: $(MIP)/pcc_types.h
complex_expansion.o complex_expansion.ln: $(PCC)/align.h
complex_expansion.o complex_expansion.ln: ../src/misc.h
complex_expansion.o complex_expansion.ln: ../src/page.h
complex_expansion.o complex_expansion.ln: ../src/iv.h
complex_expansion.o complex_expansion.ln: /usr/include/stdio.h
bit_util.o bit_util.ln: ../src/bit_util.s
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
