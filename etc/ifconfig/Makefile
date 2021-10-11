#
# @(#)Makefile 1.1 94/10/31 SMI
#
PROG =		ifconfig

CFLAGS =	

LDFLAGS +=	-Bstatic

OBJS=		ifconfig.o revarp.o

.KEEP_STATE:

all:		$(PROG)

$(PROG):	$(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS)

clean:
	-rm -f $(OBJS)

clobber:	clean
	-rm -f $(PROG)

install:	$(PROG)
	install -s $(PROG) $(DESTDIR)/usr/etc
	ln -s ../usr/etc/$(PROG) $(DESTDIR)/etc/$(PROG)
