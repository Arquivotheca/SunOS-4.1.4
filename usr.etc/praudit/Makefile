#
# @(#)Makefile 1.1 94/10/31 SMI
#
BINS= praudit
OBJS= praudit.o
HDRS= praudit.h

CFLAGS=	  -O
CPPFLAGS= -DNFSSERVER
LDFLAGS= -n

.KEEP_STATE:

.INIT: $(HDRS)

all: $(BINS)

clean:
	$(RM) $(BINS) $(OBJS)

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s -m 755 $(BINS) $(DESTDIR)/usr/etc
