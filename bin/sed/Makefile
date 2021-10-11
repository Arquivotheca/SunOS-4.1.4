#
# @(#)Makefile 1.1 94/10/31 SMI; from S5R2 1.7
#
#	sed make file

SRCDIR = ../../5bin/sed
CFLAGS = -O -I$(SRCDIR)
FILES = sed0.o sed1.o

sed:	$(FILES)
	$(CC) -o sed $(FILES)

sed0.o: $(SRCDIR)/sed0.c $(SRCDIR)/sed.h
	$(CC) -c $(CFLAGS) $(SRCDIR)/sed0.c

sed1.o: $(SRCDIR)/sed1.c $(SRCDIR)/sed.h
	$(CC) -c $(CFLAGS) $(SRCDIR)/sed1.c

install: sed
	install -s sed $(DESTDIR)/usr/bin

clean:
	rm -f $(FILES) sed
