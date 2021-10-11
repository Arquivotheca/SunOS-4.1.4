#
# @(#)Makefile 1.1 94/10/31 SMI
#

# makefile for gtconfig and GT microcode

BIN = gtconfig gt_lpconfig

LIB = lib/libgt.a

SUBDIR = probe quicktest 

SRC = gtconfig.c
SRC1 = gt_lpconfig.c
OBJ = ${SRC:.c=.o}
OBJ1 = ${SRC1:.c=.o}

UCODE = $(GT_UCODE) $(GT_DSP0) $(GT_DSP1)
GT_UCODE = gt.ucode
GT_DSP0  = gt.c30.ucode
GT_DSP1  = gt.c31.ucode

BINDIR = /usr/etc
LIBDIR = /usr/lib

LDLIBS = -L./lib -lgt -lm
LDLIBS2 = -lsuntool -lsunwindow -lpixrect
CPPFLAGS = -I. -I./include -I../../sys/sbusdev -I../../sys/sun -I../../sys/sundev
DEST = .
CFLAGS = -g  

PPWD = `/bin/pwd`
RM = rm -f
OWNER = bin

.KEEP_STATE:

all: $(SUBDIR) $(BIN) $(UCODE)

${SUBDIR}: FRC
	cd $@; ${MAKE} CC=${CC} ${MFLAGS} all

gtconfig:  $(LIB) $(OBJ)
	$(LINK.c) -o $@ $(OBJ) $(LDLIBS)

gt_lpconfig: $(LIB) $(OBJ1)
	$(CC) $(CFLAGS) -o $@ $(OBJ1) $(LDLIBS) $(LDLIBS2)

install: all
	install -d -m 755 -o bin $(DESTDIR)/usr/diag
	install -d $(DESTDIR)/usr/diag/gt
	@set -x;for i in ${SUBDIR}; do \
	(cd $$i; ${MAKE} CC=${CC} ${MFLAGS} DESTDIR=${DESTDIR} CHOWN=$(CHOWN) install); done

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

$(LIB): 
	(cd lib; ${MAKE} CC=${CC} ${MFLAGS})

clean:
	 rm -f ${BIN} ${OBJ} ${OBJ1}

FRC:
