#
# @(#)Makefile 1.1 10/31/94
#

DESTDIR=

FILES=	pc2.il

.DEFAULT:
	sccs get -G$@ $@

all: ${FILES}

install: all
	for f in ${FILES}; do \
		install -m 444 $$f ${DESTDIR}/usr/lib/$$f; \
	done

clean:
	rm -f ${FILES}

FRC:
