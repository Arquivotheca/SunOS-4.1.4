
# @(#)Makefile	1.1 94/10/31 SMI

SUBDIR=	sony_lib lasertool
HDRS=	sony_codes.h sony_laser.h
DEST_HDRS=	$(DESTDIR)/usr/demo/SUNVIDEO/LASERDISC

all:	$(HDRS)
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS}); \
		done

clean:
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} clean); \
		done

install:	all
	install -d -m 755 $(DEST_HDRS);
	install -m 444 $(HDRS) Makefile $(DEST_HDRS); 
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); \
		done

#
