# @(#)Makefile 1.1 94/10/31 (Sun)
DESTDIR=

#
# This Makefile handles adb only; kadb has been moved into
# its own directory (./kadb).
#
RM=	/bin/rm -f
CPP=	/lib/cpp
ARCH=	`/bin/arch`
MACH=	`/bin/mach`
ASDIR=/usr/kvm
COMMON=../common
.KEEP_STATE:

#
# The CFLAGS are used both in compiling and linking.
# The CPPOPTS are used only in compiling.
#
# CFLAGS_N= -O -fsoft		# Normal C flags
CFLAGS_N= # fsoft is irrelevant on sparc, and -O is temporarily
          # disabled due to a bug in the optimizer.
CFLAGS_D= -g -DDEBUG -fsoft $(CB_FLAG) 	# No -O when we're doing -g.

CPPOPTS = -D${ARCH} -D${MACH} -I. -I${COMMON} -DADB

#### #### #### #### #### #### #### #### #### #### #### #### #### #### ####
#	The main target name --
#
all:	adb

#### #### #### #### #### #### #### #### #### #### #### #### #### #### ####
# Get lists of common object files
#
# Should be ${COMMON}/MakeCommon, but make is broken.
include ../common/MakeCommon


#### #### #### #### #### #### #### #### #### #### #### #### #### #### ####
# sparc-specific object files
#
OBJ_TARGET=\
	adb_ptrace.o\
	accesssr.o\
	disasm.o\
	fpasciisr.o\
	ixtbls.o\
	opcodetable.o\
	opsetsr.o\
	printsr.o\
	setupsr.o

# sparc-specific include files
H_TARGET=\
	sr_general.h\
	sr_indextables.h\
	sr_instruction.h\
	sr_opcodetable.h\
	sr_processor.h\
	sr_sw_trap.h\
	sr_switchindexes.h\
	sparc.h

# sparc-specific C source files
C_TARGET=\
	adb_ptrace.c\
	accesssr.c\
	disasm.c\
	fpasciisr.c\
	ixtbls.c\
	opcodetable.c\
	opsetsr.c\
	printsr.c\
	setupsr.c


OBJ= ${OBJ_COM} ${OBJ_TARGET}
HFILES = ${H_COM} ${H_TARGET}
CFILES = ${C_COM} ${C_TARGET}
SRC=	${CFILES}

#### #### #### #### #### #### #### #### #### #### #### #### #### #### ####
# default rules
#
# These rules behave like default rules, but allow the source to live in
# another directory.  This rule provides (for each object file) the action
# command(s) to compile the C source, but the "real" dependencies are created
# by "make depend", and are listed at the end of the Makefile.
#
${OBJ_COM}:
	${CC} ${CFLAGS_N} ${CPPOPTS} -c ${COMMON}/`basename $@ .o`.c

${OBJ_TARGET}:
	${CC} ${CFLAGS_N} ${CPPOPTS} -c `basename $@ .o`.c


#### #### #### #### #### #### #### #### #### #### #### #### #### #### ####
# adb itself!
#
adb:	${OBJ}
	${CC} ${CFLAGS_N} -o $@ ${OBJ} -lkvm

# debugging version
dadb:	${OBJ}
	${CC} ${CFLAGS_D} -o $@ ${OBJ} -lkvm

dadb := CFLAGS_N = ${CFLAGS_D}

#### #### #### #### #### #### #### #### #### #### #### #### #### #### ####
#	Temporary Kluge ...
#
# We are currently in the midst of a transition from "FAB1" sparc chips
# to "FAB2".  This Makefile now normally creates an adb for "FAB2" chips;
# Temporarily, during the transition, a "FAB1" adb can be built by doing
# "make fab1adb".  [ "FAB1" is also called "P1.5" or "Version 5"; "FAB2"
# is also called "P2.0" or "Version 6".  Ain't consistency wonderful? ]
#
#   N O T E :   You must still set the sr version to 5 or 6, according
#		to which kind of chip you want this adb to run on.
#   What is this ridiculous insistence on ".o" for object files around
#   here?  DAMN!  I can't even get "srcc" to create ".f1o" files instead of
#   ".o" files, and damn ld complains about 'em too.
#
FAB1OC = ${OBJ_COM:.o=.f1o}
FAB1OT = ${OBJ_TARGET:.o=.f1o}
FAB1FLAGS = -DFAB1 ${CFLAGS_N} ${CPPOPTS}
${FAB1OC}:
	${CC} ${FAB1FLAGS} -c ${COMMON}/`basename $@ .f1o`.c ; mv `basename $@ .f1o`.o $@

${FAB1OT}:
	${CC} ${FAB1FLAGS} -c `basename $@ .f1o`.c ; mv `basename $@ .f1o`.o $@

fab1adb:	${FAB1OC} ${FAB1OT}
	${CC} ${CFLAGS_N} -o $@ ${FAB1OC} ${FAB1OT} -lkvm

#	... End of Temporary Kluge
#### #### #### #### #### #### #### #### #### #### #### #### #### #### ####


#### #### #### #### #### #### #### #### #### #### #### #### #### #### ####
# The rest of the goals need work
#

