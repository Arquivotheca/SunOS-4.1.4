#ifndef lint
static	char sccsid[] = "@(#)cgflags.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
/*	
 *	Modified by:	R. Schell
 *	Date:			7/6/87
 *	Modificiation:	Added debug flag options to cgflags (lflag, edebug,
 *					sdebug, odebug).  Adding edebug requires defining
 *					edebug to be e2debug here (cpass2.h does that, but
 *					only if ONEPASS is defined.)
 */
# define FORT
/* this forces larger trees, etc. */
#if (TARGET==I386)
#define edebug e2debug
#endif
# include "cpass2.h"


/* from pcc.c */
extern int stmtprofflag;	/* =1 -> enable statement profiling */
extern FILE *dotd_fp;		/* output file for statement profiling */
extern char *dotd_filename;	/* name of statement profiling output file */
				/* ... compiled into object code! (bletch) */
extern char *dotd_basename;	/* 'basename' of dotd_filename */
extern int dotd_baselen;
char *infilename = NULL;
int ir_debug = 0;		/* =1 -> enable IR debugging output */
char *outfilename = NULL;

extern char *rindex();

int
cgflags(argc, argv)
    char *argv[];
{
    int i;
    char *s;
    char **argp;
    int myargs;
#if TARGET == I386
    int j;
#endif

    /*
     * scan through argv[] for cgrdr-specific options.
     * If you recognize something, remove it, because
     * nobody else is going to want it.
     */
    infilename = NULL;
    myargs = 0;
    argp = argv+1;
    for (i = 1; i < argc; i++) {
	s = argv[i];
	if (s[0] == '-') {
	    switch(s[1]) {
	    case 'I':
		ir_debug++;
		myargs++;
		continue;
	    case 'A':
		stmtprofflag = 1;
		dotd_filename = s+2;
		if((dotd_fp = fopen(dotd_filename, "w")) == NULL){
		    fprintf(stderr, "cg: can't open stmt profiling file '%s'",
			dotd_filename);
		    exit(1);
		}
		dotd_basename = rindex(dotd_filename, '/');
		if (dotd_basename == NULL)
			dotd_basename = dotd_filename;
		else
			dotd_basename++;
		dotd_baselen = strlen(dotd_basename);
		myargs++;
		continue;
#if (TARGET==I386)
	    case 'X':
		for (j = 2;;j++){
		    switch(s[j]){
		    case 'o':
			odebug++;
			continue;
		    case 's':
			sdebug++;
			continue;
		    case 'e':
			edebug++;
			continue;
		    case 'l':
			lflag++;
			continue;
		    default:
			break;
		    }
		    break;
		}
		myargs++;
		continue;
#endif
	    }
	} else if (infilename == NULL) {
	    infilename = s;
	    myargs++;
	    continue;
	} else if (outfilename == NULL) {
	    outfilename = s;
	    myargs++;
	    continue;
	}
	/*
	 * if here, we didn't recognize it;
	 * pass it along to p2init()
	 */
	*argp++ = s;
    }
    return (argc - myargs);
}
