#
# @(#)Makefile 1.1 94/10/31 SMI
#

LOCALE = C
LANGDIR = usr/share/lib/locale/LANGINFO

all:	${LOCALE}

install: ${LOCALE}
	install -d -m 755 $(DESTDIR)/$(LANGDIR)
	install -m 644 ${LOCALE} $(DESTDIR)/$(LANGDIR)
	rm -f $(DESTDIR)/$(LANGDIR)/default
	ln -s ./${LOCALE} $(DESTDIR)/$(LANGDIR)/default


clean:
	rm -f C
