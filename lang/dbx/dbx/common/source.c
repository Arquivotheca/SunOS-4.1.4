#ifndef lint
static	char sccsid[] = "@(#)source.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Source file management.
 */

#include "defs.h"
#include "symbols.h"
#include "source.h"
#include "object.h"
#include "mappings.h"
#include "machine.h"
#include <ctype.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "library.h"
#include <errno.h>

#ifndef public
String prevsource;
String cursource;
Lineno cursrcline;		/* Current line number for 'list' cmd */
String brksource;		/* File with current breakpoint */
Lineno brkline;			/* Line num of break point */

#define LASTLINE 0		/* recognized by printlines */

#include "lists.h"

List sourcepath;
#endif

private Lineno lastlinenum;
private char curdir[MAXPATHLEN+1];
private char cursourcebuf[MAXPATHLEN];
private char prevsourcebuf[MAXPATHLEN];
extern Boolean nocerror;

/*
 * Data structure for indexing source seek addresses by line number.
 *
 * The constraints are:
 *
 *  we want an array so indexing is fast and easy
 *  we don't want to waste space for small files
 *  we don't want an upper bound on # of lines in a file
 *  we don't know how many lines there are
 *
 * The solution is a "dirty" hash table.  We have NSLOTS pointers to
 * arrays of NLINESPERSLOT addresses.  To find the source address of
 * a particular line we find the slot, allocate space if necessary,
 * and then find its location within the pointed to array.
 */

typedef long Seekaddr;

#define NSLOTS 2
#define NLINESPERSLOT 500

#define slotno(line)    ((line) div NLINESPERSLOT)
#define lineindex(line)	((line) mod NLINESPERSLOT)
#define slot_alloc()    newarr(Seekaddr, NLINESPERSLOT)
#define srcaddr(line)	seektab[slotno(line)][lineindex(line)]

private File srcfp;
private int nslots = NSLOTS;
private Seekaddr **seektab;

/*
 * Print out the given lines from the source.
 */

public printlines(l1, l2)
Lineno l1, l2;
{
    register int c;
    register Lineno i, lb, ub;
    register File f;

    if (cursource == nil) {
	beginerrmsg();
	printf("no source file\n");
	return;
    }
    if (istool()) {
	check_outofdate(cursource);
	dbx_printlines(l1, l2);
	return;
    }
    if (not same_file(cursource, prevsource)) {
	skimsource();
    }
    if (lastlinenum == 0) {
	beginerrmsg();
	printf("couldn't read \"%s\"\n", cursource);
    } else {
	lb = (l1 == 0) ? lastlinenum : l1;
	ub = (l2 == 0) ? lastlinenum : l2;
	if (lb < 1) {
	    beginerrmsg();
	    printf("line number must be positive\n");
	} else if (lb > lastlinenum) {
	    beginerrmsg();
	    if (lastlinenum == 1) {
		printf("\"%s\" has only 1 line\n", shortname(cursource));
	    } else {
		printf("\"%s\" has only %d lines\n",
		    shortname(cursource), lastlinenum);
	    }
	} else if (ub < lb) {
	    beginerrmsg();
	    printf("second number must be greater than first\n");
	} else {
	    if (ub > lastlinenum) {
		ub = lastlinenum;
	    }
	    f = srcfp;
#ifdef FINDCASE
	    /*
	     * Find out if we are about to print a line that's a
	     * switch-case.  I put it in #ifdefs because it might
	     * adversely affect performance.
	     */
	    if( lb > 1 ) {
		lb = checkCase( f, lb, srcaddr(lb-1) );
	    }
#endif FINDCASE
	    fseek(f, srcaddr(lb), 0);
	    for (i = lb; i <= ub; i++) {
		printf("%5d   ", i);
		while ((c = getc(f)) != '\n') {
		    if ((c != '\t') and iscntrl(c)) {
			printf("^");
			if (c == 0177) {
			    printf("?");
			} else {
			    printf("%c", c | ('A' - 1));
			}
		    } else {
			printf("%c", c);
		    }
		}
		printf("\n");
	    }
	    cursrcline = ub + 1;
	}
    }
}


#ifdef FINDCASE
/*
 * Find out if we are about to print a line that's a
 * switch-case.  I put it in #ifdefs because it might
 * adversely affect performance.
 */
#define MAXLINE 100
private checkCase ( f, lb, sa )  File f;  Seekaddr sa; {
  char pline[ MAXLINE ], thisline[ MAXLINE ];

    fseek( f, sa, 0 );
    getCase( pline, f );
    getCase( thisline, f );
    if( strcmp( thisline, "case" ) ) {
	if( strcmp( pline, "case" ) == 0 ) {
	    --lb;
	}
    }
    return lb;
}

