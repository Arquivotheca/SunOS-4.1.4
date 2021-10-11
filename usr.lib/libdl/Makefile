#
# @(#)Makefile 1.1 94/10/31 SMI
#
LIBS= libdl.so.1.0

OBJS= dl.o
HDRS= dlfcn.h

LIBDIR= ${DESTDIR}/usr/lib
HDRDIR= ${DESTDIR}/usr/include

CFLAGS= -O -pic -R

.KEEP_STATE:

.INIT:	${HDRS}

all:	${LIBS}

${LIBS}:	${OBJS}
	ld -assert pure-text -o libdl.so.1.0 dl.o

install:	${LIBS} install_h
	install -m 755 ${LIBS} ${LIBDIR}

install_h:	${HDRS}
	install -m 444 $? ${HDRDIR}

clean:
	${RM} ${LIBS} ${OBJS}
