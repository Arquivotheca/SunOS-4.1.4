#
# @(#)Makefile 1.1 94/10/31 SMI
#
PROG =		hostconfig

CFLAGS =	

LDFLAGS +=	-Bstatic

OBJS=		$(PROG).o

.KEEP_STATE:

all:		$(PROG)

$(PROG):	$(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) -lrpcsvc

clean:
	-rm -f $(OBJS)

clobber:	clean
	-rm -f $(PROG)

install:	$(PROG)
	install -s $(PROG) $(DESTDIR)/usr/etc
	install -s $(PROG) $(DESTDIR)/usr/kvm/boot
