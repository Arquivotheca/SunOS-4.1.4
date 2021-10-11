#
# @(#)Makefile 1.1 94/10/31 SMI
#
HFILES = lnode.h loinfo.h

HDIR=$(DESTDIR)/usr/include/lofs

install_h: $(HFILES)
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
