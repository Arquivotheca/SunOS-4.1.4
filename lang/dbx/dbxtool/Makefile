#
# @(#)Makefile 1.1 94/10/31 SMI
#

.KEEP_STATE:

WINCLUDE=/usr/include
WLIBS=/usr/lib/lib

CPU	= `mach`

# See mkmkdefs!
DBXDIR	= ../dbx/${CPU}

COMDIR	= ../dbx/common
AOUT	= dbxtool
DBX	=
RM     = /bin/rm

CFILES = bp.c buttons.c cmd.c dbxenv.c dbxtool.c dbxlib.c \
	 decorations.c disp.c pipe.c selection.c \
	 src.c status.c confirm.c menu.c
HFILES = typedefs.h ttysw_impl.h
CPPFLAGS = -I${DBXDIR} -I${COMDIR} -I${WINCLUDE} ${DBX}

PIX= arrow.10 arrow.12 arrow.14 arrow.16 hollow.10 hollow.12 hollow.14 \
	hollow.16 solid.10 solid.12 solid.14 solid.16 stoparrow.10	\
	stoparrow.12 stoparrow.14 stoparrow.16 stopsign.10 stopsign.12	 \
	stopsign.14 stopsign.16

CFLAGS = -O
OBJ    = bp.o buttons.o cmd.o dbxenv.o dbxtool.o dbxlib.o \
	 decorations.o disp.o pipe.o selection.o \
	 src.o status.o confirm.o menu.o
HDR    = bp.hdr buttons.hdr cmd.hdr dbxenv.hdr dbxtool.hdr dbxlib.hdr \
	 decorations.hdr disp.hdr pipe.hdr selection.hdr src.hdr status.hdr \
	 menu.hdr

WINLIBS= -lsuntool -lsunwindow -lpixrect
# or, when you're working with a test version of the libraries,
# WINLIBS= ${WLIBS}suntool.a ${WLIBS}sunwindow.a ${WLIBS}pixrect.a
LIBS   =  ${WINLIBS}

.SUFFIXES:
.SUFFIXES: .hdr .c .s .o .c~ .s~ .h~ .h .10~ .12~ .14~ .16~ .10 .12 .14 .16

.10~.10:
	$(GET) -G$*.10 $(GFLAGS) $<

.12~.12:
	$(GET) -G$*.12 $(GFLAGS) $<

.14~.14:
	$(GET) -G$*.14 $(GFLAGS) $<

.16~.16:
	$(GET) -G$*.16 $(GFLAGS) $<

.c.hdr:
	${DBXDIR}/makedefs $*.c $*.h

${AOUT}:	mkmkdefs ${PIX} ${HFILES} ${HDR} ${OBJ}
	@echo Linking ${AOUT}
	@rm -f ${AOUT}
	@${CC} ${DBX} -o ${AOUT} ${OBJ} ${LIBS}

#
# Before we get started, we require ${DBXDIR} point to
# the m68k directory, and we require makedefs to exist.
mkmkdefs:
	@echo "Checking 'mach' and 'mkdefs'"
	@if [ ! -d ${DBXDIR} ] ; then \
	    ( cd ../dbx ; ln -s m68k ${CPU} ) \
	fi
	@if [ ! -x ${DBXDIR}/makedefs ] ; then \
	    ( cd ${DBXDIR}; $(MAKE) makedefs ) \
	fi

clean:
	rm -f ${HDR} ${OBJ} ${AOUT}
	for i in ${HDR}; do\
		rm -f `basename $$i .hdr`.h; \
	done


#
# A few explicit .h dependencies, to "jump-start" the make.
#
buttons.o menu.o pipe.o selection.o : pipeout.hdr

pipeout.hdr : ${COMDIR}/pipeout.c
	${DBXDIR}/makedefs $? `basename $@ .hdr`.h

tags:
	ctags $(CFILES)
