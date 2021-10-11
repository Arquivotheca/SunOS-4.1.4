#	@(#)Makefile 1.1 94/10/31 SMI
HFILES = dumprestore.h routed.h rwhod.h timed.h

CHOWN=/etc/chown

HDIR=$(DESTDIR)/usr/include/protocols

all:

install: $(HFILES)
	install -d -o bin -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
