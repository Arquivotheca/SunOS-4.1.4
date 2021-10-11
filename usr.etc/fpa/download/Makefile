#	@(#)Makefile 1.1 94/10/31 SMI
CFLAGS=-O
GET= sccs get

all: fpa_download

fpa_download: fpa_download.c fpa.h
	${CC} fpa_download.c -o fpa_download

clean:
	rm -f fpa_download core

install:	fpa_download
	install -s fpa_download ${DESTDIR}/usr/etc/fpa/fpa_download

fpa.h:
	$(GET) $@
