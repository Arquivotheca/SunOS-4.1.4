#
# @(#)Makefile 1.1 94/10/31 SMI
#

# makefile for pp microcode

PROC=pp

ALL = $(UCODE) $(UCODE+)
UCODE = gp1cg2.1152.$(PROC).o gp1cg2.1024.$(PROC).o
UCODE+ = gp1+cg2.1152.$(PROC).o gp1+cg2.1024.$(PROC).o

INCS = pp.h macros.pp.h s1024.pp.h s1152.pp.h

MAIN = pp.u

SRCS = $(INCS) $(MAIN) \
	ack.pp.u \
	clipoff.pp.u \
	delay.pp.u \
	div.pp.u \
	fatvec.pp.u \
	gbuffer.pp.u \
	init.pp.u \
	m1024.pp.u \
	m1152.pp.u \
	mul.pp.u \
	octant.pp.u \
	offset.pp.u \
	polyline.pp.u \
	ropfb.pp.u \
	ropnull.pp.u \
	roptex.pp.u \
	setlineatts.pp.u \
	setpat.pp.u \
	shade.pp.u \
	squareroot.pp.u \
	statm1024.pp.u \
	statm1152.pp.u \
	statmul.pp.u \
	statpick.pp.u \
	statshade.pp.u \
	stclip.pp.u \
	texvec.pp.u \
	texvector.pp.u \
	vector.pp.u \
	xfline.pp.u \
	xflineinit.pp.u

SRCS+ =	$(SRCS) \
	mulchip.pp.u \
	pick.pp.u \
	statpick.pp.u \
	vec3.pp.u \
	zboctant.pp.u \
	dcoctant.pp.u \
	dctexvec.pp.u \
	zbtexvec.pp.u \
	pkzboctant.pp.u \
	pickline3.pp.u \
	pkzbtexvec.pp.u \
	pkndzbtexvec.pp.u \
	pkndzboctant.pp.u
			
DEBUG =	shmemdebug.pp.u	

CPP = /lib/cpp
UAS = $(PROC)as
RM = rm -f

.DEFAULT: ; $(GET) -G$@ $(GFLAGS) SCCS/s.$<

all: $(ALL)

gp1cg2.1152.$(PROC).o: $(SRCS)
	$(RM) $@ $(@:.o=.list)
	$(CPP) -DSWIDTH=1152 $(MAIN) | $(UAS) $@ > $(@:.o=.list)

gp1cg2.1024.$(PROC).o: $(SRCS)
	$(RM) $@ $(@:.o=.list)
	$(CPP) -DSWIDTH=1024 $(MAIN) | $(UAS) $@ > $(@:.o=.list)

gp1+cg2.1152.$(PROC).o: $(SRCS+) $(DEBUG)
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
