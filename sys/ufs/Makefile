#
# @(#)Makefile 1.1 94/10/31 SMI
#
HFILES = fs.h fsdir.h inode.h mount.h quota.h lockf.h lockfs.h

HDIR=$(DESTDIR)/usr/include/ufs

install_h: $(HFILES)
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
