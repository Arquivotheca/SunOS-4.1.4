#
# @(#)Makefile 1.1 94/10/31 SMI; from UCB 4.23 83/07/01
#

DESTDIR=
CRYPT=
CFLAGS=	-O ${CRYPT} -DDEBUG
DEBUGDIR= ${DESTDIR}/usr/lib/debug
CC = /bin/cc

OBJS=	malloc.o mallocmap.o

debug: ${OBJS}

malloc.o: ../malloc.c ../mallint.h
	${CC} ${CFLAGS} -c -I.. ../malloc.c

mallocmap.o: mallocmap.c ../mallint.h
	${CC} ${CFLAGS} -c -I.. mallocmap.c
	
clean:
	rm -f ${OBJS}

install: debug  ${DEBUGDIR}
	-for i in ${OBJS}; do \
		install -m 644 $${i} ${DEBUGDIR}/$${i}; \
	done

${DEBUGDIR}:
	install -d -m 755 ${DEBUGDIR}
