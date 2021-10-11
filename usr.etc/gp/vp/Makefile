#
# @(#)Makefile 1.1 94/10/31 SMI
#

# makefile for vp microcode

PROC=vp

ALL = $(UCODE) $(UCODE+)
UCODE = gp1cg2.1152.$(PROC).o gp1cg2.1024.$(PROC).o
UCODE+ = gp1+cg2.1152.$(PROC).o gp1+cg2.1024.$(PROC).o

INCS = vp.h
MAIN = main.vp.u

SRCS = $(INCS) $(MAIN) \
	2Dfp.vp.u \
	3Dfp.vp.u \
	attributes.vp.u \
	cgi.vp.u \
	clpvec2.vp.u \
	clpvec3.vp.u \
	dummy.vp.u \
	gbuffer.vp.u \
	multiply.vp.u \
	pixrect.vp.u \
	polyscan.vp.u \
	polyscan3.vp.u \
	recip.vp.u \
	ropfb.vp.u \
	ropnull.vp.u \
	roptex.vp.u \
	statatts.vp.u \
	statpixrect.vp.u \
	tex.vp.u \
	xfpolygon2.vp.u \
	xfpolygon3.vp.u 

SRCS+ =	$(SRCS) \
	zbline.vp.u \
	depthcue.vp.u \
	stattex.vp.u \
	statmultiply.vp.u \
	statrecip.vp.u \
	xfpoint.vp.u \
	pick.vp.u
	
CPP = /lib/cpp
UAS = $(PROC)as
RM = rm -f

#.DEFAULT: ; $(GET) -G$@ $(GFLAGS) SCCS/s.$<

all: $(ALL)

gp1cg2.1152.$(PROC).o: $(SRCS)
	$(RM) $@ $(@:.o=.list)
	$(CPP) -DSWIDTH=1152 $(MAIN) | $(UAS) $@ > $(@:.o=.list)

gp1cg2.1024.$(PROC).o: $(SRCS)
	$(RM) $@ $(@:.o=.list)
	$(CPP) -DSWIDTH=1024 $(MAIN) | $(UAS) $@ > $(@:.o=.list)

gp1+cg2.1152.$(PROC).o: $(SRCS+)
	$(RM) $@ $(@:.o=.list)
	$(CPP) -DSWIDTH=1152 -DGPPLUS $(MAIN) | $(UAS) $@ > $(@:.o=.list)

gp1+cg2.1024.$(PROC).o: $(SRCS+)
	$(RM) $@ $(@:.o=.list)
	$(CPP) -DSWIDTH=1024 -DGPPLUS $(MAIN) | $(UAS) $@ > $(@:.o=.list)

get:	$(SRCS+)

clean:
	$(RM) core errs* $(ALL) *.o *.list *.tmp

spotless: clean
	$(RM) junk* tmp* temp* *.BAK *.CKP .[a-z]*

dev:	gp1cg2.1152.$(PROC).o
dev+:	gp1+cg2.1152.$(PROC).o
