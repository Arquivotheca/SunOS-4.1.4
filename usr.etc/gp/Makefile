#
# @(#)Makefile 1.1 94/10/31 SMI
#

# makefile for gpconfig and GP microcode

ALL = $(BIN) $(UCODE)

BIN = gpconfig
GPCONFIG_SRC = gpconfig.c gputil.c gp2util.c util.c cg12util.c
GPCONFIG_OBJ = $(GPCONFIG_SRC:.c=.o)

UCODE = $(GP1UCODE) $(GP1+UCODE) $(GP2UCODE) $(GSUCODE)
GP2UCODE = gp2.ucode
GSUCODE = gs.ucode
GP1UCODE = gp1cg2.1152.ucode gp1cg2.1024.ucode
GP1+UCODE = gp1+cg2.1152.ucode gp1+cg2.1024.ucode
GP1+ = gp1+cg2.1152.ucode 
GP1 = gp1cg2.1152.ucode 
DESTDIR =
BINDIR = /usr/etc
LIBDIR = /usr/lib

SUBDIR = vp pp as

PPWD = `/bin/pwd`
RM = rm -f
OWNER = bin

# compilation flags

INC = -I../../include -I../../sys
OPTIM = -O
READONLY = 
CPPFLAGS= $(INC)
CFLAGS = $(OPTIM) $(READONLY)

.KEEP_STATE:

all: $(ALL)

gp1+: $(GP1+)

gp1: $(GP1)

cg12util.c:	coff.h

gpconfig: $(GPCONFIG_OBJ)
	$(LINK.c) -o $@ $(GPCONFIG_OBJ)

$(GP1UCODE): as FRC what
	PWD=$(PPWD) \
	cd vp; $(MAKE) -$(MAKEFLAGS) UAS=$$PWD/as/vpas $(@:.ucode=.vp.o)
	PWD=$(PPWD) \
	cd pp; $(MAKE) -$(MAKEFLAGS) UAS=$$PWD/as/ppas $(@:.ucode=.pp.o)
	$(RM) $@
	cat vp/$(@:.ucode=.vp.o) pp/$(@:.ucode=.pp.o) whatstring > $@

$(GP1+UCODE): as FRC what+
	PWD=$(PPWD) \
	cd vp; $(MAKE) -$(MAKEFLAGS) UAS=$$PWD/as/vpas $(@:.ucode=.vp.o)
	PWD=$(PPWD) \
	cd pp; $(MAKE) -$(MAKEFLAGS) UAS=$$PWD/as/ppas $(@:.ucode=.pp.o)
	$(RM) $@
	cat vp/$(@:.ucode=.vp.o) pp/$(@:.ucode=.pp.o) whatstring > $@

as: FRC
	cd as; $(MAKE) -$(MAKEFLAGS) vpas ppas
	
what:	$(GET) whatstring
what+:	$(GET) whatstring+

install: $(ALL)
	-set +e ; \
	for i in $(BIN); do \
		$(RM) $(DESTDIR)$(BINDIR)/$$i; \
		install -c -s -m 755 -o $(OWNER) \
			$$i $(DESTDIR)$(BINDIR)/$$i ; \
	done ; \
	for i in $(UCODE); do \
		$(RM) $(DESTDIR)$(LIBDIR)/$$i; \
		install -c -m 644 -o $(OWNER) \
			$$i $(DESTDIR)$(LIBDIR)/$$i ; \
	done 

clean:
	$(RM) a.out core errs* $(ALL) *.o
	for i in $(SUBDIR) ; do (cd $$i; $(MAKE) -$(MAKEFLAGS) $@); done

spotless: clean
	$(RM) junk* tmp* temp* *.BAK *.CKP .[a-z]*
	for i in $(SUBDIR) ; do (cd $$i; $(MAKE) -$(MAKEFLAGS) $@); done

FRC:

# handy targets for development
dev:	gp1cg2.1152.ucode
local:	dev
	gpconfig gpone0 -u gp1cg2.1152.ucode -f -b cgtwo0 > /tmp/gpconfig.out
	cat /tmp/gpconfig.out
	$(RM) /tmp/gpconfig.out

dev+:	gp1+cg2.1152.ucode
local+:	dev+
	gpconfig gpone0 -u gp1+cg2.1152.ucode -f -b cgtwo0 > /tmp/gpconfig.out
	cat /tmp/gpconfig.out
	$(RM) /tmp/gpconfig.out
