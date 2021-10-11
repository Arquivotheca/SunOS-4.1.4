#ifndef lint
static	char sccsid[] = "@(#)mappings.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Mappings from source to object and vice versa.
 */

#include "defs.h"
#include "mappings.h"
#include "symbols.h"
#include "source.h"
#include "object.h"
#include "machine.h"

#ifndef public
#include "machine.h"
#include "source.h"
#include "symbols.h"

public Address exit_addr;

#define	NUM_FUNC_INCR		500	/* Increment for functab[] */
public number_of_functions;		/* Default is 500 */

typedef struct {
    Address addr;
    String filename;
    String modname;				/* module to which the source file belongs */
    Lineno lineindex;			/* Index to first linetab entry */
} Filetab;

typedef struct {
    Lineno line;
    Address addr;
} Linetab;

Filetab *filetab;
Linetab *linetab;

public Name sigtramp_Name = (Name) nil;
public Address sigtramp_begin, sigtramp_end;

public int offset_findline;	/* if findline not exact, by how much? */

#define in_caught_sig_rtn(pc) (pc > sigtramp_begin and pc <= sigtramp_end)

#define NOADDR ((Address) -1)	/* no address for line or procedure */

#endif

/*
 * Get the source file name associated with a given address.
 */

public String srcfilename(addr)
Address addr;
{
    register Address i, j, k;
    Address a;
    Filetab *ftp;
    String s;

    s = nil;
    if (nlhdr.nfiles != 0 and addr >= filetab[0].addr) {
	i = 0;
	j = nlhdr.nfiles - 1;
	while (i < j) {
	    k = (i + j) / 2;
	    ftp = &filetab[k];
	    a = ftp->addr;
	    if (a == addr) {
		s = ftp->filename;
		break;
	    } else if (addr > a) {
		i = k + 1;
	    } else {
		j = k - 1;
	    }
	}
	if (s == nil) {
	    if (addr >= filetab[i].addr) {
		s = filetab[i].filename;
	    } else {
		s = filetab[i-1].filename;
	    }
	}
    }
    return s;
}

/*
 * Find the line associated with the given address.
 * If the second parameter is true, then the address must match
 * a source line exactly.  Otherwise the nearest source line
 * below the given address is returned.  In any case, if no suitable
 * line exists, 0 is returned.
 */

private Lineno findline(addr, exact)
Address addr;
Boolean exact;
{
    register Address i, j, k;
    register Lineno r;
    register Address a;

    offset_findline = 0;
    if (linetab == nil) {
	r = 0;
    } else if (nlhdr.nlines == 0 or addr < linetab[0].addr) {
	r = 0;
    } else {
	i = 0;
	j = nlhdr.nlines - 1;
	if (addr == linetab[i].addr) {
	    r = linetab[i].line;
	} else if (addr == linetab[j].addr) {
	    r = linetab[j].line;
	} else if (addr > linetab[j].addr) {
	    r = exact ? 0 : linetab[j].line;
	} else {
	    do {
		k = (i + j) div 2;
		a = linetab[k].addr;
	    if (a == addr) break;
		if (addr > a) {
		    i = k + 1;
		} else {
		    j = k - 1;
		}
	    } while (i <= j);

	    if (a == addr) {
		r = linetab[k].line;
	    } else if (exact) {
		r = 0;
	    } else if (addr > linetab[i].addr) {
		r = linetab[i].line;
		offset_findline = addr - linetab[i].addr;
	    } else {
		r = linetab[i-1].line;
		offset_findline = addr - linetab[i-1].addr;
	    }
	}
    }
    return r;
}

/*
 * Lookup the source line number nearest from below to an address.
 */

public Lineno srcline(addr)
Address addr;
{
    return findline(addr, false);
}

/*
 * Look for a line exactly corresponding to the given address.
 */

public Lineno linelookup(addr)
Address addr;
{
    return findline(addr, true);
}

/*
 * Lookup the address of the next source line from the given address.
 * (We know the address is not at a source line.)
 */

public Address srcaddr(addr)
Address addr;
{
    register Address i, j, k;
    register Address r;
    register Address a;

    i = 0;
    j = nlhdr.nlines - 1;
    if (linetab == nil) {
	r = addr;
    } else if (nlhdr.nlines == 0 or addr > linetab[j].addr) {
	r = addr;
    } else {
	if (addr < linetab[i].addr) {
	    r = linetab[i].addr;
	} else {
	    do {
		k = (i + j) div 2;
		a = linetab[k].addr;
		if (addr > a) {
		    i = k + 1;
		} else {
		    j = k - 1;
		}
	    } while (i <= j);
	    if (addr > linetab[i].addr) {
		r = linetab[i+1].addr;
	    } else {
		r = linetab[i].addr;
	    }
	}
    }
    return r;
}

