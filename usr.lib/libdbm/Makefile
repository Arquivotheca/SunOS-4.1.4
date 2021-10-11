#
# @(#)Makefile 1.1 94/10/31 SMI; from UCB 4.1 83/06/27
#
LIBS= libdbm.a

OBJS= dbm.o
HDRS= dbm.h

LIBDIR= $(DESTDIR)/usr/lib
HDRDIR= $(DESTDIR)/usr/include

CFLAGS= -O

.KEEP_STATE:

.INIT: $(HDRS)

all: $(LIBS)

$(LIBS): $(OBJS)
	ar rv $@ $?
	ranlib $@

install: $(LIBS) install_h
	install -d -m 755 $(LIBDIR)
	install -m 644 $(LIBS) $(LIBDIR)
	ranlib $(LIBDIR)/$(LIBS)

install_h: $(HDRS)
	install -d -m 755 $(HDRDIR)
	install -m 444 $? $(HDRDIR)

clean:
	$(RM) $(LIBS) $(OBJS)
