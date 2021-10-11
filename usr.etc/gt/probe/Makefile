######################################################################
#
# @(#)Makefile 1.1 94/10/31 21:05:00
#
# Copyright (c) 1991 by Sun Microsystems, Inc.
#
# Comment:
#
# REVISION HISTORY
#
# 03/31/91     Roger Pham      Originated
#
######################################################################
#HKFE = /usr/hawk2/diag/hk2_fe/include
HKFE1 = /usr/hawk2/diags/exec/include
HKFE2 = /usr/hawk2/diags/include
HAWKII = -DHWARE -DHK2 -DHFE_HARDWARE

CFLAGS = -g -I${HKFE} -I${HKFE1} -I${HKFE2} -I$(HKFE) $(HAWKII)

#CFLAGS = -O
#CFLAGS = -g

SRC =	gtprobe.c gtmenu.c gtlib.c \
	device_info.c gt_info.c \
	testhsa.c testmia.c rp_xsum.c

OBJ = $(SRC:.c=.o)

HFILES	= gtprobe.h
RFILES	= README

all: gtprobe

gtprobe:  $(OBJ) $(HFILES) $(RFILES)
	$(CC) $(CFLAGS) $(OBJ) -lm -o $@

install: all
	install -s -m 755 -o root gtprobe $(DESTDIR)/usr/diag

clean:
	rm -f gtprobe *.o
purge:
	rm -f gtprobe *.o *.h README *.c