/*
 * Lookup the object address of a given line from the named file.
 *
 * Potentially all files in the file table need to be checked
 * until the line is found since a particular file name may appear
 * more than once in the file table (caused by includes).
 *
 * If getnextline is true, return a line even if it isn't a valid
 * source line for stopping/tracing by returning the next line
 * within the same routine (if it exists).
 */

public Address objaddr(line, name, getnextline)
Lineno line;
String name;
Boolean getnextline;
{
    register Filetab *ftp;
    register Lineno i, j;
    Boolean foundfile;
	char buf1[1024], buf2[1024];
    char *listname;

    if (nlhdr.nlines == 0) {
	return NOADDR;
    }
    if (name == nil) {
	name = cursource;
    }

    if (line <= 0) line = 1;  /* fix: stop at 0, cont at 0, trace 0, etc */
    foundfile = false;
	name=findsource(name, buf1);
    for (ftp = &filetab[0]; ftp < &filetab[nlhdr.nfiles]; ftp++) {
	listname = findsource(ftp->filename, buf2);
	if (name && listname && !strcmp(name, listname)) {
	    foundfile = true;
	    i = ftp->lineindex;
	    if (ftp == &filetab[nlhdr.nfiles-1]) {
		j = nlhdr.nlines;
	    } else {
		j = (ftp + 1)->lineindex;
	    }
	    while (i < j) {
		if (linetab[i].line == line) {
		    return linetab[i].addr;
		}
		i++;
	    }
	    if (getnextline) {
		int k, highest;

		i = ftp->lineindex;
		highest = -1;
		    /* Find highest line in table */
		    /* fix 1013647, some compilers put out bogus line 65535, */
		    /* so reject it. */
		for (k = i; k < j; k++) {
		    if (linetab[k].line > highest && 
		    		linetab[k].line != 65535) {
			highest = linetab[k].line;
		    }
		}
		if (line > highest) {
		    /*
		     * There is no next line.  But maybe there's another
		     * "copy" of the file entry.  Keep looking.
		     */
		    continue;
		}
		highest = 0x7fffffff;
		k = 0;
		while (i < j) {
			if ((line < linetab[i].line) &&
			    ((linetab[i].line - line) < highest)) {
				k = i;
				highest = linetab[i].line - line;
			}
		    i++;
		}
		return linetab[k].addr;
	    }
	}
    }
    for (ftp = &filetab[0]; ftp < &filetab[nlhdr.nfiles]; ftp++) {
	if (same_file(name, findsource(ftp->filename, buf2))) {
	    foundfile = true;
	    i = ftp->lineindex;
	    if (ftp == &filetab[nlhdr.nfiles-1]) {
		j = nlhdr.nlines;
	    } else {
		j = (ftp + 1)->lineindex;
	    }
	    while (i < j) {
		if (linetab[i].line == line) {
		    return linetab[i].addr;
		}
		i++;
	    }
	    if (getnextline) {
		int k, highest;

		i = ftp->lineindex;
		highest = -1;
		    /* Find highest line in table */
		    /* fix 1013647, some compilers put out bogus line 65535, */
		    /* so reject it. */
		for (k = i; k < j; k++) {
		    if (linetab[k].line > highest && 
		    		linetab[k].line != 65535) {
			highest = linetab[k].line;
		    }
		}
		if (line > highest) {
		    /*
		     * There is no next line.  But maybe there's another
		     * "copy" of the file entry.  Keep looking.
		     */
		    continue;
		}
		highest = 0x7fffffff;
		k = 0;
		while (i < j) {
			if ((line < linetab[i].line) &&
			    ((linetab[i].line - line) < highest)) {
				k = i;
				highest = linetab[i].line - line;
			}
		    i++;
		}
		return linetab[k].addr;
	    }
	}
    }
    if (not foundfile) {
	if( name && name[0] == '.'  &&  name[1] == '/' ) {
	    /* Avoid dbxtool misfeature:  skip the "./" and recur */
	    return   objaddr(line, name +2, getnextline);
	}
	error("file \"%s\" was not compiled with the \"-g\" option", name);
    }
    return NOADDR;
}

