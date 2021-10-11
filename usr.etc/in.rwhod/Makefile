#
# @(#)Makefile 1.1 94/10/31 SMI; from UCB 4.1 83/07/01
#
BINS= rwhod
CFLAGS= -O

.KEEP_STATE:

all : $(BINS)

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s $(BINS) $(DESTDIR)/usr/etc/in.rwhod
	install -d -m 755 $(DESTDIR)/var/spool/rwho

clean:
	$(RM) $(BINS)
