#
# @(#)Makefile 1.1 94/10/31 SMI
#

DESTDIR=
CFLAGS=	-O
OBJS=	lifetool.o life.o circle.o patterns.o
LIBS= -lsuntool -lsunwindow -lpixrect
CC= /bin/cc

life: ${OBJS}
	$(CC) ${CFLAGS} -o life ${OBJS} ${LIBS}

${OBJS}: life.h

install: life
	install -s life ${DESTDIR}/usr/games/life
clean:
	rm -f ${OBJS} life
