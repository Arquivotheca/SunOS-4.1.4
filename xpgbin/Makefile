#
#@(#)Makefile 1.1 94/10/31 SMI;
#
DESTDIR=

.KEEP_STATE:

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=	cc
SCRIPTSRC= cc.sh

all:


install: ${SCRIPTSRC}
	install -d -o bin -m 755 ${DESTDIR}/usr/xpg2bin
	for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/usr/xpg2bin/$$i); done

clean:

FRC:

