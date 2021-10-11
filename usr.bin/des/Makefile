#
# @(#)Makefile 1.1 94/10/31 SMI
#
DESTDIR=
CFLAGS=	-O

all:	des

des:	des.o
	${CC} ${CFLAGS} des.o -o des 

install: all
	install -s des ${DESTDIR}/usr/bin

clean:
	-rm -f a.out core des.o des

international:
	-rm -f ${DESTDIR}/usr/bin/des

