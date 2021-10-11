#
# @(#)Makefile 1.1 94/10/31 SMI
#

# Makefile for obsolete pixrect library test programs

# pixrect library source directory
LPDIR= ../..

DEMOS=	demotest demo demo_time demo_page demo_sample demo_spin demo_mem demo2
DEMOLIB= $(LPDIR)/libpixrect.a
WINLIB= -lsuntool -lsunwindow

# compilation flags
DEBUG=
FLOAT= -fsoft
INC= -I$(LPDIR)/../../include
OPTIM= -O
READONLY= -R

CFLAGS= $(INC) $(FLOAT) $(DEBUG) $(OPTIM)

default: demotest demo

all: $(DEMOS)

demotest: demotest.o demosubs.o $(DEMOLIB)
	$(CC) $(CFLAGS) -o $@ $@.o demosubs.o $(DEMOLIB)

demo: demo.o $(DEMOLIB)
	$(CC) $(CFLAGS) -o $@ $@.o $(DEMOLIB)

testpolytrap: testpolytrap.o $(DEMOLIB)
	$(CC) $(CFLAGS) -o $@ $@.o $(WINLIB) $(DEMOLIB)

demo2: demo2.o $(DEMOLIB)
	$(CC) $(CFLAGS) -o $@ $@.o $(DEMOLIB)

demo_cg: demo_cg.o $(DEMOLIB)
	$(CC) $(CFLAGS) -o $@ $@.o $(DEMOLIB)

demo_sample: demo_sample.o $(DEMOLIB)
	$(CC) $(CFLAGS) -o $@ $@.o $(DEMOLIB)

demo_page: demo_page.o $(DEMOLIB)
	$(CC) $(CFLAGS) -o $@ $@.o $(DEMOLIB)

demo_time: demo_time.o $(DEMOLIB)
	$(CC) $(CFLAGS) -o $@ $@.o $(DEMOLIB)

demo_spin: demo_spin.o $(DEMOLIB)
	$(CC) $(CFLAGS) -o $@ $@.o $(DEMOLIB)

demo_mem: demo_mem.o $(DEMOLIB)
	$(CC) $(CFLAGS) -o $@ $@.o $(DEMOLIB)

clean:
	rm -f *.o core errs $(DEMOS)

demosubs.o: report.h
demotest.o: report.h
# this file seems to be missing!
demotest.o: demoglobals.h
