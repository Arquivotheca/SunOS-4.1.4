#
# @(#) Makefile 1.1 10/31/94 20:04:00  @(#)
#
SUBDIRS = fddi

all: FRC
	for i in $(SUBDIRS) ; do ( cd $$i ; $(MAKE) $(MFLAGS) all); done

clean: FRC
	for i in $(SUBDIRS) ; do ( cd $$i ; $(MAKE) $(MFLAGS) clean); done

lint: FRC
	for i in $(SUBDIRS) ; do ( cd $$i ; $(MAKE) $(MFLAGS) lint); done

HDIR = $(DESTDIR)/usr/include/sunfw

install_h: FRC
	-mkdir $(HDIR)
	set -x; for i in $(SUBDIRS) ; do \
		rm -fr $(HDIR)/$$i; \
		-mkdir $(HDIR)/$$i; \
		$(CHOWN) bin $(HDIR)/$$i; \
		( cd $$i; $(MAKE) install_h CHOWN=$(CHOWN) DESTDIR=$(DESTDIR) );
	done;

install: FRC
	-mkdir $(HDIR)
	set -x; for i in $(SUBDIRS) ; do \
		(cd $$i; $(MAKE) ${MFLAGS} DESTDIR=${DESTDIR} install); done
	done;

FRC:
