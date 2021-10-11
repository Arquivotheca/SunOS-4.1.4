#
# @(#)Makefile 1.1 94/10/31 SMI;
#
# Copyright (c) 1988 by Sun Microsystems, Inc.
#

BINS= fpuversion4
OBJS= fpuversion4.o fpuversion4sub.o
CFLAGS=	-O

.KEEP_STATE:

all: $(BINS)

fpuversion4: $(OBJS)
	$(LINK.c) -o $@ $(OBJS)

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s $(BINS) $(DESTDIR)/usr/etc

clean:
	$(RM) $(BINS) $(OBJS)
