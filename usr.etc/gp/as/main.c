#ifndef lint
static	char sccsid[] = "@(#)main.c 1.1 94/10/31 SMI";
#endif

/*
 *	Micro-assembler -- main driver
 *		main.c	1.1	31 May 1984
 */

#include "micro.h"
#include <stdio.h>

char * curfilename = "<stdin>";
int curaddr;
int curlineno;
Boolean dflag = False;
Boolean waserror = False;
Boolean Scanning = True;
NODE n[NNODE];
NODE *curnode = n-1;

main(argc, argv)
    int argc;
    char **argv;
{
    int i;
    FILE *out_stream;
    if (argc == 2) {
	if ((out_stream = fopen(argv[1], "w")) == NULL) {
	    fatal("could not open file %s",argv[1]);
	}
    } else {
	out_stream = 0;
    }
    curaddr = 0;
    init_symtab();
    resw_hash(); /* hash keywords */
    init_assm();
    scanprog();
    Scanning = False;
    if (waserror) exit(1);
    resolve_addrs();
    output(out_stream);
    if (waserror) exit(1);
    exit(0);
    /* NOTREACHED */
}

fatal(s)
    char *s;
{
    printf("File %s, line %d: %s\n", curfilename, curlineno, s);
    _cleanup();
    abort();
}

error(s, a, b, c, d, e, f)
    char *s;
{
    if (Scanning)
	printf("File %s, line %d: ", curfilename, curlineno);
    printf(s, a, b, c, d, e, f);
    printf("\n");
    waserror = True;
}

warn(s, a, b, c, d, e, f)
    char *s;
{
    printf("Warning:");
    if (Scanning)
	printf(" file %s, line %d: ", curfilename, curlineno);
    printf(s, a, b, c, d, e, f);
    printf("\n");
}
