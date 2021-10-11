#
# @(#)Makefile 1.1 10/31/94
#	Makefile for Pascal runtime library
#

CPPFLAGS = -DLIBPC
CFLAGS = -O ${CPPFLAGS}
LIB = ${DESTDIR}/usr/lib
INSTALL= install -c -m 644
CPP= /lib/cpp
RM = /bin/rm -f
RANLIB= ranlib
CPU= `mach`	# default target == host

CMDS =	ACTFILE.c ADDT.c ARGV.c ASRT.c ASRTS.c ATAN.c BUFF.c CARD.c \
	CASERNG.c CHR.c CLCK.c COS.c CTTOT.c DATE.c DEFNAME.c \
	DFDISPOSE.c DISPOSE.c EXCEPT.c EXP.c EXPO.c FCALL.c FLUSH.c \
	FNIL.c FNILCHK.c FRTN.c FSAV.c GET.c GETC.c HALT.c IN.c INCT.c LINO.c \
	LLIMIT.c LN.c MAX.c MULT.c NAM.c NEW.c NIL.c PACK.c \
	PAGE.c PRED.c PUT.c PUTC.c RANDOM.c RANG4.c READ4.c READ8.c READC.c \
	READS.c READV.c SUBSTR.c VSUBSTR.c VBLANKS.c\
	READE.c READLN.c RELEQ.c RELNE.c RELSGE.c RELSGT.c RELSLE.c \
	RELSLT.c RELTGE.c RELTGT.c RELTLE.c RELTLT.c REMOVE.c RESET.c \
	REWRITE.c ROUND.c RSNG4.c SCLCK.c SEED.c SIN.c SQRT.c STLIM.c \
	SUBSC.c SUBSCZ.c SUBT.c SUCC.c TEOF.c TEOLN.c TIME.c TRUNC.c \
	UNIT.c UNPACK.c WRITEC.c WRITEF.c WRITES.c WRITLN.c \
	DROUND.c str.c main.c MAIN.c RANGE_CHECK.c

SYS =	ERROR.c GETNAME.c IOSYNC.c PCEXIT.c PCLEANUP.c PCLOSE.c PCSTART.c \
	PFCLOSE.c PERROR.c PFLUSH.c PMFLUSH.c UNSYNC.c \
	blkclr.c blkcpy.c ioerrs.c

EXTN =	APPEND.c SEEK.c TELL.c GETFILE.c

HDRS =	h00vars.h ioerrs.h libpc.h unixio.h whoami.h

SRCS =	${CMDS} ${SYS} ${EXTN} ${HDRS}

OBJS =	ACTFILE.o ADDT.o ARGV.o ASRT.o ASRTS.o ATAN.o BUFF.o CARD.o \
	CASERNG.o CHR.o CLCK.o COS.o CTTOT.o DATE.o DEFNAME.o \
	DFDISPOSE.o DISPOSE.o EXCEPT.o EXP.o EXPO.o FCALL.o FLUSH.o \
	FNIL.o FNILCHK.o FRTN.o FSAV.o GET.o GETC.o HALT.o IN.o INCT.o LINO.o \
	LLIMIT.o LN.o MAX.o MULT.o NAM.o NEW.o NIL.o PACK.o\
	PAGE.o PRED.o PUT.o PUTC.o RANDOM.o RANG4.o READ4.o READ8.o READC.o\
	READS.o READV.o SUBSTR.o VSUBSTR.o VBLANKS.o\
	READE.o READLN.o RELEQ.o RELNE.o RELSGE.o RELSGT.o RELSLE.o\
	RELSLT.o RELTGE.o RELTGT.o RELTLE.o RELTLT.o REMOVE.o RESET.o\
	REWRITE.o ROUND.o RSNG4.o SCLCK.o SEED.o SIN.o SQRT.o STLIM.o\
	SUBSC.o SUBSCZ.o SUBT.o SUCC.o TEOF.o TEOLN.o TIME.o TRUNC.o\
	UNIT.o UNPACK.o WRITEC.o WRITEF.o WRITES.o WRITLN.o \
	DROUND.o str.o main.o MAIN.o RANGE_CHECK.o

