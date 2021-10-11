#
# @(#)Makefile 1.1 94/10/31 SMI
#

HFILES = af.h if.h if_arp.h if_ieee802.h netisr.h nit.h nit_buf.h nit_if.h \
	nit_pf.h packetfilt.h raw_cb.h route.h

HDIR=$(DESTDIR)/usr/include/net

install_h: $(HFILES) FRC
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
FRC:
