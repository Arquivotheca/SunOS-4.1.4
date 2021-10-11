#
# @(#)Makefile	1.1 (Sun) 10/31/94
# compiler bootstrap and fixed-point test
# Step 0 of a crank turn
#
RELEASEBIN = /usr/release/bin
SRCDIR	= /usr/src
LANGSRC	= $(SRCDIR)/lang
LANGUCB = $(SRCDIR)/ucblib
MAKE	= make ${MFLAGS} CPU=$(CPU)
MAKE_ALT= make ${MFLAGS} CPU=$(CPU) \
	YACC="$(alt)/yacc -P$(alt)/yaccpar" \
	CC="$(CC)" AS="$(alt)/as" LD="$(alt)/ld "
CC	= $(alt)/cc -Qpath $(alt) 
bin	= $(LANGSRC)/bin
alt	= $(LANGSRC)/bin/$(CPU)
as	= $(LANGSRC)/as
c2	= $(LANGSRC)/c2
vroot	= $(LANGSRC)/vroot
compile	= $(LANGUCB)/compile
yaccdir = $(LANGSRC)/yacc
cpp	= $(LANGSRC)/cpp
iropt	= $(LANGSRC)/iropt
inline  = $(LANGSRC)/inline
ld	= $(LANGSRC)/ld
ld.so	= $(LANGSRC)/rtld
lint	= $(LANGSRC)/lint
pcc	= $(LANGSRC)/pcc
libc	= $(SRCDIR)/lib/libc
csu	= $(SRCDIR)/lib/csu/$(CPU)
libm	= $(SRCDIR)/usr.lib/libm
5lib_compile = $(LANGSRC)/compile
ld.path	= LD_LIBRARY_PATH=$(alt)

VERS-mc68010 = vers-m68k
VERS-mc68020 = vers-m68k
VERS-sparc = vers-sparc
CMDS = cc
INSTALLCMDS = /usr/ucb/cc /usr/bin/cc /usr/bin/lint
STEP1PROGS = \
	step1cc step1yacc step1cpp step1pcc step1lint step1c2 step1as step1ld \
	step1iropt step1inline
STEP3PROGS = \
	step3cc step3yacc step3cpp step3pcc step3lint step3c2 step3as step3ld \
	step3ldso step3iropt step3inline
STEP5PROGS = \
	step5cc step5yacc step5cpp step5pcc step5lint step5c2 step5as step5ld \
	step5ldso step5iropt step5inline

#
# goal: install stabilized compilers & libraries
#
all: finish
	${RELEASEBIN}/makeinstall

#
# Initialize for step 0
#
step0clean:
	cd $(libc) ; make clean > clean.out 2>&1 ; rm -f Make.out
	cd $(csu) ; make clean > clean.out 2>&1 ; rm -f Make.out
	rm -f sanity step*
	rm -rf $(bin)
	if test ! -d $(bin); then mkdir $(bin); fi
	if test ! -d $(alt); then mkdir $(alt); fi
	touch $@

#
# step 0: Make a library with the old compiler components and install it. 
#
step0libc: step0clean
	cd $(libc) ; $(MAKE) libc.a libc.so libc.sa >> Make.out 2>&1
	touch $@

step0csu: step0libc
	cd $(csu) ; $(MAKE) >> Make.out 2>&1
	touch $@

step0: step0csu
	cd $(libc) ; \
	if [ -s $(VERS$(TARGET_MACH)) ]; then \
        	install libc.so $(alt)/libc.so.`cat $(VERS$(TARGET_MACH))`; \
        	install libc.sa $(alt)/libc.sa.`cat $(VERS$(TARGET_MACH))`; \
        	ranlib $(alt)/libc.sa.`cat $(VERS$(TARGET_MACH))`; \
	else echo "/usr/src/lang/boot/Makefile:WARNING shared libraries not installed"; \
        fi; \
	install libc.a $(alt)/libc.a ; ranlib $(alt)/libc.a; 
	cd $(csu) ; cp *[01].o $(alt)
	touch $@

