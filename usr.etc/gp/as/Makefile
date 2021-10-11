#
# @(#)Makefile 1.1 94/10/31 SMI
#

# makefile for GP micro-assemblers

ALL= vpas ppas

CFILES= main.c sym.c scan.c assemble.c output.c
HFILES= micro.h
SRCS= $(HFILES) $(CFILES)

# compilation flags
DEBUG=
INC=
OPTIM= -O
READONLY=
CFLAGS= $(OPTIM) $(READONLY) $(DEBUG) $(INC)

all: $(ALL)

vpas: $(SRCS)
	$(CC) $(CFLAGS) -DVIEW -o $@ $(CFILES)

ppas: $(SRCS)
	$(CC) $(CFLAGS) -DPAINT -o $@ $(CFILES)

get: $(SRCS)

clean:
	rm -f a.out core errs* $(ALL) *.o

spotless: clean
	$(RM) junk* tmp* temp* *.BAK *.CKP .[a-z]*
