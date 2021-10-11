#
# @(#)Makefile 1.1 94/10/31 SMI
#
HFILES = wintree.h

HDIR=$(DESTDIR)/usr/include/sunwindowdev

install_h: $(HFILES)
	@set -x; for i in $(HFILES); do \
		install -c -m 444 $$i $(HDIR)/$$i ; \
	done;