/*
 * Table for going from object addresses to the functions in which they belong.
 */

typedef struct AddrOfFunc {
    Symbol func;
    Address addr;
} *AddrOfFunc;

private AddrOfFunc *functab;		/* Table of pointers */
private int nfuncs;
private	AddrOfFunc get_newfunc();

/*
 * Insert a new function into the table.
 * The table is ordered by object address.
 */

public newfunc(f, addr)
Symbol f;
Address addr;
{
    register AddrOfFunc af;

    if (nfuncs >= number_of_functions) {
	grow_functab();
    }
    af = get_newfunc();
    af->func = f;
    af->addr = addr;
    functab[nfuncs] = af;
    ++nfuncs;
}

#ifndef CONSTANT_FCN_PROLOG
public void find_each_fcn_begin ( )
{
    register int i;

    for (i = 0; i < nfuncs; i++) {
	findbeginning( functab[i]->func );
    }
}
#endif CONSTANT_FCN_PROLOG

/*
 * Return the function that begins at the given address.  If skipInline
 * is true, return the outermost function block (the function itself);
 * if not, return an inline block if it exists.
 */

public Symbol whatblock(addr, skipInline)
Address addr;
Boolean skipInline;
{
    register int i, j, k;
    register Symbol func;
    Address a;

    if (nfuncs == 0) {
	return(nil);
    }
    i = 0;
    j = nfuncs - 1;
    func = nil;
    if ((addr < functab[i]->addr) or (addr == codestart)) {
	func = program;
    } else if (addr == functab[i]->addr) {
	func = functab[i]->func;
    } else if (addr >= functab[j]->addr) {
	func = functab[j]->func;
    } else while (i <= j) {
	k = (i + j) / 2;
	a = functab[k]->addr;
	if (a == addr) {
	    func = functab[k]->func;
	    break;
	} else if (addr > a) {
	    i = k+1;
	} else {
	    j = k-1;
	}
    }
    if (func == nil) {
        if (addr > functab[i]->addr) {
	    func = functab[i]->func;
        } else {
	    func = functab[i-1]->func;
        }
    }
    if (skipInline) {
	skipInlineBlocks(func);
    }
    return func;
}

/*
 * Return the name of the function that begins at the given
 * address or return nil if no function starts at that address.
 */

public String srcprocname(addr)
Address addr;
{
    register int i, j, k;
    register Symbol func;
    Address a;

    if (nfuncs == 0) {
	return(nil);
    }
    i = 0;
    j = nfuncs - 1;
    func = nil;
    if (addr == codestart) {
	func = program;
    } else if ((addr < functab[i]->addr) or (addr > functab[j]->addr)) {
	func = nil;
    } else if (addr == functab[i]->addr) {
	func = functab[i]->func;
    } else if (addr == functab[j]->addr) {
	func = functab[j]->func;
    } else while (i <= j) {
	k = (i + j) / 2;
	a = functab[k]->addr;
	if (a == addr) {
	    func = functab[k]->func;
	    break;
	} else if (addr > a) {
	    i = k+1;
	} else {
	    j = k-1;
	}
    }
    if (func != nil) {
	return(symname(func));
    } else {
	return nil;
    }
}

/*
 * Return the address of the beginning of the given function as it is
 * represented by functab[], rather than in s->symvalue.funcv.beginaddr.
 * Only functab[] has the real begin address that can map to the source
 * line for some arches (at least the sun386).
 */
public Address firstaddrproc(s)
Symbol s;
{
    register int i;
    register Address addr;

    addr = NOADDR;
    for (i = 0; i < nfuncs; i++) {
	if (functab[i]->func == s) {
	    addr = functab[i]->addr;
	    break;
	}
    }
    return addr;
}

/*
 * Return the address of the beginning of the next function beyond
 * that represented by s (will not be at a source line)
 */

public Address lastaddrproc(s)
Symbol s;
{
    register int i, j;
    register Address addr;

    addr = NOADDR;
    for (i = 0; i < nfuncs; i++) {
	if (functab[i]->func == s) {
	    for (j = i+1; j <= nfuncs; j++) {
	    	if (j == nfuncs) {
		    addr = objsize + codestart;
		    break;
	    	} else if ((functab[j]->func != s) and
		   (functab[j]->func->symvalue.funcv.inline == false)) {
		    addr = functab[j]->addr;
		    break;
	    	}
	    }
	    break;
	}
    }
    return addr;
}

/*
 * Order the functab.
 */

private int cmpfunc(f1, f2)
AddrOfFunc *f1, *f2;
{
    register Address a1, a2;

    a1 = (*f1)->addr;
    a2 = (*f2)->addr;
    return ( (a1 < a2) ? -1 : ( (a1 == a2) ? 0 : 1 ) );
}

public ordfunctab()
{
    qsort(functab, nfuncs, sizeof(AddrOfFunc), cmpfunc);
}

/*
 * Clear out the functab, used when re-reading the object information.
 */

public clrfunctab()
{
    nfuncs = 0;
}

public dumpfunctab( verbose, gonly )
Boolean verbose;
Boolean gonly;
{
    int i;

    for (i = 0; i < nfuncs; i++) {
	if( gonly  and  nosource(functab[i]->func) )
	    continue;
	psym( functab[i]->func, verbose );
    }
}

/*
 * Find the upper and lower address bounds of the _sigtramp() library
 * routine for later use by nextframe()
 */

public set_sigtramp_bounds()
{
    register Symbol s;

    s = lookup(sigtramp_Name);
    if (s != nil) {
    	sigtramp_begin = rcodeloc(s);
    	sigtramp_end = lastaddrproc(s);
    } else {
	sigtramp_begin = sigtramp_end = 0;
    }
}

/*
 * Return the address of  _exit routine
 */

public Address get_exit_addr()
{
    register int i;

    for (i = 0; i < nfuncs; i++) {
	if (strcmp(symname(functab[i]->func), "_exit") == 0) {
	    exit_addr = functab[i]->addr;
	    return exit_addr;
	}
    }
    return (Address) 0;
}

private	AddrOfFunc newfunc_pool;	/* Pool of function addr blocks */
private int	newfunc_top;		/* # of blocks in the pool */
private int	newfunc_index;		/* Index to next free block */

public alloc_functab()
{
    functab = (AddrOfFunc *) calloc(sizeof(AddrOfFunc), number_of_functions);
    if (functab == nil) {
	panic("couldnt malloc enough memory - check \"-f\" flag value");
    }
    newpool(number_of_functions);
}

/*
 * Get a new pool of function address blocks.
 */
private	newpool(n)
{
    if (newfunc_top != newfunc_index) {
	panic("newpool");
    }
    newfunc_pool = (AddrOfFunc) calloc(sizeof(struct AddrOfFunc), n);
    if (newfunc_pool == 0)
	fatal("No memory available. Out of swap space?");
    newfunc_top = n;
    newfunc_index = 0;
}

/*
 * Get a structure for a function address.  These are allocated
 * out of a pool.  When functab is created or enlarged a new
 * pool is gotten.
 */
private	AddrOfFunc get_newfunc()
{
    AddrOfFunc	af;

    af = &newfunc_pool[newfunc_index];
    newfunc_index++;
    return(af);
}

/*
 * Enlarge the function table.
 * Essentially to a realloc() here.  Malloc some new space,
 * and copy the data from the old function table to the new one.
 */
private	grow_functab()
{
    AddrOfFunc *newfunctab;
    int	oldnum;

    oldnum = number_of_functions;
    number_of_functions += NUM_FUNC_INCR;
    newfunctab = (AddrOfFunc *) calloc(sizeof(AddrOfFunc),
    	number_of_functions);
    if (newfunctab == 0)
	fatal("No memory available. Out of swap space?");
    bcopy(functab, newfunctab, oldnum * sizeof(AddrOfFunc));
    dispose(functab);
    functab = newfunctab;
    newpool(NUM_FUNC_INCR);
}

/*
 * Has a file been compiled with the -g option?
 */
public Boolean file_isdebugged(fname)
char	*fname;
{

    Filetab *ftp;
	char buf1[1024], buf2[1024];

	char *a = findsource(fname, buf1);
    for (ftp = &filetab[0]; ftp < &filetab[nlhdr.nfiles]; ftp++) {
	char *b = findsource(ftp->filename, buf2);
		if (a && b && !strcmp(a,b)) {
			return(true);
		}
    }
    for (ftp = &filetab[0]; ftp < &filetab[nlhdr.nfiles]; ftp++) {
	char *b = findsource(ftp->filename, buf2);
		if (same_file(a,b)) {
			return(true);
		}
    }
    return(false);
}