getCase ( lbuf, f ) char *lbuf;   File f ; {
  register int i;
  register int ch;

    for( i= 0; i < MAXLINE; ++i ) {
	ch = getc( f );
	if( ch == ' '   || ch == '\t' ) {
	    if( i == 0 ) {
		--i ;
		continue;
	    } else {
		ch = 0;
	    }
	}
	lbuf[ i ] = ch;
	if( ch == '\n' ) {
	    lbuf[i] = 0;
	    break;
	}
    }
}

#endif FINDCASE


/*
 * Search the sourcepath for a file.
 */


public String findsource(filename, fileNameBuf)
String filename, fileNameBuf;
{
    register File f;
    register String src, dir;

    if (filename == nil) {
	return(nil);
    }
    if ( filename[0] == '/' ) {
	src = filename;
    } else {
	src = nil;
	foreach (String, dir, sourcepath)
	    sprintf(fileNameBuf, "%s/%s", dir, filename);
	    f = fopen(fileNameBuf, "r");
	    if (f != nil) {
		fclose(f);
		src = fileNameBuf;
		break;
	    }
	endfor
    }
    return src;
}

/*
 * Open a source file looking in the appropriate places.
 */

public File opensource(filename)
String filename;
{
    String s;
    File f;
	char fileNameBuf[1024];

    s = findsource(filename, fileNameBuf);
    if (s == nil) {
	f = nil;
    } else {
	f = fopen(s, "r");
	check_outofdate(filename);
    }
    return f;
}

/*
 * Check if the given file is out of date with respect to the object file.
 * If so, print a warning.  Also, print warning if file can't be found.
 */
public check_outofdate(file)
char *file;
{
    String dot;
    String base_file;
    String fullfile;
    struct stat statb;
    Symbol sym;
    Name n;
	char fileNameBuf[1024];

	if(file == nil)
		return;
    fullfile = findsource(file, fileNameBuf);
    if( fullfile != nil ) {
	stat(fullfile, &statb);
	if (statb.st_mtime <= get_objtime()) {
	    return;		/* not out of date */
	}
    }

    /* File is either out of date or nonexistent. */

    /* Set n to the Name for <filename> without .o */
    dot = rindex(file, '.');
    if (dot != nil) {
	*dot = '\0';
    }
    base_file = rindex(file, '/');
    if (base_file != nil) {
	    base_file++;
    } else {
	    base_file = file;
    }

    n = identname(base_file, false);

    if (dot != nil) {
	    *dot = '.';
    }

    find (sym, n) where
	sym->class == MODULE and sym->block == program
    endfind(sym);

    if (sym != nil) {
	if( fullfile == nil ) {
	    if( sym->symvalue.funcv.src ) {
		warning( "Cannot find file `%s'.", file );
		sym->symvalue.funcv.src = false;
	    }
	} else if (sym->symvalue.funcv.outofdate == false) {
	    warning("File `%s' has been modified more recently than program `%s'",
		file, objname);
	    sym->symvalue.funcv.outofdate = true;
        }
    }
}

/*
 * Set the current source file.
 */

public setsource(filename)
String filename;
{
	char fileNameBuf[1024];
	String qual_name = findsource(filename, fileNameBuf);
	
	if(qual_name and (not same_file(cursource, qual_name))) {
		if(cursource) {
			strcpy(prevsourcebuf, cursource);
			prevsource = prevsourcebuf;
		} else {
			prevsource = cursource;
		}
		if ( qual_name[0] != '/' ) {
		    qual_name = filename;
		}	
		strcpy(cursourcebuf, qual_name);
		cursource = cursourcebuf;
		cursrcline = 1;
	}

}

public source_init()
{
	seektab = newarr(Seekaddr *, NSLOTS);
	if (seektab == 0)
		fatal("No memory available. Out of swap space?");
	nslots = NSLOTS;
	remember_curdir();
}

/*
 * Read the source file getting seek pointers for each line.
 */

private skimsource()
{
    register int c;
    register Seekaddr count;
    register File f;
    register Lineno linenum;
    register Seekaddr lastaddr;
    register int slot;

    f = opensource(cursource);
    if (f == nil) {
	lastlinenum = 0;
    } else {
	if (prevsource != nil) {
	    free_seektab();
	    if (srcfp != nil) {
		fclose(srcfp);
	    }
	}
	prevsource = cursource;
	linenum = 0;
	count = 0;
	lastaddr = 0;
	while ((c = getc(f)) != EOF) {
	    ++count;
	    if (c == '\n') {
		slot = slotno(++linenum);
		if (slot >= nslots) {
		    seektab = (Seekaddr **) realloc(seektab, 
			(nslots + NSLOTS) * sizeof(Seekaddr *));
		    if (seektab == 0)
			fatal("No memory available. Out of swap space?");
		    bzero((char *) &seektab[nslots], 
			NSLOTS * sizeof(Seekaddr *));
		    nslots += NSLOTS;
		}
		if (seektab[slot] == nil) {
		    seektab[slot] = slot_alloc();
		    if (seektab[slot] == 0)
			fatal("No memory available. Out of swap space?");
		}
		seektab[slot][lineindex(linenum)] = lastaddr;
		lastaddr = count;
	    }
	}
	lastlinenum = linenum;
	srcfp = f;
    }
}

/*
 * Erase information and release space in the current seektab.
 * This is in preparation for reading in seek pointers for a
 * new file.  It is possible that seek pointers for all files
 * should be kept around, but the current concern is space.
 */

private free_seektab()
{
    register int slot;

    for (slot = 0; slot < nslots; slot++) {
	if (seektab[slot] != nil) {
	    dispose(seektab[slot]);
	}
    }
}

/*
 * Figure out current source position.
 */

public getsrcpos()
{
    String filename;

    brkline = srcline(pc);
    setcurfunc(whatblock(pc, true));
    filename = srcfilename(pc);
    setsource(filename);
    if (brkline != 0) {
	cursrcline = brkline;
    }
}

/*
 * Print out the current source position.
 */

public printsrcpos()
{
    printf("at line %d", brkline);
    if (nlhdr.nfiles > 1) {
	printf(" in file \"%s\"", shortname(cursource));
    }
}

#define DEF_EDITOR  "vi"

/*
 * Invoke an editor on the given file.  Which editor to use might change
 * installation to installation.  For now, we use "vi".  In any event,
 * the environment variable "EDITOR" overrides any default.
 */

public edit(filename)
String filename;
{
    extern String getenv();
    String ed, src, s;
    Symbol f;
    Address addr;
    char cmd[100];
    char lineno[10];
	char fileNameBuf[1024];

    if (filename == nil and cursource == nil) {
	error("no file to edit");
    }
    ed = getenv("EDITOR");
    if (ed == nil) {
	ed = DEF_EDITOR;
    }
    src = findsource((filename[0] != '\0') ? filename : cursource, fileNameBuf);
    if (src == nil) {
	f = which(identname(filename, true));
	if (f->class != FUNC and f->class != PROC and f->class != PROG) {
	    error("name clash for \"%s\" (set scope using \"func\")", filename);
	}
	addr = firstline(f);
	if (addr == NOADDR) {
	    error("no source for \"%s\"", filename);
	}
	src = srcfilename(addr);
	s = findsource(src, fileNameBuf);
	if (s != nil) {
	    src = s;
	}
	sprintf(lineno, "+%d", srcline(addr));
    } else {
	sprintf(lineno, "+1");
    }
	if(not streq(ed,DEF_EDITOR)) /* only pass lineno to vi */
		 sprintf(lineno, " ");
    if (istool()) {
	sprintf(cmd, "shelltool %s %s %s", ed, lineno, src);
	system(cmd);
    } else {
	sprintf(cmd, "%s %s %s", ed, lineno, src);
	system(cmd);
    }
} 

#define REGEX_BAD	-1		/* Bad regular expression */ 
#define REGEX_MISSING	0		/* Did not find regular expression */ 
#define REGEX_FOUND	1		/* Found regular expression */ 

private char *reg_exp;			/* The last regular expression */

/*
 * Search the current source file for a given regular expression.
 */
public	search_src(str, forward)
char *str;
Boolean forward;
{
    char *err;
    char *re_comp();

    if (str[0] == '\0') {
	if (reg_exp == nil) {
	    error("No saved search string");
	}
	str = reg_exp;
    } else {
	reg_exp = strdup(str);
    }
    err = re_comp(str);
    if (err != nil) {
	error("Bad regular expression: %s", err);
    }
    if (not same_file(cursource, prevsource)) {
	skimsource();
    }
    if (srcfp == nil) {
	error("Source `%s' not found", shortname(cursource));
    }
    if (forward) {
	search_forward(str);
    } else {
	search_backward(str);
    }
}

/*
 * Search towards the end of the file for a regular expression.
 */
private search_forward(str)
char *str;
{
    Lineno n;
    char line[256];

    fseek(srcfp, srcaddr(cursrcline + 1), 0);
    for (n = cursrcline + 1; fgets(line, sizeof(line), srcfp) != nil; n++) {
	if (do_search(line, n) == REGEX_FOUND) {
	    return;
	}
    }
    error("End-of-file; did not find search string: %s", str);
}

