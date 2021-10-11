########################################################################
#
# @(#)Makefile 1.1 94/10/31 SMI
#
# Copyright (c) 1991 by Sun Microsystems, Inc.
#
# Makefile for the demo program
#
# 18-Apr-91 Kevin C. Rushforth	Added SCCS IDs
# 10-May-91 Chris Klein         Added quicktest target
# 16-May-91 John M. Perry	Convert from Xlib frame to XView 
# 20-May-91 John M. Perry	separate quicktest.sv and quicktest.ow targets
# 30-May-91 John M. Perry	add stereo support for XNeWS QUICKTEST
#
########################################################################

BINS = quicktest.ow quicktest.sv
SCRIPT = gt_quicktest load_images
HDL_FILES = f15v.hdl new_x29.hdl sun_logo2.hdl

# A few architecture specific macros:
FLAG_68881:sh=if [ `arch` = sun3 ] ; then echo "-f68881" ; else echo "" ; fi
#MAKEDEP:sh= echo /usr/shbam2/bin/`arch`/makedep

CFLAGS = -fsingle $(FLAG_68881) \
	-I$(OPENWINHOME)/include $(CPPFLAGS) $(XVIEW_FLAGS)
LDFLAGS = -fsingle $(FLAG_68881)
LINK_TYPE=
OPENWINHOME=/openwin_files-V3

ARCH:sh = arch

#HAWK_LIB = -L/usr/shbam2/sim/$(ARCH) -lhcom
#SCCS_SIM = /usr/shbam2/sim/SCCS
#SCCS_OTHER = /usr/shbam2/sim/SCCS_OTHER

CURSES_LIB = -lcurses -ltermcap

SV_LIB = -lsuntool -lsunwindow -lpixrect

XLIB=  -L$(OPENWINHOME)/lib -lolgx \
       -L$(OPENWINHOME)/lib -lX11 \
       -L$(OPENWINHOME)/lib -lXext \
       -L$(OPENWINHOME)/lib -lXau \
       -L$(OPENWINHOME)/lib -lX11 

#.c.o:
#	rm -f $*.o
#	cc $(CFLAGS) -c $*.c

DEMO_H = \
	cmap_grab.h \
	demo.h \
	demo_sv.h \
	hk_comm.h \
	hk_public.h \
	win_grab.h \
	wx.h

DEMO_C = \
	demo_colors.c \
	demo_dials.c \
	demo_dl_create.c \
	demo_globals.c \
	demo_hk_comm.c \
	demo_main.c \
	demo_mat_util.c \
	demo_menus.c \
	demo_obj_control.c \
	demo_rubber_band.c \
	demo_sv.c \
	demo_x.c \
	demo_templates.c \
	demo_transforms.c \
	demo_user_interf.c \
	demo_windows.c \
	hk_comm.c \
	hk_comm_dlx.c \
	hk_error.c \
	hk_init_ctx.c \
	hk_load_hdl.c \
	XGrabWin.c \
	win_grab.c \
	cmap_grab.c

DEMO_O = $(DEMO_C:.c=.o)

QUICK_O = demo_hk_comm.o \
	  demo_x.o \
	  demo_globals.o \
	  demo_main.o \
	  demo_sv.o \
	  demo_dl_create.o \
	  demo_user_interf.o

openwin_check:
	@if [ ! -f /openwin_files-V3/lib/libxview.so.3.[1-9]* ] ; then \
		echo "OPENWINHOME location \"/openwin_files-V3\" does not appear to be OWv3" ; \
		false ; \
	fi

all: openwin_check $(BINS)

# Demo makes the standard "Nelson" demo, unless QUICKTEST is defined
demo: $(DEMO_O)
	rm -f demo 
	cc $(CFLAGS) -o demo $(DEMO_O) $(CURSES_LIB) \
		$(XVIEW_DIR) $(XVIEW_LIB) \
		$(LINK_TYPE) $(HAWK_LIB) $(XLIB) -Bdynamic \
		-lm $(SV_LIB)

quicktest.sv:
	rm -f $(QUICK_O)
	make demo CPPFLAGS=-DQUICKTEST LINK_TYPE=-Bstatic
	mv demo quicktest.sv

quicktest.ow:
	rm -f $(QUICK_O)
	make demo CPPFLAGS=-DQUICKTEST \
		  XVIEW_FLAGS=-DXVIEW \
		  XVIEW_DIR=-L$(OPENWINHOME)/lib \
		  XVIEW_LIB=-lxview
	mv demo quicktest.ow

demo.ow:
	rm -f $(QUICK_O)
	make demo XVIEW_FLAGS=-DXVIEW \
		  XVIEW_DIR=-L$(OPENWINHOME)/lib \
		  XVIEW_LIB=-lxview
	rm -f $(QUICK_O)
	mv demo gt/demo.ow

install: all ${SCRIPT} ${HDL_FILES}
	install -d $(DESTDIR)/usr/diag
	install -d $(DESTDIR)/usr/diag/gt
	install -m 755 -o root gt_quicktest ${DESTDIR}/usr/diag
	install -s -m 755 -o root $(BINS) ${DESTDIR}/usr/diag/gt
	install -m 644 -o root $(HDL_FILES) load_images ${DESTDIR}/usr/diag/gt

clean:
	/usr/ucb/sccs clean
	rm -f ${BINS} *.o

lint: $(DEMO_C)
	rm -f demo.lint
	lint $(DEMO_C) > demo.lint

depend: $(DEMO_C) $(DEMO_H)
	$(MAKEDEP) -N $(DEMO_C) $(DEMO_H)
	rm -f Makefile.BAK
	cp Makefile Makefile.BAK
	echo '/^# Dependencies/+2,$$d' > edmakedep
	echo w >> edmakedep
	ed - Makefile < edmakedep
	cat dependencies >> Makefile
	rm -f dependencies edmakedep

#
# Dependencies - Added by make depend
#
cmap_grab.o: \
	cmap_grab.h
win_grab.o: \
	win_grab.h
XGrabWin.o: \
	wx.h \
	win_grab.h
demo_windows.o: \
	demo.h \
	hk_public.h \
	win_grab.h
demo_user_interf.o: \
	demo.h \
	hk_public.h \
	win_grab.h \
	demo_sv.h
demo_transforms.o: \
	demo.h \
	hk_public.h \
	win_grab.h
demo_templates.o: \
	demo.h \
	hk_public.h \
	win_grab.h
demo_sv.o: \
	demo_sv.h \
	hk_public.h
demo_rubber_band.o: \
	demo.h \
	hk_public.h \
	win_grab.h
demo_obj_control.o: \
	demo.h \
	hk_public.h \
	win_grab.h
demo_menus.o: \
	demo.h \
	hk_public.h \
	win_grab.h
demo_mat_util.o: \
	demo.h \
	hk_public.h \
	win_grab.h
demo_main.o: \
	hk_comm.h \
	demo.h \
	hk_public.h \
	win_grab.h
demo_hk_comm.o: \
	/usr/include/sun/fbio.h \
	hk_comm.h \
	demo.h \
	hk_public.h \
	win_grab.h \
	demo_sv.h
demo_globals.o: \
	demo.h \
	hk_public.h \
	win_grab.h \
	demo_sv.h
demo_dl_create.o: \
	demo.h \
	hk_public.h \
	win_grab.h \
	demo_sv.h
demo_dials.o: \
	demo.h \
	hk_public.h \
	win_grab.h
demo_colors.o: \
	demo.h \
	hk_public.h \
	win_grab.h
hk_load_hdl.o: \
	hk_header.h \
	hasm_hdl.h
