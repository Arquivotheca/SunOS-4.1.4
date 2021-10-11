#
# @(#)Makefile 1.1 94/10/31 SMI; from UCB 4.5 06/12/83
#
BINS= implog implogd

CFLAGS= -O

.KEEP_STATE:

all:	$(BINS)

clean:
	$(RM) $(BINS)

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install $(BINS) $(DESTDIR)/usr/etc
