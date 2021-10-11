#
# @(#)Makefile 1.1 94/10/31 SMI; from UCB 4.1 83/03/30
#
CFLAGS=-O
DESTDIR=

graph: graph.c
	${CC} $(CFLAGS) -o graph graph.c -lplot -lm

install: graph
	install -s graph ${DESTDIR}/usr/bin/graph

clean:
	rm -f *.o graph
