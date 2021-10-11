#	Makefile for dcptest.
#
#	@(#)Makefile 1.1 94/10/31 SMI;
#

.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros
PROGRAM = dcptest
DESTDIR=
INCLUDES = dcptest.h dcp_amddata.h dcpdata.h dcptest_msg.h $(INCDIR)/sdrtns.h\
	$(ONLINEINC)/libonline.h
LIBS   =

SOURCES = dcptest.c dcp_probe.c dcp_amdtest.c dcptest_msg.c
OBJECTS = $(SOURCES:.c=.o)
LINTFILES = $(SOURCES:.c=.ln)

DBX=-O
# specify DBX=-g for dbx version

CFLAGS= $(DBX) -I${INCDIR} -D`arch`
LDFLAGS = $(DBX)
LINTFLAGS= -D$(REV) -I$(INCDIR)

.KEEP_STATE:

all:	${INCLUDES} ${PROGRAM}

$(PROGRAM): $(OBJECTS)
	${CC} $(LDFLAGS) -o $@ $(OBJECTS) $(SDLIBS) $(LIBS)

install: all
	@if [ $(DESTDIR) ]; then \
	  set -x; install -s dcptest ${DESTDIR}; \
	else \
	  set -x; install dcptest ../../bin; \
	fi

clean:
	rm -f ${PROGRAM} $(OBJECTS) $(LINTFILES) core

lint: $(LINTFILES)
	lint $(LINTFLAGS) $(LINTFILES)

info: FRC
	sccs info
include ../../include/Makefile.end

