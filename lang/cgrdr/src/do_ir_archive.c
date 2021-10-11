#ifndef lint
static	char sccsid[] = "@(#)do_ir_archive.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "cg_ir.h"
#include "opdescr.h"
#include <stdio.h>
#include <sys/file.h>

/* export */
char *ir_file; 
void do_ir_archive(/*fn*/);
HEADER hdr;
LIST *leaf_hash_tab[LEAF_HASH_SIZE];

/* from read_ir.c */
extern BOOLEAN read_ir();

extern int stmtprofflag;	/* from pcc.c */
extern int ir_debug;		/* from cgflags.c */

extern void dump_segments();
extern void dump_blocks();
extern void dump_leaves();
extern void dump_triples();

void
do_ir_archive(fn)
	char *fn;
{
	int irfd;

	ir_file = fn;
	irfd = open(ir_file,O_RDONLY,0);
	if (irfd == -1) {
		perror(ir_file);
		quit("can't open ir file");
	}
	while( read(irfd, (char*)&hdr, sizeof(HEADER)) == sizeof(HEADER) ) {
		free_ir_alloc_list();
		if(read_ir(irfd) == IR_TRUE) {
			if(ir_debug) {
				dump_segments();
				dump_blocks();
				dump_leaves();
				dump_triples();
			}
			map_to_pcc();
		}
	}
	if(stmtprofflag) {
		stmtprof_eof();
	}
}