/*
 * Search towards the beginning of the file for a regular expression.
 */
private search_backward(str)
char *str;
{
    Lineno n;
    char line[256];

    for (n = cursrcline - 1; n > 0; n--) {
        fseek(srcfp, srcaddr(n), 0);
	fgets(line, sizeof(line), srcfp);
	if (do_search(line, n) == REGEX_FOUND) {
	    return;
	}
    }
    error("Top-of-file; did not find search string: %s", str);
}

/*
 * Compare a line with a regular expression.
 * If a match is found, print the line.
 */
private do_search(line, n)
char *line;
Lineno n;
{
    int r;

    if ((r = re_exec(line)) == REGEX_FOUND) {
	if (istool()) {
	    dbx_emphasize(n);
	} else {
	    printlines(n, n);
	}
        cursrcline = n;
        return(REGEX_FOUND);
    } else if (r == REGEX_BAD) {
        error("Internal error in regular expression handler");
    }
    return(REGEX_MISSING);
}

public remember_curdir()
{

	char *end;

	nocerror=true;
	end = curdir+strlen(getwd(curdir));
	*end++='/';
	*end='\0';
	nocerror=false;
}

public String shortname(name)
char *name;
{
	/* abbreviate an absolute file name by omitting cwd */
	char *c;
	int d;
	if(name && name[0] == '/' && (d = substr(name,curdir))) {
		c = name+d;
	} else {
		c= name;
	}
	return c;
}

private substr(path,cwd)
char *path,*cwd;
{
	register char *p, *d;

	for(p=path,d=cwd; *p != '\0' && *d != '\0' && *p == *d; p++, d++ );
	if(*p == '\0') /* ran out of path ?! */
		return 0;
	/* if we did not match all of cwd backup to the last / */
	if( *d != '\0' ) {
		while(p != path && *p != '/')
			p--;
		if(p != path)
			p++;
	}
	return (p-path);
}


public String export_curdir()
{
	return(curdir);
}

#define STAT_HASH_SIZE 16

struct Stat_cache {
	dev_t  st_dev;
	ino_t  st_ino;
	struct Stat_cache *next;
	int	hash;
	int	len;
	char	string[1];
};
private struct Stat_cache *stat_hash[STAT_HASH_SIZE];


private Boolean fast_stat(path, statp)
String path;
struct stat *statp;
{
	int hash= 0, len= 0;
	char *p;
	struct Stat_cache *slot;

	for (p = path; *p != 0; p++) {
		hash += *p;
		len++;
	}

	slot = stat_hash[hash % STAT_HASH_SIZE];
	for (; slot != NULL; slot = slot->next) {
		if ((slot->hash == hash) &&
		    (slot->len == len) &&
		    !strcmp(path, slot->string)) {
			statp->st_dev = slot->st_dev;
			statp->st_ino = slot->st_ino;
			return slot->st_ino == -1;
		}
	}

	slot = (struct Stat_cache *)malloc(sizeof(struct Stat_cache)+len);
	if (slot == 0)
		fatal("No memory available. Out of swap space?");
	slot->next = stat_hash[hash % STAT_HASH_SIZE];
	stat_hash[hash % STAT_HASH_SIZE] = slot;
	if (stat(path, statp)) {
		statp->st_ino = -1;
		statp->st_dev = -1;
	}
	slot->st_ino = statp->st_ino;
	slot->st_dev = statp->st_dev;
	slot->hash = hash;
	slot->len = len;
	strcpy(slot->string, path);
	return slot->st_ino == -1;
}

public clear_stat_cache()
{
	int	n;
	struct Stat_cache *p;

	for (n = STAT_HASH_SIZE-1; n >= 0; n--) {
		for (p = stat_hash[n]; p != NULL; p = p->next) {
			free(p);
		}
		stat_hash[n] = NULL;
	}
}


public Boolean same_file(path1,path2)
String path1,path2;
{
	struct stat stat1, stat2;
	Boolean areequal;

	nocerror=true;
	if (!path1 or !path2) {
		areequal = false;
	} else if (!strcmp(path1, path2)) {
		areequal = true;
	} else if (fast_stat(path1,&stat1) or fast_stat(path2,&stat2) ) {
		areequal = false;
	} else if((stat1.st_ino != stat2.st_ino) ||
		  (stat1.st_dev != stat2.st_dev)) {
		areequal = false;
	} else {
		areequal = true;
	}
	nocerror=false;
	return areequal;
}
