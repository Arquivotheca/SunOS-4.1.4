#
# @(#)Makefile 1.1 94/10/31 SMI; from UCB 5.1 4/29/85
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
BINS= getty
OBJS= main.o init.o subr.o gettytab.o get_date.o setmode.o
HDRS= gettytab.h

CFLAGS=	-O
LDFLAGS=

.KEEP_STATE:

.INIT: $(HDRS)

all: $(BINS)

$(BINS): $(OBJS)
	$(LINK.c) -o $@ $(OBJS)

setmode.o := CFLAGS += -R

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s $(BINS) $(DESTDIR)/usr/etc

clean:
	$(RM) $(BINS) $(OBJS)
