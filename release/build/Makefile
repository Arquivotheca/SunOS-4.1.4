#
# @(#)Makefile 1.1 94/10/31 SMI
#

FILES= build nightly sccs_clean
DESTDIR= /var/adm/build

all: $(FILES)

install: $(FILES)
	install -d -m 775 -o root -g staff $(DESTDIR)
	install $(FILES) $(DESTDIR)
