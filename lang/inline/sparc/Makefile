#
# @(#)Makefile	1.1 (Sun) 10/31/94 18:01:08
#

TARGET=	SPARC
SRC =	../src

DBX =
CPP =	/lib/cpp	# 'make depend' assumes -M feature of version 3.2!
CPPFLAGS = -DTARGET=$(TARGET) -DMC68000=1 -DSPARC=2 -I/usr/ucbinclude
CFLAGS=	-O $(DBX) $(CPPFLAGS)
DESTDIR=

.DEFAULT:
	sccs get -G$@ $@

SRCS =	$(SRC)/inline.c

inline: inline.o
	${CC} ${CFLAGS} inline.o -o $@

inline.o: $(SRC)/inline.c
	${CC} -c ${CFLAGS} $(SRC)/inline.c

lint:
	lint $(CPPFLAGS) $(SRCS)

clean:
	rm -f *.o inline

install: inline
	install -s -m 555 inline $(DESTDIR)/usr/lib/inline

depend: $(SRCS)
	rm -f makedep
	for i in $(SRCS); do \
	    ( $(CPP) -M $(CPPFLAGS) $$i >> makedep ); \
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

inline.o: ../src/inline.c
inline.o: /usr/include/stdio.h
inline.o: /usr/include/ctype.h
inline.o: /usr/include/strings.h
inline.o: /usr/include/sys/types.h
inline.o: /usr/include/sys/sysmacros.h
inline.o: /usr/include/sys/stat.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
