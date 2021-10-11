#
# @(#)Makefile 1.1 94/10/31 SMI
#

DESTDIR=

all: FRC dbx dbxtool

dbx: FRC
	cd dbx/${CPU}; make ${MFLAGS} CC="${CC}" AS="${AS}" LD="${LD}"

dbxtool: FRC
	cd $@; make ${MFLAGS} CC="${CC}" AS="${AS}" LD="${LD}"

install: all
	install -s dbx/${CPU}/dbx ${DESTDIR}/usr/ucb/dbx
	install -s dbxtool/dbxtool ${DESTDIR}/usr/bin/dbxtool

clean:
	cd dbx/${CPU}; make ${MFLAGS} clean
	cd dbxtool; make ${MFLAGS} clean

FRC:
