#	@(#)Makefile 1.1 94/10/31 SMI
all:
	sh makewidths
	-for i in *.c; do \
		/bin/cc -c $$i; \
		strip `basename $$i .c`.o; \
	done
	>all

install: all
	-for i in *.o; do \
	    install -c -m 755 $$i ${DESTDIR}/usr/lib/font/`basename $$i .o`; \
	done

clean:
	rm -f all