#
# Initialize for step 1
#
step1clean: step0
	cd $(vroot) ; make clean
	cd $(compile) ; make clean
	cd $(yaccdir) ; make clean
	cd $(cpp) ; make clean
	cd $(pcc) ; make clean
	cd $(c2) ; make clean
	cd $(as) ; make clean
	cd $(ld) ; make clean
	cd $(ld.so) ; make clean
	cd $(iropt) ; make clean
	cd $(inline) ; make clean
	cd $(libm)   ; make clean
	cd $(5lib_compile) ; make clean
	touch $@

#
# step 1: make new components using existing components.
#
step1cc: step1clean
	cd $(compile) ; $(ld.path) $(MAKE)
	touch $@

step1yacc: step1clean
	cd $(yaccdir) ; $(ld.path) $(MAKE)
	touch $@

step1cpp: step1clean
	cd $(cpp) ; $(ld.path) $(MAKE)
	touch $@

step1pcc: step1clean
	cd $(pcc) ; $(ld.path) $(MAKE)
	touch $@

step1lint: step1clean
	cd $(lint) ; $(ld.path) $(MAKE)
	touch $@

step1c2: step1clean
	cd $(c2) ; $(ld.path) $(MAKE)
	touch $@

step1as: step1clean
	cd $(as) ; $(ld.path) $(MAKE)
	touch $@

step1ld: step1clean
	cd $(ld) ; $(ld.path) $(MAKE)
	touch $@

step1iropt: step1clean
	cd $(iropt) ; $(ld.path) $(MAKE)
	touch $@

step1inline: step1clean
	cd $(inline) ; $(ld.path) $(MAKE)
	touch $@

#
# Install the new compiler passes.
#

step1:	$(STEP1PROGS)
	cd $(compile) ; install compile $(alt)/compile ; \
	    for i in ${CMDS}; do \
		(rm -f $(alt)/$$i; ln -s compile $(alt)/$$i)\
	    done
	cd $(yaccdir) ; install yacc $(alt)/yacc
	cd $(yaccdir) ; install yaccpar $(alt)/yaccpar
	cd $(cpp) ; install cpp $(alt)/cpp
	cd $(pcc)/$(CPU)  ; install ccom $(alt)/ccom
	cd $(pcc)/$(CPU)  ; install cg $(alt)/cg
	case $(CPU) in \
	m68k) cd $(c2)/m68k ; install c2 $(alt)/c2 ;\
	esac
	cd $(as)/$(CPU)  ; install as $(alt)/as
	cd $(ld)/$(CPU)  ; install ld $(alt)/ld
	cd $(iropt)/$(CPU)  ; install iropt $(alt)/iropt
	cd $(inline)/$(CPU)  ; install inline $(alt)/inline
	touch $@

#
# Initialize for step 2
#
step2clean: step1
	cd $(libc) ; make clean > clean.out 2>&1 ; rm -f Make.out
	cd $(csu) ; make clean > clean.out 2>&1 ; rm -f Make.out
	touch $@

#
# step 2: Make a library with the new components and install it. 
#
step2libc: step2clean
	cd $(libc) ; $(MAKE_ALT) MAKE='$(MAKE_ALT)' \
		libc.a libc.so libc.sa >> Make.out 2>&1
	touch $@

step2csu: step2libc
	cd $(csu) ; $(MAKE_ALT) MAKE='$(MAKE_ALT)' >> Make.out 2>&1
	touch $@

step2: step2csu
	cd $(libc) ; \
	if [ -s $(VERS$(TARGET_MACH)) ]; then \
        	install libc.so $(alt)/libc.so.`cat $(VERS$(TARGET_MACH))`; \
        	install libc.sa $(alt)/libc.sa.`cat $(VERS$(TARGET_MACH))`; \
        	ranlib $(alt)/libc.sa.`cat $(VERS$(TARGET_MACH))`; \
	else echo "/usr/src/lang/boot/Makefile:WARNING shared libraries not installed"; \
        fi; \
	install libc.a $(alt)/libc.a ; ranlib $(alt)/libc.a; 
	cd $(csu) ; cp *[01].o $(alt)
	touch $@

#
# step 3: Make new compiler passes with the new compiler
# and link them with the new library.
#

step3clean: step2
	cd $(vroot) ; make clean
	cd $(compile) ; make clean
	cd $(yaccdir) ; make clean
	cd $(cpp) ; make clean
	cd $(pcc) ; make clean
	cd $(c2) ; make clean
	cd $(as) ; make clean
	cd $(ld) ; make clean
	cd $(iropt) ; make clean
	cd $(inline) ; make clean
	touch $@

step3cc: step3clean
	cd $(compile) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@

step3yacc: step3clean
	cd $(yaccdir) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@

step3cpp: step3clean
	cd $(cpp) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' 
	touch $@

step3pcc: step3clean
	cd $(pcc) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@

step3lint: step3clean
	cd $(lint) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@

step3c2: step3clean
	cd $(c2) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@

step3as: step3clean
	cd $(as) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@

step3ld: step3clean
	cd $(ld) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@

# ld.so needs to be compiled pic so it waits until new cc is installed
step3ldso: step3clean
	cd $(ld.so); $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@

step3iropt: step3clean
	cd $(iropt) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@

step3inline: step3clean
	cd $(inline) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@

#
# install new components...
#
step3:	$(STEP3PROGS)
	cd $(compile) ; install compile $(alt)/compile ; \
	    for i in ${CMDS}; do \
		(rm -f $(alt)/$$i; ln -s compile $(alt)/$$i)\
	    done
	cd $(yaccdir) ; install yacc $(alt)/yacc
	cd $(yaccdir) ; install yaccpar $(alt)/yaccpar
	cd $(cpp) ; install cpp $(alt)/cpp
	cd $(pcc)/$(CPU)  ; install ccom $(alt)/ccom
	cd $(pcc)/$(CPU)  ; install cg $(alt)/cg
	case $(CPU) in \
	m68k) cd $(c2)/m68k ; install c2 $(alt)/c2 ;\
	esac
	cd $(as)/$(CPU)  ; install as $(alt)/as
	cd $(ld)/$(CPU)  ; install ld $(alt)/ld
	cd $(ld.so)/$(CPU) ; install ld.so $(alt)/ld.so
	cd $(iropt)/$(CPU)  ; install iropt $(alt)/iropt
	cd $(inline)/$(CPU)  ; install inline $(alt)/inline
	touch $@

#
# At this point, we should have achieved a 'fixed point'.
# To test this, repeat steps 2 and 3 and compare results.
#
step4clean: step3
	-cd $(libc) ; \
	if [ -s $(VERS$(TARGET_MACH)) ]; then \
		mv libc.so libc.so.bak ; mv libc.sa libc.sa.bak ; \
	fi; \
	mv libc.a libc.a.bak ; make clean > clean.out 2>&1 ;
	-cd $(csu) ; make clean > clean.out 2>&1 ;
	touch $@

step4libc: step4clean
	cd $(libc) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' \
		libc.a libc.so libc.sa libbsdmalloc.a >> Make.out 2>&1
	touch $@

step4csu: step4libc
	cd $(csu) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' \
	    >> Make.out 2>&1
	touch $@

step4: step4csu
	cd $(libc) ; \
	if [ -s $(VERS$(TARGET_MACH))  ]; then \
        	install libc.so $(alt)/libc.so.`cat $(VERS$(TARGET_MACH))`; \
        	install libc.sa $(alt)/libc.sa.`cat $(VERS$(TARGET_MACH))`; \
        	ranlib $(alt)/libc.sa.`cat $(VERS$(TARGET_MACH))`; \
	else echo "/usr/src/lang/boot/Makefile:WARNING shared libraries not installed"; \
        fi; \
	install libc.a $(alt)/libc.a ; ranlib $(alt)/libc.a
	cd $(csu) ; cp *[01].o $(alt)
	touch $@

#
# ...rebuild the new components
#
step5clean: step4
	cd $(vroot) ; make clean
	cd $(compile) ; make clean
	cd $(yaccdir) ; make clean
	cd $(cpp) ; make clean
	cd $(pcc) ; make clean
	cd $(c2) ; make clean
	cd $(as) ; make clean
	cd $(ld) ; make clean
	cd $(ld.so) ; make clean
	cd $(iropt) ; make clean
	cd $(inline) ; make clean
	touch $@

step5cc: step5clean
	cd $(compile) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' 
	touch $@

step5yacc: step5clean
	cd $(yaccdir) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@

step5cpp: step5clean
	cd $(cpp) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' 
	touch $@

step5pcc: step5clean
	cd $(pcc) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@

step5lint: step5clean
	cd $(lint) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@

step5c2: step5clean
	cd $(c2) ; $(ld.path) $(MAKE_ALT)  MAKE='$(MAKE_ALT)'
	touch $@

step5as: step5clean
	cd $(as) ; $(ld.path) $(MAKE_ALT)  MAKE='$(MAKE_ALT)'
	touch $@

step5ld: step5clean
	cd $(ld) ; $(ld.path) $(MAKE_ALT)  MAKE='$(MAKE_ALT)'
	touch $@

step5ldso: step5clean
	cd $(ld.so); $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@

step5iropt: step5clean
	cd $(iropt) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@

step5inline: step5clean
	cd $(inline) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@

#
# ...and if the results differ, something is broken.
# note: the special case treatment for the assembler is necessary
# because it always gets timestamped
#
sanity: $(STEP5PROGS)
	cd $(compile) ; cmp compile $(alt)/compile
	cd $(yaccdir) ; cmp yacc $(alt)/yacc
	cd $(cpp) ; cmp cpp $(alt)/cpp
	cd $(pcc)/$(CPU) ; cmp ccom $(alt)/ccom
	cd $(pcc)/$(CPU) ; cmp cg $(alt)/cg
	case $(CPU) in \
	m68k)    cd $(c2)/m68k ; cmp c2 $(alt)/c2 ;\
	esac
	cd $(as)/$(CPU) ; cmp as $(alt)/as ;\
	cd $(ld)/$(CPU) ; cmp ld $(alt)/ld
	cd $(ld.so)/$(CPU) ; cmp ld.so $(alt)/ld.so
	cd $(iropt)/$(CPU) ; cmp iropt $(alt)/iropt
	cd $(inline)/$(CPU) ; cmp inline $(alt)/inline
	touch $@

#
#  This stuff isn't part of the C compiler, but needs to
#  be built before getting on with the rest of the crank-turn.
#
libc_p: sanity
	cd $(libc)   ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' libc_p.a
	touch $@
libcs5: sanity
	cd $(libc)   ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' libcs5.a libcs5.so libcs5_p.a libcs5.sa
	touch $@
_libm: sanity
	cd $(libm)   ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' fsoft
	touch $@
5compile: sanity
	cd $(5lib_compile)   ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)'
	touch $@
finish: libc_p libcs5 _libm 5compile
	touch $@


install: finish
	cd $(compile) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' install DESTDIR=$(DESTDIR)
	for i in ${INSTALLCMDS}; do \
	    (rm -f $(DESTDIR)/$$i; ln -s /usr/lib/compile $(DESTDIR)/$$i); \
	done
	cd $(yaccdir) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' install DESTDIR=$(DESTDIR)
	cd $(cpp) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' install DESTDIR=$(DESTDIR)
	cd $(pcc) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' install DESTDIR=$(DESTDIR)
	cd $(lint) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' install DESTDIR=$(DESTDIR)
	cd $(c2) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' install DESTDIR=$(DESTDIR)
	cd $(as) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' install DESTDIR=$(DESTDIR)
	cd $(ld) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' install DESTDIR=$(DESTDIR)
	cd $(ld.so) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' install DESTDIR=$(DESTDIR)
	cd $(iropt) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' install DESTDIR=$(DESTDIR)
	cd $(inline) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' install DESTDIR=$(DESTDIR)
	cd $(libc) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' install DESTDIR=$(DESTDIR)
	cd $(csu) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' install DESTDIR=$(DESTDIR)
	cd $(libm) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' installfsoft DESTDIR=$(DESTDIR)
	cd $(5lib_compile) ; $(ld.path) $(MAKE_ALT) MAKE='$(MAKE_ALT)' install DESTDIR=$(DESTDIR)
	/usr/etc/ldconfig

depend:
	@echo 'are you kidding?'

