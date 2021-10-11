# @(#)Makefile	1.1 (Sun) 10/31/94
DESTDIR=

#
# This makefile handles sparc (sun4) kadb
#
.KEEP_STATE:
COMMON=../../common
SPARC=..
ARCH:sh = arch

CFLAGS_K= -O -fsoft		# C flags for KADB.

KREDEFS= -Dprintf=_printf -Dopenfile=_openfile\
	-Dopen=_open -Dclose=_close -Dlseek=_lseek -Dread=_read	\
	-Dwrite=_write

COPTS= -D$(ARCH) -I${SPARC} -I${COMMON} -DKADB ${KREDEFS}
KCOPTS= -O -fsoft ${COPTS}
CPPFLAGS= ${COPTS}

RM=	/bin/rm -f

CPP=	/lib/cpp


#### #### #### #### #### #### #### #### #### #### #### #### #### #### ####
#	The main target name --
#
all:	kadb.o

#### #### #### #### #### #### #### #### #### #### #### #### #### #### ####
# Get lists of common object files and rules for their kadb versions.
#
# Should use ${COMMON}, but "make" is broken.
include ../../common/MakeCommon


#### #### #### #### #### #### #### #### #### #### #### #### #### #### ####
# sparc-specific kadb object files
#

KOBJ_TARGET=\
	kadb_ptrace.o\
	kaccesssr.o\
	kopsetsr.o\
	ksetupsr.o\
	kdisasm.o\
	kixtbls.o\
	kopcodetable.o\
	kprintsr.o

# sparc-specific include files
H_TARGET=\
	${SPARC}/sr_general.h\
	${SPARC}/sr_indextables.h\
	${SPARC}/sr_instruction.h\
	${SPARC}/sr_opcodetable.h\
	${SPARC}/sr_processor.h\
	${SPARC}/sr_sw_trap.h\
	${SPARC}/sr_switchindexes.h\
	${SPARC}/sparc.h
# sparc-specific C source files
C_TARGET=\
	${SPARC}/adb_ptrace.c\
	${SPARC}/accesssr.c\
	${SPARC}/disasm.c\
	${SPARC}/ixtbls.c\
	${SPARC}/opcodetable.c\
	${SPARC}/opsetsr.c\
	${SPARC}/printsr.c\
	${SPARC}/setupsr.c

.INIT:	$(H_TARGET)

KOBJ= ${KOBJ_COM} ${KOBJ_TARGET}
HFILES = ${H_COM} ${H_TARGET}
CFILES = ${KC_COM} ${C_TARGET}
SRC=	${CFILES} ${SFILES}

#### #### #### #### #### #### #### #### #### #### #### #### #### #### ####
# kadb.o is a partial object file used to build kadb

kadb.o: ${KOBJ}
	${LD} -r -o $@ ${KOBJ}
	size $@

install:

clean:
	rm -f eddep makedep makenok Makefile.bak kadb.o ${KOBJ}

print:
	@ls -l | pr
	@cd /usr/include; pr a.out.h frame.h nlist.h stab.h sys/pcb.h sys/reg.h
	@pr Makefile ${HFILES} ${CFILES} ${SFILES}

lint:
	lint -DKADB ${CFILES} > linterrs

tags:	${CFILES}
	ctags ${CFILES}

#### #### #### #### #### #### #### #### #### #### #### #### #### #### ####
# The following rules are for building .o's for use with kadb.
# _Rules_ for building the rest of the kadb ".o" files live in the
# file ${COMMON}/MakeCommon; the sources are in ${COMMON}.
#
kadb_ptrace.o:	${SPARC}/adb_ptrace.c
	${CC} ${KCOPTS} -c ${SPARC}/adb_ptrace.c -o $@

kaccesssr.o:	${SPARC}/accesssr.c
	${CC} ${KCOPTS} -c ${SPARC}/accesssr.c -o $@

kdisasm.o:	${SPARC}/disasm.c
	${CC} ${KCOPTS} -c ${SPARC}/disasm.c -o $@

kixtbls.o:	${SPARC}/ixtbls.c
	${CC} ${KCOPTS} -c ${SPARC}/ixtbls.c -o $@

kopcodetable.o:	${SPARC}/opcodetable.c
	${CC} ${KCOPTS} -c ${SPARC}/opcodetable.c -o $@

kopsetsr.o:	${SPARC}/opsetsr.c
	${CC} ${KCOPTS} -c ${SPARC}/opsetsr.c -o $@

kprintsr.o:	${SPARC}/printsr.c
	${CC} ${KCOPTS} -c ${SPARC}/printsr.c -o $@

ksetupsr.o:	${SPARC}/setupsr.c
	${CC} ${KCOPTS} -c ${SPARC}/setupsr.c -o $@

#### #### #### #### #### #### #### #### #### #### #### #### #### #### ####
