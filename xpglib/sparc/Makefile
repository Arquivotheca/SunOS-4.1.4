#
#	@(#)Makefile 1.1 94/10/31 SMI;
#

ASFLAGS += -P -I../../lib/libc/sys/common/sparc

ARCH-mc68020=	m68k
ARCH-sparc = sparc
ARCH= $(ARCH$(TARGET_MACH))


.KEEP_STATE:

sbrk.o: sbrk.s
	$(AS) $(ASFLAGS) $(CPPFLAGS) -o $(OBJDIR)/$@ $<
	
clean:
	@rm -f sbrk.o

