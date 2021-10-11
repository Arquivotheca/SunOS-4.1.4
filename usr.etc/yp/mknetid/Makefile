#
#  @(#)Makefile 1.1 94/10/31 Copyright (c) 1985 by Sun Microsystems, Inc.
#

SRCS = mknetid.c getname.c 
OBJS = $(SRCS:.c=.o)

CFLAGS= -O
DESTDIR=

mknetid: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBC) -o mknetid

lint:
	lint $(SRCS)

clean: 
	rm -f mknetid $(OBJS)

install: mknetid
	install -s mknetid $(DESTDIR)/usr/etc/yp

