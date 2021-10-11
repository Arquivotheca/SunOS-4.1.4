#
# @(#)Makefile 1.1 94/10/31 SMI
#
HFILES = tnode.h tfs.h

HDIR=$(DESTDIR)/usr/include/tfs

install_h: $(HFILES) FRC
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)

FRC:
