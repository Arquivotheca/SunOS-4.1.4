#
#	@(#)Makefile 1.1 94/10/31 SMI; from S5R2 1.9
#

INS	= install
INSDIR	= $(DESTDIR)/usr/lib
#
# Note: if the installed cpp doesn't know about -undef, do
#	rm cpp.o; make UNDEF="" ; make install
#	(/lib/cpp now knows about -undef, but may have bogus predefinitions)
#	rm cpp.o; make install
#	(/lib/cpp now knows about -undef, but predefines only the names below)
#

cpu-sun2 = m68k
cpu-sun3 = m68k
cpu-sun3x = m68k
cpu-sun4 = sparc
cpu-sun4d = sparc
cpu-sun386 = i386
CPU = $(cpu$(TARGET_ARCH))

# To turn off the code browser, remove the following line.
BROWSER = -DBROWSER
BROWSER_DIR = ../browser/cblib
BROWSER_BIN_DIR = $(BROWSER_DIR)/$(CPU)
BROWSER_SRC_DIR = $(BROWSER_DIR)/src

BROWSER_LIB-DBROWSER = $(BROWSER_BIN_DIR)/libcb.a
BROWSER_LIBRARY = $(BROWSER_LIB$(BROWSER))
BROWSER_INCL-DBROWSER = -I$(BROWSER_SRC_DIR)
BROWSER_FLAGS = $(BROWSER) $(BROWSER_INCL$(BROWSER))


YACC	= /usr/bin/yacc
VROOT	= ../vroot
VROOTLIB = $(VROOT)/vroot.a
CPPFLAGS = -DSUNPRO -DFLEXNAMES -I$(VROOT) $(BROWSER_FLAGS)
COPTS	= -O
DBX	=		# Use 'make DBX=-g' for debuggable version
CFLAGS	= $(COPTS) $(CPPFLAGS) $(DBX)
UNDEF   = -Qoption cpp -undef
SUNDEFS	= -Dunix -Dsun
SRCS	= cpy.c cpp.c
HDRS	= $(VROOT)/vroot.h $(VROOT)/report.h
# TARGET is the same as the host, by default, but may be overridden in the
# "make" command line for a cross-preprocessor.
TARGET:sh	= arch

.DEFAULT:
	sccs get -G$@ $@

cpp :	cpp.o cpy.o rodata.o yylex.o $(VROOTLIB)
	$(CC) $(LDFLAGS) $(DBX) -o cpp cpp.o rodata.o cpy.o yylex.o $(VROOTLIB)

cpp.o cpy.o: $(HDRS)

y.tab.h cpy.c : cpy.y yyfix.sh
	$(YACC) -d cpy.y
	/bin/sh ./yyfix.sh yyexca yyact yypact yypgo yyr1 yyr2 yychk yydef
	mv y.tab.c cpy.c

cpp.o : cpp.c $(BROWSER_LIBRARY)
	@set -x; case ${TARGET} in \
	sun2) \
	    $(CC) $(CFLAGS) $(UNDEF) $(SUNDEFS) \
		-Dmc68000 -Dmc68010 -Umc68020 -Usparc -Ui386 -c cpp.c;;\
	sun3 | sun3? ) \
	    $(CC) $(CFLAGS) $(UNDEF) $(SUNDEFS) \
		-Dmc68000 -Umc68010 -Dmc68020 -Usparc -Ui386 -c cpp.c;;\
	sun4*) \
	    $(CC) $(CFLAGS) $(UNDEF) $(SUNDEFS) \
		-Umc68000 -Umc68010 -Umc68020 -Dsparc -Ui386 -c cpp.c;;\
	sun386) \
	    $(CC) $(CFLAGS) $(UNDEF) $(SUNDEFS) \
		-Umc68000 -Umc68010 -Umc68020 -Usparc -Di386 -c cpp.c;;\
	* ) \
	    echo 'Unknown machine???'; exit 2;;\
	esac

rodata.o : cpy.c
	$(CC) $(CFLAGS) -R -c rodata.c

$(HDRS) $(VROOTLIB): FRC
	cd $(@D); $(MAKE) $(@F)

$(BROWSER_BIN_DIR)/libcb.a: FRC
	cd $(@D); $(MAKE) $(@F)

FRC:

clean :
	rm -f *.s *.o
	rm -f cpy.c rodata.c cpp

install : cpp
	$(INS) -s cpp $(INSDIR)

depend: $(SRCS) $(HDRS) yylex.c
	rm -f makedep
	for i in $(SRCS) yylex.c; do \
	    ( $(CC) -M $(CPPFLAGS) $$i >> makedep ); \
	done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile
	rm -f eddep makedep

# DO NOT DELETE THIS LINE -- make depend uses it

cpy.o: cpy.c
cpp.o: cpp.c
cpp.o: /usr/include/stdio.h
cpp.o: /usr/include/ctype.h
cpp.o: /usr/include/sys/file.h
cpp.o: /usr/include/sys/fcntlcom.h
cpp.o: ../vroot/vroot.h
cpp.o: /usr/include/stdio.h
cpp.o: ../vroot/report.h
cpp.o: /usr/include/stdio.h
yylex.o: yylex.c
yylex.o: ./y.tab.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