SYOBJ =	ERROR.o GETNAME.o IOSYNC.o PCEXIT.o PCLEANUP.o PCLOSE.o PCSTART.o \
	PFCLOSE.o PERROR.o PFLUSH.o PMFLUSH.o UNSYNC.o \
	blkclr.o blkcpy.o ioerrs.o

EXOBJ =	APPEND.o SEEK.o TELL.o GETFILE.o

.DEFAULT:
	sccs get -G$@ $@

.c.o:
	${CC} -p ${CFLAGS} -c $*.c
	mv $@ tmp.o
	${LD} -X -r -o profiled/$@ tmp.o
	${CC} ${CFLAGS} -c $*.c
	mv $@ tmp.o
	${LD} -x -r -o $@ tmp.o

DIRS =	profiled

libpc.a libpc_p.a: $(DIRS) $(SRCS) ${OBJS} ${SYOBJ} ${EXOBJ}
	cd profiled; ar cru ../libpc_p.a ${OBJS} ${SYOBJ} ${EXOBJ}
	${RANLIB} libpc_p.a
	ar cru libpc.a ${OBJS} ${SYOBJ} ${EXOBJ}
	${RANLIB} libpc.a

$(DIRS):
	if test ! -d $@; then mkdir $@; fi;

HFILES = unixio.h unixio.i
HDIR=$(DESTDIR)/usr/include/pascal
CHOWN=/etc/chown

install: libpc.a libpc_p.a install_h
	${INSTALL} libpc_p.a ${LIB}/libpc_p.a
	${RANLIB} ${LIB}/libpc_p.a
	${INSTALL} libpc.a ${LIB}/libpc.a
	${RANLIB} ${LIB}/libpc.a
	case $(CPU) in \
	mc68010|mc68020) cd m68k; make install DESTDIR=${DESTDIR} ;;\
	sparc)	 cd sparc; make install DESTDIR=${DESTDIR} ;;\
	esac

install_h: $(HFILES)
	install -d -o bin -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)