install:	adb
	install -d -o bin -m 755 ${DESTDIR}${ASDIR}/adb
	install -s adb ${DESTDIR}${ASDIR}/adb/adb
	-rm -f ${DESTDIR}/bin/adb
	/usr/bin/ln -s ../kvm/adb/adb ${DESTDIR}/bin/adb

clean:
	${RM} adb eddep makedot makedep tags Makefile.bak ${OBJ} Made ,Made

print:
	@ls -l | pr
	@cd /usr/include; pr a.out.h frame.h nlist.h stab.h sys/pcb.h sys/reg.h
	@pr Makefile ${HFILES} ${CFILES}

lint:
	lint -DADB ${CFILES} > linterrs

tags:	${CFILES}
	ctags ${CFILES}

depend: ${HFILES} ${CFILES}
	${RM} makedot
	for i in ${CFILES}; do \
	    ( ${CPP} -M ${CPPOPTS} $$i >> makedot ); \
	done
	# "../common/../sparc" is just "."!
	sed -e 's=\.\./common/\.\./sparc/==' -e 's=: \./=: =' makedot | \
		sed '/: \/usr\/include/d' > makedep
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile
	${RM} eddep makedep makedot

# DO NOT DELETE THIS LINE nor the two following! -- make depend uses them
#
access.o: ../common/access.c
access.o: ../common/adb.h
access.o: sparc.h
access.o: ../common/process.h
command.o: ../common/command.c
command.o: ../common/adb.h
command.o: sparc.h
command.o: ../common/process.h
command.o: ../common/fio.h
command.o: fpascii.h
expr.o: ../common/expr.c
expr.o: ../common/adb.h
expr.o: sparc.h
expr.o: ../common/process.h
expr.o: ../common/fio.h
expr.o: ../common/symtab.h
fio.o: ../common/fio.c
fio.o: ../common/adb.h
fio.o: sparc.h
fio.o: ../common/process.h
fio.o: ../common/fio.h
format.o: ../common/format.c
format.o: ../common/adb.h
format.o: sparc.h
format.o: ../common/process.h
format.o: ../common/symtab.h
format.o: ../common/fio.h
format.o: fpascii.h
input.o: ../common/input.c
input.o: ../common/adb.h
input.o: sparc.h
input.o: ../common/process.h
output.o: ../common/output.c
output.o: ../common/adb.h
output.o: sparc.h
output.o: ../common/process.h
pcs.o: ../common/pcs.c
pcs.o: ../common/adb.h
pcs.o: sparc.h
pcs.o: ../common/process.h
print.o: ../common/print.c
print.o: ../common/adb.h
print.o: sparc.h
print.o: ../common/process.h
print.o: ../common/symtab.h
print.o: fpascii.h
runpcs.o: ../common/runpcs.c
runpcs.o: ../common/adb.h
runpcs.o: sparc.h
runpcs.o: ../common/process.h
runpcs.o: fpascii.h
sym.o: ../common/sym.c
sym.o: ../common/adb.h
sym.o: sparc.h
sym.o: ../common/process.h
sym.o: ../common/fio.h
sym.o: ../common/symtab.h
main.o: ../common/main.c
main.o: ../common/adb.h
main.o: sparc.h
main.o: ../common/process.h
main.o: fpascii.h
ttycontrol.o: ../common/ttycontrol.c
adb_ptrace.o: adb_ptrace.c
adb_ptrace.o: ../common/adb.h
adb_ptrace.o: sparc.h
adb_ptrace.o: ../common/process.h
adb_ptrace.o: sr_instruction.h
accesssr.o: accesssr.c
accesssr.o: ../common/adb.h
accesssr.o: sparc.h
accesssr.o: ../common/process.h
accesssr.o: fpascii.h
accesssr.o: ../common/symtab.h
accesssr.o: allregs.h
accesssr.o: sr_instruction.h
disasm.o: disasm.c
disasm.o: sr_general.h
disasm.o: sr_instruction.h
disasm.o: sr_indextables.h
disasm.o: sr_switchindexes.h
disasm.o: sr_opcodetable.h
disasm.o: sr_processor.h
fpasciisr.o: fpasciisr.c
fpasciisr.o: ../common/adb.h
fpasciisr.o: sparc.h
fpasciisr.o: ../common/process.h
fpasciisr.o: fpascii.h
ixtbls.o: ixtbls.c
ixtbls.o: sr_general.h
ixtbls.o: sr_opcodetable.h
ixtbls.o: sr_indextables.h
ixtbls.o: sr_instruction.h
ixtbls.o: sr_switchindexes.h
opcodetable.o: opcodetable.c
opcodetable.o: sr_general.h
opcodetable.o: sr_switchindexes.h
opcodetable.o: sr_opcodetable.h
opcodetable.o: sr_instruction.h
opcodetable.o: sr_sw_trap.h
opsetsr.o: opsetsr.c
opsetsr.o: ../common/adb.h
opsetsr.o: sparc.h
opsetsr.o: ../common/process.h
opsetsr.o: ../common/symtab.h
printsr.o: printsr.c
printsr.o: ../common/adb.h
printsr.o: sparc.h
printsr.o: ../common/process.h
printsr.o: ../common/symtab.h
setupsr.o: setupsr.c
setupsr.o: ../common/adb.h
setupsr.o: sparc.h
setupsr.o: ../common/process.h
setupsr.o: ../common/symtab.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
