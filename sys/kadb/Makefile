#
# @(#)Makefile 1.1 94/10/31 SMI
#

ARCH:sh = arch -k

#
# All the real work gets done in ./$(ARCH), and that's where the targets
# wind up. This makefile just gets us there.
#
kadb all lint install install_h clean tags depend:
	cd $(ARCH); $(MAKE) $(MFLAGS) $@
