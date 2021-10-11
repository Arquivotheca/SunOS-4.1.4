#
# @(#)Makefile 1.1 94/10/31 SMI
#
HFILES=fifo.h fifonode.h snode.h
HDIR=$(DESTDIR)/usr/include/specfs

all: $(HFILES)

clean:

install: $(HFILES)

install_h: $(HFILES)
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