clean:
	${RM} libpc.a libpc_p.a *.o profiled/*.o a.out lpr tmp core errs

depend:	${SRCS}
	rm -f makedep
	for i in ${CMDS} ${SYS} ${EXTN}; do \
	    ( $(CPP) -M $(CPPFLAGS) $$i >> makedep ); \
	done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it
# DEPENDENCIES MUST END AT END OF FILE
ACTFILE.o: ACTFILE.c
ACTFILE.o: ./h00vars.h
ACTFILE.o: /usr/include/stdio.h
ACTFILE.o: ./whoami.h
ADDT.o: ADDT.c
ARGV.o: ARGV.c
ARGV.o: ./h00vars.h
ARGV.o: /usr/include/stdio.h
ARGV.o: ./whoami.h
ASRT.o: ASRT.c
ASRTS.o: ASRTS.c
ATAN.o: ATAN.c
ATAN.o: /usr/include/math.h
ATAN.o: /usr/include/floatingpoint.h
ATAN.o: /usr/include/sys/ieeefp.h
BUFF.o: BUFF.c
BUFF.o: ./h00vars.h
BUFF.o: /usr/include/stdio.h
BUFF.o: ./whoami.h
CARD.o: CARD.c
CASERNG.o: CASERNG.c
CHR.o: CHR.c
CLCK.o: CLCK.c
COS.o: COS.c
COS.o: /usr/include/math.h
COS.o: /usr/include/floatingpoint.h
COS.o: /usr/include/sys/ieeefp.h
CTTOT.o: CTTOT.c
CTTOT.o: ./whoami.h
CTTOT.o: ./h00vars.h
CTTOT.o: /usr/include/stdio.h
CTTOT.o: ./whoami.h
CTTOT.o: /usr/include/varargs.h
DATE.o: DATE.c
DEFNAME.o: DEFNAME.c
DEFNAME.o: ./h00vars.h
DEFNAME.o: /usr/include/stdio.h
DEFNAME.o: ./whoami.h
DFDISPOSE.o: DFDISPOSE.c
DFDISPOSE.o: ./h00vars.h
DFDISPOSE.o: /usr/include/stdio.h
DFDISPOSE.o: ./whoami.h
DFDISPOSE.o: ./libpc.h
DISPOSE.o: DISPOSE.c
DISPOSE.o: ./h00vars.h
DISPOSE.o: /usr/include/stdio.h
DISPOSE.o: ./whoami.h
EXCEPT.o: EXCEPT.c
EXCEPT.o: /usr/include/signal.h
EXP.o: EXP.c
EXP.o: /usr/include/math.h
EXP.o: /usr/include/floatingpoint.h
EXP.o: /usr/include/sys/ieeefp.h
EXPO.o: EXPO.c
FCALL.o: FCALL.c
FCALL.o: ./h00vars.h
FCALL.o: /usr/include/stdio.h
FCALL.o: ./whoami.h
FLUSH.o: FLUSH.c
FLUSH.o: ./h00vars.h
FLUSH.o: /usr/include/stdio.h
FLUSH.o: ./whoami.h
FNIL.o: FNIL.c
FNIL.o: ./h00vars.h
FNIL.o: /usr/include/stdio.h
FNIL.o: ./whoami.h
FNIL.o: ./ioerrs.h
FNILCHK.o: FNILCHK.c
FNILCHK.o: ./h00vars.h
FNILCHK.o: /usr/include/stdio.h
FNILCHK.o: ./whoami.h
FNILCHK.o: ./ioerrs.h
FRTN.o: FRTN.c
FRTN.o: ./h00vars.h
FRTN.o: /usr/include/stdio.h
FRTN.o: ./whoami.h
FSAV.o: FSAV.c
FSAV.o: ./h00vars.h
FSAV.o: /usr/include/stdio.h
FSAV.o: ./whoami.h
GET.o: GET.c
GET.o: ./h00vars.h
GET.o: /usr/include/stdio.h
GET.o: ./whoami.h
GET.o: ./ioerrs.h
GETC.o: GETC.c
GETC.o: ./h00vars.h
GETC.o: /usr/include/stdio.h
GETC.o: ./whoami.h
GETC.o: ./ioerrs.h
HALT.o: HALT.c
HALT.o: ./h00vars.h
HALT.o: /usr/include/stdio.h
HALT.o: ./whoami.h
IN.o: IN.c
IN.o: ./h00vars.h
IN.o: /usr/include/stdio.h
IN.o: ./whoami.h
INCT.o: INCT.c
INCT.o: ./h00vars.h
INCT.o: /usr/include/stdio.h
INCT.o: ./whoami.h
INCT.o: /usr/include/varargs.h
LINO.o: LINO.c
LINO.o: ./h00vars.h
LINO.o: /usr/include/stdio.h
LINO.o: ./whoami.h
LLIMIT.o: LLIMIT.c
LLIMIT.o: ./h00vars.h
LLIMIT.o: /usr/include/stdio.h
LLIMIT.o: ./whoami.h
LN.o: LN.c
LN.o: /usr/include/math.h
LN.o: /usr/include/floatingpoint.h
LN.o: /usr/include/sys/ieeefp.h
MAX.o: MAX.c
MAX.o: ./h00vars.h
MAX.o: /usr/include/stdio.h
MAX.o: ./whoami.h
MULT.o: MULT.c
NAM.o: NAM.c
NAM.o: ./h00vars.h
NAM.o: /usr/include/stdio.h
NAM.o: ./whoami.h
NEW.o: NEW.c
NEW.o: ./h00vars.h
NEW.o: /usr/include/stdio.h
NEW.o: ./whoami.h
NIL.o: NIL.c
NIL.o: ./h00vars.h
NIL.o: /usr/include/stdio.h
NIL.o: ./whoami.h
PACK.o: PACK.c
PAGE.o: PAGE.c
PAGE.o: ./h00vars.h
PAGE.o: /usr/include/stdio.h
PAGE.o: ./whoami.h
PRED.o: PRED.c
PUT.o: PUT.c
PUT.o: ./h00vars.h
PUT.o: /usr/include/stdio.h
PUT.o: ./whoami.h
PUTC.o: PUTC.c
PUTC.o: ./h00vars.h
PUTC.o: /usr/include/stdio.h
PUTC.o: ./whoami.h
PUTC.o: ./ioerrs.h
RANDOM.o: RANDOM.c
RANDOM.o: ./h00vars.h
RANDOM.o: /usr/include/stdio.h
RANDOM.o: ./whoami.h
RANG4.o: RANG4.c
READ4.o: READ4.c
READ4.o: ./h00vars.h
READ4.o: /usr/include/stdio.h
READ4.o: ./whoami.h
READ4.o: /usr/include/errno.h
READ4.o: /usr/include/sys/errno.h
READ8.o: READ8.c
READ8.o: ./h00vars.h
READ8.o: /usr/include/stdio.h
READ8.o: ./whoami.h
READ8.o: /usr/include/errno.h
READ8.o: /usr/include/sys/errno.h
READ8.o: /usr/include/floatingpoint.h
READ8.o: /usr/include/sys/ieeefp.h
READC.o: READC.c
READC.o: ./h00vars.h
READC.o: /usr/include/stdio.h
READC.o: ./whoami.h
READC.o: ./ioerrs.h
READS.o: READS.c
READS.o: ./h00vars.h
READS.o: /usr/include/stdio.h
READS.o: ./whoami.h
READS.o: /usr/include/errno.h
READS.o: /usr/include/sys/errno.h
READV.o: READV.c
READV.o: ./h00vars.h
READV.o: /usr/include/stdio.h
READV.o: ./whoami.h
READV.o: /usr/include/errno.h
READV.o: /usr/include/sys/errno.h
SUBSTR.o: SUBSTR.c
SUBSTR.o: ./h00vars.h
SUBSTR.o: /usr/include/stdio.h
SUBSTR.o: ./whoami.h
VSUBSTR.o: VSUBSTR.c
VSUBSTR.o: ./h00vars.h
VSUBSTR.o: /usr/include/stdio.h
VSUBSTR.o: ./whoami.h
VBLANKS.o: VBLANKS.c
VBLANKS.o: ./h00vars.h
VBLANKS.o: /usr/include/stdio.h
VBLANKS.o: ./whoami.h
VBLANKS.o: /usr/include/errno.h
VBLANKS.o: /usr/include/sys/errno.h
READE.o: READE.c
READE.o: ./h00vars.h
READE.o: /usr/include/stdio.h
READE.o: ./whoami.h
READLN.o: READLN.c
READLN.o: ./h00vars.h
READLN.o: /usr/include/stdio.h
READLN.o: ./whoami.h
READLN.o: ./ioerrs.h
RELEQ.o: RELEQ.c
RELEQ.o: ./h00vars.h
RELEQ.o: /usr/include/stdio.h
RELEQ.o: ./whoami.h
RELNE.o: RELNE.c
RELNE.o: ./h00vars.h
RELNE.o: /usr/include/stdio.h
RELNE.o: ./whoami.h
RELSGE.o: RELSGE.c
RELSGE.o: ./h00vars.h
RELSGE.o: /usr/include/stdio.h
RELSGE.o: ./whoami.h
RELSGT.o: RELSGT.c
RELSGT.o: ./h00vars.h
RELSGT.o: /usr/include/stdio.h
RELSGT.o: ./whoami.h
RELSLE.o: RELSLE.c
RELSLE.o: ./h00vars.h
RELSLE.o: /usr/include/stdio.h
RELSLE.o: ./whoami.h
RELSLT.o: RELSLT.c
RELSLT.o: ./h00vars.h
RELSLT.o: /usr/include/stdio.h
RELSLT.o: ./whoami.h
RELTGE.o: RELTGE.c
RELTGE.o: ./h00vars.h
RELTGE.o: /usr/include/stdio.h
RELTGE.o: ./whoami.h
RELTGT.o: RELTGT.c
RELTGT.o: ./h00vars.h
RELTGT.o: /usr/include/stdio.h
RELTGT.o: ./whoami.h
RELTLE.o: RELTLE.c
RELTLE.o: ./h00vars.h
RELTLE.o: /usr/include/stdio.h
RELTLE.o: ./whoami.h
RELTLT.o: RELTLT.c
RELTLT.o: ./h00vars.h
RELTLT.o: /usr/include/stdio.h
RELTLT.o: ./whoami.h
REMOVE.o: REMOVE.c
REMOVE.o: ./h00vars.h
REMOVE.o: /usr/include/stdio.h
REMOVE.o: ./whoami.h
RESET.o: RESET.c
RESET.o: ./h00vars.h
RESET.o: /usr/include/stdio.h
RESET.o: ./whoami.h
REWRITE.o: REWRITE.c
REWRITE.o: ./h00vars.h
REWRITE.o: /usr/include/stdio.h
REWRITE.o: ./whoami.h
ROUND.o: ROUND.c
RSNG4.o: RSNG4.c
SCLCK.o: SCLCK.c
SEED.o: SEED.c
SEED.o: ./h00vars.h
SEED.o: /usr/include/stdio.h
SEED.o: ./whoami.h
SIN.o: SIN.c
SIN.o: /usr/include/math.h
SIN.o: /usr/include/floatingpoint.h
SIN.o: /usr/include/sys/ieeefp.h
SQRT.o: SQRT.c
SQRT.o: /usr/include/math.h
SQRT.o: /usr/include/floatingpoint.h
SQRT.o: /usr/include/sys/ieeefp.h
STLIM.o: STLIM.c
STLIM.o: ./h00vars.h
STLIM.o: /usr/include/stdio.h
STLIM.o: ./whoami.h
SUBSC.o: SUBSC.c
SUBSCZ.o: SUBSCZ.c
SUBT.o: SUBT.c
SUCC.o: SUCC.c
TEOF.o: TEOF.c
TEOF.o: ./h00vars.h
TEOF.o: /usr/include/stdio.h
TEOF.o: ./whoami.h
TEOLN.o: TEOLN.c
TEOLN.o: ./h00vars.h
TEOLN.o: /usr/include/stdio.h
TEOLN.o: ./whoami.h
TEOLN.o: ./ioerrs.h
TIME.o: TIME.c
TRUNC.o: TRUNC.c
TRUNC.o: ./h00vars.h
TRUNC.o: /usr/include/stdio.h
TRUNC.o: ./whoami.h
UNIT.o: UNIT.c
UNIT.o: ./h00vars.h
UNIT.o: /usr/include/stdio.h
UNIT.o: ./whoami.h
UNPACK.o: UNPACK.c
WRITEC.o: WRITEC.c
WRITEC.o: ./h00vars.h
WRITEC.o: /usr/include/stdio.h
WRITEC.o: ./whoami.h
WRITEC.o: ./ioerrs.h
WRITEF.o: WRITEF.c
WRITEF.o: ./h00vars.h
WRITEF.o: /usr/include/stdio.h
WRITEF.o: ./whoami.h
WRITES.o: WRITES.c
WRITES.o: ./h00vars.h
WRITES.o: /usr/include/stdio.h
WRITES.o: ./whoami.h
WRITLN.o: WRITLN.c
WRITLN.o: ./h00vars.h
WRITLN.o: /usr/include/stdio.h
WRITLN.o: ./whoami.h
DROUND.o: DROUND.c
str.o: str.c
str.o: /usr/include/strings.h
main.o: main.c
MAIN.o: MAIN.c
RANGE_CHECK.o: RANGE_CHECK.c
RANGE_CHECK.o: /usr/include/signal.h
RANGE_CHECK.o: /usr/include/machine/trap.h
ERROR.o: ERROR.c
ERROR.o: /usr/include/stdio.h
ERROR.o: /usr/include/signal.h
GETNAME.o: GETNAME.c
GETNAME.o: ./h00vars.h
GETNAME.o: /usr/include/stdio.h
GETNAME.o: ./whoami.h
GETNAME.o: ./libpc.h
IOSYNC.o: IOSYNC.c
IOSYNC.o: ./h00vars.h
IOSYNC.o: /usr/include/stdio.h
IOSYNC.o: ./whoami.h
IOSYNC.o: ./ioerrs.h
PCEXIT.o: PCEXIT.c
PCEXIT.o: ./h00vars.h
PCEXIT.o: /usr/include/stdio.h
PCEXIT.o: ./whoami.h
PCLEANUP.o: PCLEANUP.c
PCLEANUP.o: ./h00vars.h
PCLEANUP.o: /usr/include/stdio.h
PCLEANUP.o: ./whoami.h
PCLOSE.o: PCLOSE.c
PCLOSE.o: ./h00vars.h
PCLOSE.o: /usr/include/stdio.h
PCLOSE.o: ./whoami.h
PCLOSE.o: ./libpc.h
PCSTART.o: PCSTART.c
PCSTART.o: /usr/include/signal.h
PCSTART.o: ./h00vars.h
PCSTART.o: /usr/include/stdio.h
PCSTART.o: ./whoami.h
PCSTART.o: ./libpc.h
PFCLOSE.o: PFCLOSE.c
PFCLOSE.o: ./h00vars.h
PFCLOSE.o: /usr/include/stdio.h
PFCLOSE.o: ./whoami.h
PFCLOSE.o: ./libpc.h
PERROR.o: PERROR.c
PERROR.o: /usr/include/stdio.h
PERROR.o: /usr/include/signal.h
PFLUSH.o: PFLUSH.c
PFLUSH.o: ./h00vars.h
PFLUSH.o: /usr/include/stdio.h
PFLUSH.o: ./whoami.h
PMFLUSH.o: PMFLUSH.c
PMFLUSH.o: ./h00vars.h
PMFLUSH.o: /usr/include/stdio.h
PMFLUSH.o: ./whoami.h
UNSYNC.o: UNSYNC.c
UNSYNC.o: ./h00vars.h
UNSYNC.o: /usr/include/stdio.h
UNSYNC.o: ./whoami.h
blkclr.o: blkclr.c
blkcpy.o: blkcpy.c
ioerrs.o: ioerrs.c
ioerrs.o: ./h00vars.h
ioerrs.o: /usr/include/stdio.h
ioerrs.o: ./whoami.h
ioerrs.o: ./ioerrs.h
APPEND.o: APPEND.c
APPEND.o: ./h00vars.h
APPEND.o: /usr/include/stdio.h
APPEND.o: ./whoami.h
SEEK.o: SEEK.c
SEEK.o: ./h00vars.h
SEEK.o: /usr/include/stdio.h
SEEK.o: ./whoami.h
TELL.o: TELL.c
TELL.o: ./h00vars.h
TELL.o: /usr/include/stdio.h
TELL.o: ./whoami.h
GETFILE.o: GETFILE.c
GETFILE.o: ./h00vars.h
GETFILE.o: /usr/include/stdio.h
GETFILE.o: ./whoami.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
