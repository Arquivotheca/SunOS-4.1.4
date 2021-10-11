#ifndef lint
static	char sccsid[] = "@(#)makedefs.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

/*
 * Create a definitions file (e.g. .h) from an implementation file (e.g. .c).
 *
 * Usage is "makedefs source.c source.h" where source.h is to be created.
 *
 * Lines beginning with "public" or within a "#ifndef public ... #endif"
 * block are copied to the new file.  Initializations (e.g. "int x = 3") are
 * omitted ("int x ;" is output).
 *
 * Normally a temporary definitions file is created and compared to
 * the given destination.  If they are different, the temporary file
 * is copied on top of the destination.  This is so that dependencies
 * when using "make" are not triggered.
 *
 * The "-f" option overrides this and forces the destination file to be created.
 */

#include "defs.h"
#include <signal.h>

#define prefixeq(s1, s2, len) (strncmp(s1, s2, len) == 0)

Boolean force;
Boolean copytext;

struct sigvec sigv;

String tmpname;
String modulename();
void abnorm();
Boolean istool();

main(argc, argv)
int argc;
String argv[];
{
    extern String mktemp();
    char hdrname[100];
    char cmd[512];
    String name;
    File tmp;
    Integer r;
    Integer ix;

    if (streq(argv[1], "-f")) {
	force = true;
	ix = 2;
    } else {
	force = false;
	ix = 1;
    }
    if (argc - ix > 2) {
	fatal("usage: makedefs [ -f ] file.c [ file.h ]\n");
    }
    tmp = nil;
    if (freopen(argv[ix], "r", stdin) == NULL) {
	fatal("can't read %s", argv[ix]);
    }
    sigv.sv_handler = abnorm;
    sigv.sv_mask = 0;
    sigv.sv_onstack = 0;
    sigvec(SIGINT, &sigv, 0);
    sigvec(SIGQUIT, &sigv, 0);
    if (ix + 1 < argc) {
	if (force) {
	    tmpname = argv[ix + 1];
	} else {
	    tmpname = mktemp("/tmp/makedefsXXXXXX");
	}
	tmp = freopen(tmpname, "w", stdout);
	if (tmp == nil) {
	    fatal("can't write %s", tmpname);
	}
    }
    copytext = false;
    name = modulename(argv[ix]);
    printf("#ifndef %s\n", name);
    printf("#define %s\n", name);
    copy();
    printf("#endif\n");
    if (tmp != NULL and not force) {
	fclose(tmp);
	sprintf(cmd, "cmp -s %s %s", tmpname, argv[2]);
	r = system(cmd);
	if (r != 0) {
	    sprintf(cmd, "cp %s %s", tmpname, argv[2]);
	    r = system(cmd);
	    if (r != 0) {
		fprintf(stderr, "can't create %s\n", argv[2]);
	    }
	}
	unlink(tmpname);
    }
    strcpy(hdrname, name);
    hdrname[strlen(hdrname) - 2] = '\0';
    strcat(hdrname, ".hdr");
    unlink(hdrname);
    close(creat(hdrname, 0664));
    quit(0);
}

String modulename(s)
String s;
{
    String i, j;
    static char buf[256];

    strcpy(buf, s);
    i = rindex(buf, '/');
    if (i == nil) {
	i = buf;
    } else {
	++i;	/* skip the slash */
    }
    for (j = i; *j != '.'; j++);
    *j++ = '_';
    *j++ = 'h';
    *j = '\0';
    return i;
}

copy()
{
    register char *p;
    register int ifdeflevel;
    char line[1024];

    ifdeflevel = 0;
    while (gets(line) != NULL) {
	if (prefixeq(line, "#ifndef public", 14)) {
	    copytext = true;
	    ifdeflevel = 0;
	} else if (copytext and ifdeflevel == 0 and
	  prefixeq(line, "#endif", 6)) {
	    copytext = false;
	} else if (prefixeq(line, "public", 6)) {
	    copydef(line);
	} else if (copytext) {
	    printf("%s\n", line);
	    if (prefixeq(line, "#ifdef", 6)) {
		++ifdeflevel;
	    } else if (prefixeq(line, "#endif", 6)) {
		--ifdeflevel;
	    }
	} else if (line[0] == '#') {
	    p = &line[1];
	    while (*p == ' ' or *p == '\t') {
		++p;
	    }
	    if (prefixeq(p, "if", 2) or prefixeq(p, "else", 4) or
	      prefixeq(p, "endif", 5)) {
		printf("%s\n", line);
	    }
	}
    }
}

copydef(s)
String s;
{
    register char *p;
    register Boolean isproc;
    int	leftparen;
    int	procparen;

    leftparen = 0;
    isproc = false;
    for (p = &s[7]; *p != '\0' and *p != '='; p++) {
	if (*p == '(') {
	    if (p[1] != '*' and not isproc) {
	        isproc = true;
		procparen = leftparen++;
	        printf("(/* ");
	    } else {
		leftparen++;
		putchar(*p);
	    }
	} else if (*p == ')') {
	    leftparen--;
	    if (isproc and leftparen == procparen) {
		printf(" */)");
	    } else {
		putchar(*p);
	    }
	} else {
	    putchar(*p);
	}
    }
    if (isproc or *p == '=') {
	putchar(';');
    }
    putchar('\n');
}

/*
 * Terminate program.
 */

void abnorm(signo)
int signo;
{
    unlink(tmpname);
    quit(signo);
}

quit(r)
int r;
{
    exit(r);
}
/*
 * No special error recovery strategy.
 */
erecover() { }

Boolean istool()
{
	return(false);
}
