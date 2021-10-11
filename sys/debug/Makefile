#
# @(#)Makefile 1.1 94/10/31 SMI
#
HDIR=$(DESTDIR)/usr/include/debug
HFILES= debug.h debugger.h
FILES= $(HFILES) main.c support.c

all: $(FILES)

clean:

install: $(FILES)

install_h: $(FILES)
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
