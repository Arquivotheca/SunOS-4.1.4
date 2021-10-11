#ifndef lint
static	char sccsid[] = "@(#)cg_main.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

# define FORT
/* this forces larger trees, etc. */
# include "cpass2.h"

/* from cgflags.c */
extern int cgflags(/*argc, argv*/);
extern char *infilename;	/* cgflags sets this to 1st non '-' arg */
extern char *outfilename;	/* cgflags sets this to 2nd non '-' arg */

/* from do_ir_archive.c */
extern void do_ir_archive(/*fn*/);

main( argc, argv ) 
char *argv[]; 
{
    FILE *f;

    argc = cgflags( argc, argv );
    (void) p2init( argc, argv );
    tinit();
    intr_map_init();
    if (infilename == NULL) {
	fprintf(stderr,"cg: no ir input file");
	exit(1);
    }
    if (outfilename != NULL) {
	f = freopen(outfilename, "a", stdout);
	if (f == NULL) {
	    fprintf(stderr, "cannot open %s for output", outfilename);
	    exit(1);
	}
    }
    do_ir_archive(infilename);
#   if TARGET == MC68000
	if (fltused) {
	    floatnote();
	}
#   endif
    exit(nerrors);
}
