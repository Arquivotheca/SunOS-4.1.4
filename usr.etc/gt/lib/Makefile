#
# @(#)Makefile 1.1 94/10/31 SMI
#

# makefile for gtconfig library
DEST = .

CFLAGS = -g
CPPFLAGS =  -I. -I.. -I../include -I../../../sys/sbusdev -I../../../sys/sun
HDRS = 
CSRC =  util.c coff.c a.out.c gtio.c 
OBJS = $(CSRC:%.c=%.o)
LIB = libgt.a

coff.o  := CPPFLAGS += -I../include

${LIB}:  ${OBJS}
	 rm -f ${LIB}
	 ar rv ${LIB} ${OBJS}
	 ranlib ${LIB}

clean:
	 rm -f ${LIB} ${OBJS}

coff.o: ../include/filehdr.h
coff.o: ../include/n10aouth.h
coff.o: ../include/scnhdr.h
coff.o: ../include/reloc.h
coff.o: ../include/linenum.h
coff.o: ../include/storclass.h
coff.o: ../include/syms.h
