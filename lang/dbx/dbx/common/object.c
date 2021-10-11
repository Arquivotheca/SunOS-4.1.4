#ifndef lint
static	char sccsid[] = "@(#)object.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Object code interface, mainly for extraction of symbolic information.
 */

#include <sys/file.h>
#include "defs.h"
#include "machine.h"
#include "dbxmain.h"
#include "symbols.h"
#include "object.h"
#include "names.h"
#include "library.h"
#include "languages.h"
#include "mappings.h"
#include "lists.h"
#include <a.out.h>
#include <stab.h>
#define	 N_BROWS 0x48		/* Zap when 4.1 <stab.h> is installed */
#include <ctype.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef TIMING
#include <sys/time.h>
struct	timeval	timebuf1;
Boolean timeflag;
struct	timeval	timebuf2;
#endif

#define N_INDIRECT    0x1a    /* Indirect symbol produced by ilink */
			      /* This #define should be in <a.out.h> */

#ifndef public

#include <sys/types.h>
#include <link.h>

struct {
    unsigned int stringsize;	/* size of the dumped string table */
    unsigned int nsyms;		/* number of symbols */
    unsigned int nfiles;	/* number of files */
    unsigned int nlines;	/* number of lines */
} nlhdr;

#define is_pas_fort_main_rtn(s) ( \
  ((strip_ or pascal_pgm) and streq(symname(s), "main")) \
)

struct Modnode {
    Name name;
    int  unitno;
    char key[24];	/* module key : 24 characters */
    Symbol  sym;
    struct Modnode *link;
} *modlist;

#endif

public struct link_dynamic dynam;
public caddr_t dynam_addr;
public use_shlib;

public Address codestart;
public String objname;
public Integer objsize;
public char *stringtab;

#ifdef COFF
public SCNHDR shdr;
private char *coffstringtab;
private char *cofflinetab;
#endif COFF
private String progname = nil;
private Language curlang;
private Symbol curmodule;
private Symbol curparam;
private Boolean warned;
private Symbol curcomm;
private Symbol commchain;
public Boolean strip_ = false; /* true if we've seen a fortran module */

public Boolean pascal_pgm = false;	/* true if we've seen a pascal module */
private Boolean pascal_module = false;	/* true if current module is Pascal */

public Boolean modula2_pgm = false;

private Filetab *filep;
private Linetab *linep, *prevlinep;
private Address curfaddr;

extern Boolean nocerror;
private Symbol	gettype();
private Name	make_filename();
private char *mk_absolutepath();
private char *compile_dir;

private char *where_in_obj();

#define curfilename() (filep-1)->filename

/*
 * Blocks are figured out on the fly while reading the symbol table.
 */

#define MAXBLKDEPTH 25

private Symbol curblock;
private Symbol blkstack[MAXBLKDEPTH];
private Integer curlevel;
private Integer nesting;

Boolean ignore_debug_info;	/* are we ignoring stab.h symbols from this file?*/
int nobj_seen, nobj_used;

/*
private Address addrstk[MAXBLKDEPTH];
*/

/*
 * Here's the situation on Pascal nested routines whose names are qualified
 * as in "block1.block2.block3".  There will be a "regular" symbol entry
 * for the routine, as well as a POSSIBLE stab entry.  The regular entry
 * comes first.  Check_local is entered for this entry and an entry is
 * made for "block?".  (The "parent" nodes are built, if necessary, and the
 * link is made to them.)  Then when entersym is entered, an entry already
 * exists.  (To find the entry, one must strip off the prefixes, look
 * through all dbx symbol entries for the suffix, and make sure the
 * prefix build from the parent(s) of this entry match the name in the
 * stab entry.)
 *
 * Nested Pascal routines are distinguised from module names ("t.o") by
 * the fact that the former are prefixed by '_'.  This means module names
 * can't start with a '_'.
 *
 * Secondary Pascal entry points are prefixed by "_._" in the symbol table.
 */

/*
 * A NOTE REGARDING PASCAL NESTED ROUTINES:
 *
 * Nested routines are always local and should have a local entry before
 * the -lg flag.  These entries are seen before the stab entry.  There
 * may or may not be a stab entry, depending upon whether or not the 
 * module is compiled with the -g flag but regardless, the proper dbx
 * symbol table entries are built for the routine and its parent(s)
 * so that the user can, say, "print &a.b.c".  THERE IS ONE PROBLEM,
 * HOWEVER: if the module containing nested procs is going through c2
 * (-O is on), pc puts out the nested routines as globals (after the
 * -lg flag) because otherwise c2 throws them away (this is a bug in c2).
 * The bottom line of all this is that for optimized modules (which by
 * definition means they are not compiled with -g flag) nested routines
 * are invisible.
 *  
 */

#define enterblock(b) { \
    blkstack[curlevel] = curblock; \
    ++curlevel; \
    b->level = curlevel; \
    b->block = curblock; \
    curblock = b; \
}

#define exitblock() { \
    if (curblock->class == FUNC or curblock->class == PROC) { \
	if (prevlinep != linep) { \
	    curblock->symvalue.funcv.src = true; \
	} \
    } \
    --curlevel; \
    curblock = blkstack[curlevel]; \
}

/*
 * Enter a source line or file name reference into the appropriate table.
 * Expanded inline to reduce procedure calls.
 *
 * private enterline(linenumber, address)
 * Lineno linenumber;
 * Address address;
 *
 * Also see comment in 'allocmaps()'. (ivan)
 *  ...
 */

#define enterline(linenumber, address) \
{ \
    register Linetab *lp; \
 \
    lp = linep - 1; \
    if (linenumber != lp->line) { \
	if (address != lp->addr) { \
	    ++lp; \
	} \
	lp->line = linenumber; \
	lp->addr = address; \
	linep = lp + 1; \
    } else { \
	lp->addr = address; \
    } \
}

#define NTYPES 1000
#define NSCOPE 500

private Symbol typetable[NTYPES];
private Symbol scopetable[NSCOPE];

/*
 * Read in the namelist from the obj file.
 *
 * Reads and seeks are used instead of fread's and fseek's
 * for efficiency sake; there's a lot of data being read here.
 */

public Boolean readobj(file)
String file;
{
    Fileid f;
    struct exec hdr;
    struct nlist nlist;
#ifdef COFF
    FILHDR fhdr;
    register int ix;
#endif COFF

    f = open(file, 0);
    if (f < 0) {
	warning("can't open %s", file);
	return(false);
    }
    use_shlib = 0;
    endfunc = nil;
	ignore_debug_info = false;
#ifndef COFF
    read(f, &hdr, sizeof(hdr));
    codestart = hdr.a_entry;
    objsize = hdr.a_text;
    nlhdr.nsyms = hdr.a_syms / sizeof(nlist);
#else
    read(f, &fhdr, sizeof(fhdr));
    read(f, &hdr, fhdr.f_opthdr);
    codestart = hdr.a_syms;
    objsize = hdr.a_text;
    nlhdr.nsyms = fhdr.f_nsyms;
    for (ix = 0; ix < fhdr.f_nscns; ix++) {
	read(f, &shdr, SCNHSZ);
	if (!strcmp(shdr.s_name, _TEXT)) break;
    }
#endif COFF
    nlhdr.nfiles = nlhdr.nsyms;
    nlhdr.nlines = nlhdr.nsyms;
    if (nlhdr.nsyms > 0) {
	printf("Reading symbolic information...\n");
	fflush(stdout);
	clear_stat_cache();
#ifdef TIMING
	gettimeofday(&timebuf2, NULL);
	prtime(&timebuf2, &timebuf1, "Before symbol processing");
	gettimeofday(&timebuf1, NULL);
#endif
#ifndef COFF
    	lseek(f, (long) N_STROFF(hdr), 0);
    	read(f, &(nlhdr.stringsize), sizeof(nlhdr.stringsize));
    	nlhdr.stringsize -= 4;
    	stringtab = newarr(char, nlhdr.stringsize);
	if (stringtab == 0)
		fatal("No memory available. Out of swap space?");
    	read(f, stringtab, nlhdr.stringsize);
#else
    	lseek(f, fhdr.f_symptr+fhdr.f_nsyms*SYMESZ, 0);
    	read(f, &(nlhdr.stringsize), sizeof(nlhdr.stringsize));
    	nlhdr.stringsize -= 4;
	coffstringtab = newarr(char, nlhdr.stringsize);
	if (coffstringtab == 0)
		fatal("No memory available. Out of swap space?");
	read(f, coffstringtab, nlhdr.stringsize);
	if (shdr.s_lnnoptr != 0) {
		nlhdr.nlines = shdr.s_nlnno; /* we have a better estimate for COFF */
		if (fhdr.f_symptr-shdr.s_lnnoptr > shdr.s_nlnno*LINESZ)
		nlhdr.nlines = (fhdr.f_symptr-shdr.s_lnnoptr)/LINESZ;
		cofflinetab = newarr(char, nlhdr.nlines*LINESZ);
		if (cofflinetab == 0)
			fatal("No memory available. Out of swap space?");
		lseek(f, shdr.s_lnnoptr, 0);
		read(f, cofflinetab, nlhdr.nlines*LINESZ);
	}
#endif COFF
#ifdef TIMING
	gettimeofday(&timebuf2, NULL);
	prtime(&timebuf2, &timebuf1, "to read strings");
#endif
    	allocmaps(nlhdr.nfiles, nlhdr.nlines);
#ifndef COFF
    	lseek(f, (long) N_SYMOFF(hdr), 0);
#else
	lseek(f, fhdr.f_symptr, 0);
#endif COFF
    	readsyms(f);
#ifdef TIMING
	gettimeofday(&timebuf2, NULL);
	prtime(&timebuf2, &timebuf1, "");
	timebuf1 = timebuf2;
#endif
    	ordfunctab();
    	setnlines();

    	set_sigtramp_bounds();
	printf("Read %d symbols", nlhdr.nsyms);
	if(is_sources_sel_list_active()) {
		printf("  (%d of %d files selected) ", nobj_used, nobj_seen);
	}
	putchar('\n');
	fflush(stdout);
    } else {
    	warning("no symbols");
    }
    close(f);
    return(true);
}

#ifdef COFF

#define S_ABS   -1
#define S_UNDEF  0
#define S_TEXT   1
#define S_DATA   2
#define S_BSS    3

#define FILESYM(S)  !strcmp(S->_n._n_name, ".file")
#define TEXTSYM(S)  !strcmp(S->_n._n_name, _TEXT)
#define DATASYM(S)  !strcmp(S->_n._n_name, _DATA)
#define BSSSYM(S)   !strcmp(S->_n._n_name, _BSS)
#define COMMSYM(S) !strncmp(S->_n._n_name, ".comment", 8)
#define SPECIALSYM(S) (TEXTSYM(S)||DATASYM(S)||BSSSYM(S)||COMMSYM(S))
#define bumpsym(sp) sp = (SYMENT *) ((int)sp+SYMESZ*(sp->n_numaux+1))
#define bumplin(lp) lp = (LINENO *) ((int)lp+LINESZ)
#define aux(nlp) ((AUXENT *) ((int) (nlp) + SYMESZ))
#define auxch(nlp) ((char *) ((int) (nlp) + SYMESZ))


/*
 *
 */
void printstabs(nl, num, strtab)
struct nlist *nl;
int num;
char         *strtab;
{
    register int ix;

    for (ix = 0; ix < num; ix++) {
        printf("  %08x  ", nl[ix].n_value);
        printf("%04x ", nl[ix].n_desc);
	printf("(%3x)", nl[ix].n_type);
        switch(nl[ix].n_type) {
            case N_GSYM:  printf(" GSYM"); break;
            case N_FNAME: printf("FNAME"); break;
            case N_FUN:   printf("  FUN"); break;
            case N_STSYM: printf("STSYM"); break;
            case N_LCSYM: printf("LCSYM"); break;
            case N_MAIN:  printf(" MAIN"); break;
            case N_RSYM:  printf(" RSYM"); break;
            case N_SLINE: printf("SLINE"); break;
            case N_SSYM:  printf(" SSYM"); break;
            case N_SO:    printf("   SO"); break;
            case N_LSYM:  printf(" LSYM"); break;
            case N_BINCL: printf("BINCL"); break;
            case N_SOL:   printf("  SOL"); break;
            case N_PSYM:  printf(" PSYM"); break;
            case N_EINCL: printf("EINCL"); break;
            case N_ENTRY: printf("ENTRY"); break;
            case N_LBRAC: printf("LBRAC"); break;
            case N_EXCL:  printf(" EXCL"); break;
            case N_RBRAC: printf("RBRAC"); break;
            case N_BCOMM: printf("BCOMM"); break;
            case N_ECOMM: printf("ECOMM"); break;
            case N_ECOML: printf("ECOML"); break;
            case N_LENG:  printf(" LENG"); break;
            case N_PC:    printf("   PC"); break;
            case N_M2C:   printf("  M2C"); break;
            case N_SCOPE: printf("SCOPE"); break;
	    case N_BROWS: printf("BROWS"); break;
            default:
                switch (nl[ix].n_type & 0xe) {
                    case N_UNDF: printf(" UNDF"); break;
                    case N_ABS:  printf("  ABS"); break;
                    case N_TEXT: printf(" TEXT"); break;
                    case N_DATA: printf(" DATA"); break;
                    case N_BSS:  printf("  BSS"); break;
                    default:     printf("     "); break;
                }
        }
        if (nl[ix].n_un.n_strx > 0)
            printf(" %s\n", &strtab[nl[ix].n_un.n_strx-4]);
	else printf("\n");
    }
    fflush(stdout);
}


/*
 *
*/
public int stringtabsize(cnl, cst, nsyms)
SYMENT       *cnl;
char         *cst;
unsigned int nsyms;
{
    register int ix, jx;
    SYMENT        *p;

    for (ix=0, jx=0, p=cnl; ix < nsyms; ix++, jx++) {
	if (p->n_leading_zero == 0 and p->n_zeroes != 0)
	    jx += strlen(&cst[p->n_offset-4]);
	else if (FILESYM(p))
	    jx += strlen(auxch(p));
	else if (p->n_zeroes != 0)
	    jx += strlen(p->_n._n_name) >= 8 ? 9 : strlen(p->_n._n_name);
	else
	    jx += strlen(&cst[p->n_offset-4]);
	ix += p->n_numaux;
	bumpsym(p);
    }
    return jx;
}


public unsigned char stabtype(cnl)
SYMENT       *cnl;
{
    unsigned char type;

    switch (cnl->n_scnum) {
	case S_ABS:   type = N_ABS;  break;
	case S_UNDEF: type = N_UNDF; break;
	case S_TEXT:  type = N_TEXT; break;
	case S_DATA:  type = N_DATA; break;
	case S_BSS:   type = N_BSS;  break;
	default:      type = N_UNDF; break;
    }
    switch (cnl->n_sclass) {
	case C_EXT:   type |= N_EXT;  break;
	case C_TPDEF: type |= N_TYPE; break;
	default:                      break;
    }
    return type;
}


/*
 * This procedure performs the bulk of the work of converting coff style
 * symbols to regular stabs.
 */
public long massagecoff(f, cnl, nl, strtab, cstrtab, lintab, nsyms, nlines, stlen)
Fileid       f;
SYMENT       *cnl;	/* coff namelist */
struct nlist *nl;	/* namelist */
char         *strtab;	/* string table */
char         *cstrtab;	/* coff string table */
char         *lintab;	/* line table */
unsigned int nsyms;	/* number of symbols */
unsigned int nlines;	/* number of lines */
int          stlen;     /* size of allocated string table */
{
    register long ix, jx;
    char     *ch;
    long     strindx;
    Boolean  getsrclines;
    Boolean  patchfileentry;
    long     patchfileindex;
    LINENO   *curline;
    LINENO   *lastline;

    patchfileentry = false;
    getsrclines = false;
    strindx = 0;
    for (ix=0, jx=0; ix < nsyms; ix++) {
	switch (cnl->n_sclass) {
	    case C_AUTO:
		if (ISARY(cnl->n_type)) goto nextiteration; break;
	    case C_STAT:
		if (SPECIALSYM(cnl) || ISARY(cnl->n_type)) {
		    if (TEXTSYM(cnl) and patchfileentry) {
			nl[patchfileindex].n_value = cnl->n_value;
			patchfileentry = false;
		    }
		    goto nextiteration;
		}
		break;
	    case C_MOS:
	    case C_MOU:
	    case C_TPDEF:
		if (ISARY(cnl->n_type)) goto nextiteration; break;
	    case C_FCN:
	    case C_BLOCK:
	    case C_STRTAG:
	    case C_UNTAG:
	    case C_ENTAG:
	    case C_EOS:
		goto nextiteration;
	    default:
		break;
	}

	if (cnl->n_dbx_type == N_SO)
	    getsrclines = true;
	else if (getsrclines and (cnl->n_dbx_type == N_FUN)) {
	    curline = (LINENO *) &lintab[aux(cnl)->x_sym.x_fcnary.x_fcn.x_lnnoptr-shdr.s_lnnoptr];
	    if (curline->l_addr.l_symndx == ix and curline->l_lnno == 0) {
		bumplin(curline);
		lastline = (LINENO *) (lintab + ((nlines-1) * LINESZ));
		while ((curline->l_lnno != 0) and (curline <= lastline)) {
		    nl[jx].n_un.n_strx = 0;
		    nl[jx].n_type  = N_SLINE;
		    nl[jx].n_other = 0;
		    nl[jx].n_desc  = curline->l_lnno;
		    nl[jx].n_value = curline->l_addr.l_paddr;
		    bumplin(curline);
		    jx++;
		}
	    }
	    else warning("Invalid line numbers in file");
	    getsrclines = false;
	}

	if (cnl->n_leading_zero == 0 and cnl->n_zeroes != 0) {
	    strcpy(&strtab[strindx], &cstrtab[cnl->n_offset-4]);
	    nl[jx].n_type = cnl->n_dbx_type;
	    nl[jx].n_desc = cnl->n_dbx_desc;
	}
	else if (FILESYM(cnl)) {
	    strcpy(&strtab[strindx], auxch(cnl));
	    ch = rindex (&strtab[strindx], '.');
	    if (ch != NULL) {
		*(++ch) = 'o';
		*(++ch) = 0;
	    }
	    nl[jx].n_type = N_TEXT;
            nl[jx].n_desc = 0;
	    if (cnl->n_value == 0) {
		patchfileentry = true;
		patchfileindex = jx;
	    }
	    else patchfileentry = false;
	}
	else {
	    if (cnl->n_zeroes != 0) {
		strncpy(&strtab[strindx], cnl->_n._n_name, 8);
		if (strlen(cnl->_n._n_name) > 8)
		    strtab[strindx+8] = '\0';
	    }
	    else strcpy(&strtab[strindx], &cstrtab[cnl->n_offset-4]);
	    switch (cnl->n_scnum) {
		case S_ABS:   nl[jx].n_type = N_ABS;  break;
		case S_UNDEF: nl[jx].n_type = N_UNDF; break;
		case S_TEXT:  nl[jx].n_type = N_TEXT; break;
		case S_DATA:  nl[jx].n_type = N_DATA; break;
		case S_BSS:   nl[jx].n_type = N_BSS;  break;
		default:      nl[jx].n_type = N_UNDF; break;
	    }
	    switch (cnl->n_sclass) {
		case C_EXT:   nl[jx].n_type |= N_EXT;  break;
		case C_TPDEF: nl[jx].n_type |= N_TYPE; break;
		default:                               break;
	    }
	    nl[jx].n_desc = 0;
	}
	nl[jx].n_value = cnl->n_value;
	nl[jx].n_other = 0;
	nl[jx].n_un.n_strx  = strindx + 4;
	strindx  += strlen(&strtab[strindx]) + 1;
	jx++;

nextiteration:
	ix += cnl->n_numaux;
	bumpsym(cnl);
    }

#ifdef DEBUGGING_SYMS
    printstabs(nl, jx, strtab);
    printf("Final strindx: %d; allocated space: %d\n", strindx, stlen);
    fflush(stdout);
#endif DEBUGGING_SYMS
    if(strindx > stlen)
	panic_and_where("allocated string table too small");
    return jx;
}
#endif COFF


/*
 * Read in symbols from object file.
 */

private readsyms(f)
Fileid f;
{
    struct nlist *namelist;
#ifdef COFF
    SYMENT *coffnamelist;
    SYMENT *p;
    int jx;
#endif COFF
    register struct nlist *ub;
    register int ix;
    register String name;
    register Boolean afterlg;
    struct nlist *np;

    initsyms();
#ifndef COFF
    namelist = newarr(struct nlist, nlhdr.nsyms);
    if (namelist == 0)
	fatal("No memory available. Out of swap space?");
#else
	namelist = newarr(struct nlist, nlhdr.nsyms+nlhdr.nlines);
    if (namelist == 0)
	fatal("No memory available. Out of swap space?");
    coffnamelist = newarr(SYMENT, nlhdr.nsyms);
    if (coffnamelist == 0)
	fatal("No memory available. Out of swap space?");
#endif COFF
#ifdef TIMING
    gettimeofday(&timebuf1, NULL);
#endif
#ifndef COFF
    read(f, namelist, nlhdr.nsyms * sizeof(struct nlist));
#else
    read(f, coffnamelist, nlhdr.nsyms * SYMESZ);
    jx = stringtabsize(coffnamelist, coffstringtab, nlhdr.nsyms);
    stringtab = newarr(char, jx);
    if (stringtab == 0)
	fatal("No memory available. Out of swap space?");
    nlhdr.nsyms = massagecoff(f, coffnamelist, namelist, stringtab, coffstringtab,
					cofflinetab, nlhdr.nsyms, nlhdr.nlines, jx);
    nlhdr.stringsize = jx;
    dispose(coffnamelist);
    dispose(coffstringtab);
    if (shdr.s_lnnoptr != 0)
	dispose(cofflinetab);
#endif COFF
#ifdef TIMING
    gettimeofday(&timebuf2, NULL);
    prtime(&timebuf2, &timebuf1, "to read symbols");
#endif
	if(is_sources_sel_list_active())
		find_implied_files(namelist, stringtab, nlhdr.nsyms);
    afterlg = false;
    ub = &namelist[nlhdr.nsyms];
    for (np = &namelist[0]; np < ub; np++) {
	ix = np->n_un.n_strx;
	if (ix > nlhdr.stringsize+4) {
	    continue;
	}
	if (ix != 0) {
	    name = &stringtab[ix - 4];
	    if (streq(name, "__DYNAMIC")){
		dynam_addr = (caddr_t)np->n_value;
		if (dynam_addr)
		    use_shlib++;
	    }
	} else {
	    name = nil;
	}

#ifdef DEBUGGING
	_readsyms_hook( name, np - namelist );
#endif

	/*
	 * assumptions:
	 *	not an N_STAB		==> name != nil
	 *	name[0] == '-' (HYPHEN)	==> name == "-lg"
	 *	name[0] != '_' (UNDER)	==> filename or invisible
	 *
	 * The "-lg" or the first global text symbol signals the 
	 * beginning of global loader symbols.
	 *
	 */
	if ((np->n_type & N_STAB) != 0) {
	    enter_nl(name, &np, 0);
	} else if ((np->n_type & N_TYPE) == N_INDIRECT) {
	    continue;
	} else if (not afterlg and 
	  ((name != nil and name[0] == '-') or np->n_type  == (N_TEXT|N_EXT))) {
	    afterlg = true;
		compile_dir = nil;
	    while (curblock->class != PROG) {
		exitblock();
	    }
	    enterline(0, (linep-1)->addr + 1);
	    check_global(name, np, 0);
	} else if (kernel and (np->n_type & N_TYPE) == N_ABS) {
	    kernel_abs(name, np);
	} else if (afterlg) {
	    check_global(name, np, 0);
	} else if (name[0] == '_') {
	    check_local(&name[1], np, 0);
	} else if ((np->n_type & N_TYPE) == N_TEXT) {
	    check_filename(name, np, 0);
	}
    }
    dispose(namelist);
    if (not afterlg) {
	warning("not linked for debugging, use \"cc -g ...\"");
    }
	if(is_sources_sel_list_active()) {
		print_sel_list_exceptions();
	}
/*@* strip_ = false; *@*@*/
}

#ifdef DEBUGGING
private dump_filetab()
{
    {
	Filetab *tmpfilep;

	for(tmpfilep = filetab; tmpfilep < filep; tmpfilep++) {
	    printf("%8x", tmpfilep->addr);
	    printf("\t%s", tmpfilep->modname);
	    printf("\t%s\n", tmpfilep->filename);
	}
	printf("\n");
	fflush(stdout);
    }
}
#endif DEBUGGING

/*
 * Strip a trailing underscore from the name.
 * Thanks fortran!
 */
private strip_us(name)
String  name;
{
    register char *p;

    p = name;
    while (*p != '\0') {
	++p;
    }
    --p;
    if (*p == '_') {
	*p = '\0';
    }
}

/*
 * Initialize symbol information.
 */

private initsyms()
{
    curblock = nil;
    curlevel = 0;
	nobj_seen = nobj_used = 0;
    nesting = 0;
    if (progname == nil) {
	progname = strdup(objname);
	if (rindex(progname, '/') != nil) {
	    progname = rindex(progname, '/') + 1;
	}
	if (index(progname, '.') != nil) {
	    *(index(progname, '.')) = '\0';
	}
    }
    program = insert(identname(progname, true));
    program->class = PROG;
    program->symvalue.funcv.beginaddr = codestart;
    program->symvalue.funcv.inline = false;
    newfunc(program, codeloc(program));
/*  findbeginning(program); */
    enterblock(program);
    curmodule = program;
}

/*
 * Enter a namelist entry.
 */

public enter_nl(name, npp, n_value_offset)
String name;
register struct nlist **npp;
int n_value_offset;
{
    register Symbol s;
    register Name n;
    char *p;

	if(ignore_debug_info) {
		return;
	} else switch ((*npp)->n_type) {
	/*
	 * Build a symbol for the FORTRAN common area.  All GSYMS that follow
	 * will be chained in a list with the head kept in common.offset, and
	 * the tail in common.chain.
	 */
	case N_BCOMM:
 	    if (curcomm) {
		curcomm->symvalue.common.chain = commchain;
	    }
    	    if (name == nil) {
		n = nil;
    	    } else {
		n = identname(name, true);
    	    }
	    find (curcomm, n)
		where curcomm->class == COMMON
	    endfind(curcomm);
	    if (curcomm == nil) {
		curcomm = insert(n);
		curcomm->class = COMMON;
		curcomm->block = curblock;
		curcomm->level = program->level;
		curcomm->language = curblock->language;
		curcomm->symvalue.common.chain = nil;
	    }
	    commchain = curcomm->symvalue.common.chain;
	    break;

	case N_ECOMM:
	    if (curcomm) {
		curcomm->symvalue.common.chain = commchain;
		curcomm = nil;
	    }
	    break;

	case N_LBRAC:
/*
	    ++nesting;
	    addrstk[nesting] = (linep - 1)->addr + 4; /* starts after this
							 instruction
*/
	    break;

	case N_RBRAC:
/*
	    if (addrstk[nesting] == NOADDR) {
		exitblock();
		newfunc(curblock, (linep - 1)->addr);
	    }
	    --nesting;
*/
	    break;

	case N_SLINE:
	    {  unsigned short lnr;
		lnr = (unsigned short)  (*npp)->n_desc;
		enterline((Lineno) lnr, (Address) (*npp)->n_value+n_value_offset);
	    }
	    break;

	/*
	 * Modula-2 compilation unit
	 */
	case N_M2C:
	    /* get mod name */
	    p = index(name, ',');
	    if (p==nil) 
		panic("missing ',' for N_M2C at %s %s", name,
				    where_in_obj());
	    *p = '\0';
	    n = identname(name, true);
	    add_modlist(n, (*npp)->n_desc, p+1);
	    break;

	/*
	 * Source files.
	 */
	case N_SO:
#ifdef TIMING
	    if (timeflag) {
	        gettimeofday(&timebuf2, NULL);
		prtime(&timebuf2, &timebuf1, "");
		timebuf1 = timebuf2;
	    } else {
	        timeflag = true;
	        gettimeofday(&timebuf1, NULL);
	    }
	    printf("%s: ", name);
#endif

		name = mk_absolutepath(name);
		if(name) {
		    n = make_filename(name);
		    enterSourceModule(n, 
				    (Address)(*npp)->n_value+n_value_offset);
		    new_srcfile(name, *npp);
		}
	    break;

	/*
	 *
	 */
	case N_MAIN:
	    main_routine = strdup(name);
	    break;

	/*
	 * Begin an include (header) file.
	 */
	case N_BINCL:
	    begin_incl(name, *npp);
	    break;

	/*
	 * End an include (header) file.
	 */
	case N_EINCL:
	    end_incl();
	    break;

	/*
	 * An include (header) file excluded by the loader.
	 */
	case N_EXCL:
	    excl_incl(name, *npp);
	    break;

	case N_SOL:
		name = mk_absolutepath(name);
		if(name) {
	    	n = make_filename(name);
            enterfile(n, (Address) (*npp)->n_value+n_value_offset);
		}
	    break;

	/*
	 * These symbols are assumed to have non-nil names.
	 */
	case N_SCOPE:
	case N_GSYM:
	case N_FUN:
	case N_STSYM:
	case N_LCSYM:
	case N_RSYM:
	case N_PSYM:
	case N_LSYM:
	case N_SSYM:
	case N_LENG:
		if (index(name, ':') == nil) {
			if (not warned) {
				warned = true;
				warning("old style symbol information found in \"%s\"",
				curfilename());
			}
	    } else {
			entersym(name, npp);
	    }
	    break;

	case N_PC:
#ifdef I
		if (name[0] is '_')	{ /* ignore file names */
			n = identname(name, true);
		}
#endif
	    break;

	case N_BROWS:
	    break;

	default:
	    printf("warning:  stab entry unrecognized: ");
	    if (name != nil) {
		printf("name %s,", name);
	    }
	    printf("ntype %2x, desc %x, value %x\n",
		(*npp)->n_type, (*npp)->n_desc, (*npp)->n_value);
	    break;
	}
}

private char *mk_absolutepath(name)
char *name;
{
		static char buf[MAXPATHLEN];
		struct stat statbuf;
		char *p;

		/* a name with a trailing / indicates the cwd of the compile command*/
		if(name[strlen(name)-1] == '/') {
			/* skip automounter mount point */
			if(strncmp(name,"/tmp_mnt/",9) ==0) name += 8;
			compile_dir = name;
			p = nil;
		} else if (compile_dir != nil) {
			sprintf(buf,"%s%s",compile_dir,name);
			nocerror = true;
			if(stat(buf,&statbuf) == 0) {
				p = strdup(buf);
			} else {
				p = name;
			}
			nocerror = false;
		} else {
			p = name;
		}
		return p;
}

/*
 * From a string make a name node for a file name.
 * Look for a leading "./" and remove it.
 */
private Name make_filename(name)
char *name;
{
    Name n;

    if (name == nil) {
	n = nil;
    } else {
	if (name[0] == '.' and name[1] == '/') {
	    name = &name[2];
	}
	n = identname(name, true);
    }
    return(n);
}

/*
 * Check to see if a global _name is already in the symbol table,
 * if not then insert it.
 */

public check_global(name, np, n_value_offset)
String name;
register struct nlist *np;
int n_value_offset;
{
    register Name n;
    register Symbol t, u;
    char *p;
    char *file;

#ifdef i386
    if (index(name, '$') != nil and modula2_pgm)
	return;		/* skip modula2 symbol */
#else
    if (name[0] == '_' and index(name, '$') != nil and modula2_pgm)
	return;		/* skip modula2 symbol */
    if (name[0] == '_') {
	name = &name[1];
    }
#endif i386
    if (name[0] == '.' and name[1] == '_') {
	return;		/* skip Pascal alternate entry pts */
    } 
    if (not streq(name, "end")) {
	nlhdr.nfiles = filep - filetab;
	file = srcfilename(np->n_value + n_value_offset);
	/* Ignore the funny global emitted for Pascal labels */
#ifdef i386
        if ((file != nil) &&
	    (name != nil) &&
	    (name[0] == '.') && (name[1] == 'L') && (name[2] == 'G')) {
		p = rindex(file, '.');
		if (p and (findlanguage(p) == findlanguage(".p"))) {
			return;
		}
        }
#else
        if ((file != nil) &&
	    (name != nil) &&
	    (name[0] == 'L') && (name[1] == 'G')) {
		p = rindex(file, '.');
		if (p and (findlanguage(p) == findlanguage(".p"))) {
			return;
		}
        }
#endif
        if (file != nil) {
			p = rindex(file, '.');
			if (p and (findlanguage(p) == findlanguage(".f")))
				strip_us(name);
        }
	n = identname(name, true);
	if (np->n_type == (N_TEXT | N_EXT)) {
	    find(t, n) where
		t->class == FUNC and t->block->class == MODUS
	    endfind(t);
	    if (t != nil)
		return;
	    find(t, n) where
		(char)t->level == (char)program->level and
		(t->class == PROC or t->class == FUNC)
	    endfind(t);
	    if (t == nil) {
		    find(t, n) where
		    	t->class == VAR
		    endfind(t);
		    if (t != NULL) {
			    goto var_in_text;
		    }
	    }
	    if (t == nil) {
		t = insert(n);
		t->language = findlanguage(".s");
		t->class = FUNC;
		t->type = t_int;
		t->block = curblock;
		t->level = program->level;
		t->symvalue.funcv.src = false;
		t->symvalue.funcv.inline = false;
	    } else if ((t->block)->class == PROG) {
		t->block = curblock; 	/* (Pascal non -g'd outer proc) */
	    }
	    if (strcmp(name, "_sigtramp") == 0) {
		sigtramp_Name = t->name;
	    }
	    t->symvalue.funcv.beginaddr = np->n_value + n_value_offset;
	    newfunc(t, codeloc(t));
	    findbeginning(t);
	} else if ((np->n_type & N_TYPE) != N_TEXT) {
	var_in_text:
	    find(t, n) where
		t->class == COMMON
	    endfind(t);
	    if (t != nil) {
		u = (Symbol) t->symvalue.common.offset;
		while (u != nil) {
		    u->symvalue.offset = u->symvalue.common.offset+np->n_value;
		    u = u->symvalue.common.chain;
		}
            } else {
		check_var(np, n, n_value_offset);
	    }
	}
    }
}

/*
 * Check to see if a namelist entry refers to a variable.
 * If not, create a variable for the entry.  In any case,
 * set the offset of the variable according to the value field
 * in the entry.
 */

public check_var(np, n, n_value_offset)
struct nlist *np;
register Name n;
int n_value_offset;
{
    register Symbol t;

    find(t, n) where
	t->class == VAR and (char)t->level == (char)program->level
    endfind(t);
    if (t == nil) {
	t = insert(n);
	t->language = findlanguage(".s");
	t->class = VAR;
	t->type = t_int;
	t->level = program->level;
	t->block = curblock;
    }
    t->symvalue.offset = np->n_value + n_value_offset;
}

/*
 * See if the symbol table entry for this nested routine has already been
 * built.  If it has, all parent nodes will be in place.
 */

private Boolean already_built(n, s)
String n;
Symbol s;
{
    register Symbol block;
    register unsigned int plevel;
    String unqual_name;
    char name[200];

    strcpy(name, n);
    unqual_name = rindex(name, '.');	/* get rid of last qualifier */
    *unqual_name = '\0';
    unqual_name = name;
    plevel = 1;		/* what the immediate parent's level should be */
    while (*unqual_name != '\0') {
		if (*unqual_name++ == '.') {
			plevel = (plevel == 1) ? 4 : (plevel + 1);
		}
    }
    block = s->block;
    while (plevel >= 4) {	/* 4 is the level of a rtn nested 1 deep */
		unqual_name = rindex(name, '.');
		*unqual_name++ = '\0';
		if (block->name == nil) {
			return false;
		}
		if ((block->level == plevel) and
			streq(unqual_name, (block->name)->identifier) ) {
			block = block->block;
			plevel -= 1;
		} else {
			return false;
		}
    }					/* now see if the root is correct */
    if ((block->block->block == program) and (block->class == FUNC) and
	  (strcmp(name, (block->name)->identifier) == 0)) {
		return true;
    } else {
		return false;
    }
}

/*
 * Check to see if a local name is known in the current scope.
 * If not then enter it.
 */

public check_local(name, np, n_value_offset)
String name;
register struct nlist *np;
int n_value_offset;
{
    register Name n;
    register Symbol t, cur;
    String unqual_name;
    Symbol parent, pblock;
    char *ch;
#ifdef i386
    char msgbuf[300];
#endif i386

    if ((np->n_type & N_TYPE) == N_TEXT) {
		if (name[0] == '.' and name[1] == '_') {
			return;	/* skip Pascal alternate entry pts */
		} 

		/* nested Pascal routine */
    	if ((ch = index(name, '.')) != nil) {
#ifdef i386
			/*
			 *	check for symbols of form _abcdefghijk.o, which may be truncated
			 *	the _ has already been truncated by the time we reach here
			 */
			if ((strlen(name) == 13) && (*(++ch) == 'o') && (*(++ch) == '\0')) {
				sprintf(msgbuf,
					"Symbol _%s, taken as pascal nested routine,\n", name);
				strcat(msgbuf,
					"\tbut may really be front-truncated object file name?\n");
				strcat(msgbuf,
					"\tObject file symbol limit is 14 chars, including .o\n");
				warning(msgbuf);
			}
#endif i386
			unqual_name = rindex(name, '.') + 1;
			n = identname(unqual_name, true);
			cur = lookup(n);			/* see if already built */
			while (cur != nil) {
				if ((cur->name == n) and
				  (cur->class == FUNC or cur->class == PROC)) {
					if (already_built(name, cur)) {
						cur->symvalue.funcv.beginaddr = np->n_value + n_value_offset;
						newfunc(cur, codeloc(cur));
						findbeginning(cur);
						return;			/* RETURN */
					}
				}
				cur = cur->next_sym;
			}
			t = insert(n);			/* not found */
			t->language = findlanguage(".s");
			t->class = FUNC;
			t->type = t_int;
			t->symvalue.funcv.src = false;
			t->symvalue.funcv.inline = false;
			t->symvalue.funcv.beginaddr = np->n_value + n_value_offset;
			newfunc(t, codeloc(t));
			findbeginning(t);
			unqual_name = index(name, '.');
			if (unqual_name==nil)
			    panic("missing '.' at %s %s", name, 
					where_in_obj());
			*unqual_name = '\0';
			n = identname(name, true);

			/*
			 * Find remotest ancestor.
			 * We treat it a bit specially since it can be 
			 * either a global proc/function or
			 * a "private" proc/function
			 */
			find(parent, n) where
			(   (char)parent->level == (char)program->level
			 or (char)parent->level == (char)curmodule->level + 1
			)
			and (parent->class == PROC or parent->class == FUNC)
			endfind(parent);

			if (parent == nil) {
				/* make a parent entry */
				n = identname(name, true);
				parent = insert(n);
				parent->language = findlanguage(".s");
				parent->class = FUNC;
				parent->type = t_int;
				parent->symvalue.funcv.src = false;
				parent->symvalue.funcv.inline = false;

				/* see comment in 'entersym()' --- ivan */
				parent->block = curmodule;
				parent->level = curmodule->level + 1;
			}
			while (index(++unqual_name, '.') != nil) {
				name = unqual_name;
				pblock = parent;
				unqual_name = index(name, '.');
				if (unqual_name==nil)
				    panic("missing '.' at %s %s", 
						name, where_in_obj());
				*unqual_name = '\0';
				n = identname(name, true);
				find(parent, n) where
					parent->block == pblock and
					(parent->class == PROC or parent->class == FUNC)
				endfind(parent);
				if (parent == nil) {
					parent = insert(n);
					parent->language = findlanguage(".s");
					parent->class = FUNC;
					parent->type = t_int;
					parent->symvalue.funcv.src = false;
					parent->symvalue.funcv.inline = false;
					parent->block = pblock;
					if (pblock->level == program->level) {
						parent->level = 4;
					} else {
						parent->level = pblock->level + 1;
					}
				}
			}
			t->block = parent;
			t->level =
				  (parent->level == program->level ? 4 : parent->level + 1);
			return;
    	}
		cur = curmodule;
    } else {
		cur = curblock;
    }
    n = identname(name, true);
    find(t, n) where t->block == cur endfind(t);

#ifdef UNDEF
	ivan		Wed Jan 25 16:20:44 PST 1989

	The following test has been taken out because it fixes the globals
	vs file static scope resolution problem.

	First, how does the fix work: 
	Consider 'x' which is global and has been declared 'static' in some file.
	When we get to the static version, the above 'find()' tries to find a 
	duplicate in the current module. If it fails, the below code looks for
	'x' in the global (whole program) scope. This is obviosuly the wrong thing
	to do. Taking it out certainly fixes the bug.


	Second, how come I know I haven't broken something.
		
		Sice the consequent of the below test adjusts something, it is 
		some sort of fixup. It is hard to determine what is being fixed up.
		There are no obviosu candidates.

		Simple tests indicated that I hadn't broken anything but that is no
		guarantee.

		I also reason as follows. Taking it out might force the
		creation of extra symbol entries below, but that isn't
		as bad as not creating one ...  hopefully one can
		always use scope resolution to get around the
		problem.

		Also the fixup doesn't pertain to function start
		addresses so bugs of the form "can't place bkt at 0x0"
		won't occur.

	IN CASE YOU FEEL THE URGE TO PUT THIS TEST BACK IN
	DON'T! It's not the right solution.


	if (t == nil){
		find(t, n) where 
			(char)t->level == program->level and t->class == FUNC 
		endfind(t); 
		if(t != nil){
			t->block = cur;
			t->level = cur->level+1;
		}
	}
#endif

    if (t == nil) {
		t = insert(n);
		t->language = findlanguage(".s");
		t->type = t_int;
		t->block = cur;
		t->level = cur->level;
		if ((np->n_type & N_TYPE) == N_TEXT) {
			t->class = FUNC;
			t->symvalue.funcv.src = false;
			t->symvalue.funcv.inline = false;
			t->symvalue.funcv.beginaddr = np->n_value + n_value_offset;
			newfunc(t, codeloc(t));
			findbeginning(t);
		} else {
			t->class = VAR;
			t->symvalue.offset = np->n_value + n_value_offset;
		}
    } else {
		/*
		 *	Test for the existence of 'beginaddr', because
		 * 	a 'minimal' entry might have been created by the above code for
		 *	pascal. For instance foo.bar would've created a minimal 'foo'
		 *	because 'bar' needs to have its 'block' field point to 
		 *	something valid. Unfortunately that minimal symbol for foo
		 *	doesn't have the proper address info. We fix it here.
		 */
		if ((np->n_type & N_TYPE) == N_TEXT and ! t->symvalue.funcv.beginaddr ){
			t->symvalue.funcv.beginaddr = np->n_value + n_value_offset;
			newfunc(t, codeloc(t));
			findbeginning(t);
		}
	}
}

/*
 * Check to see if a symbol corresponds to a object file name.
 * For some reason these are listed as in the text segment.
 * If the name does not end in '.o' (a filename), then assume
 * it is a function name probably from an assembler file
 * because it does not begin with an '_'.
 */

public check_filename(name, np, n_value_offset)
String name;
struct nlist *np;
int n_value_offset;
{
    register String mname;
    register Integer i;
    register Symbol s;

    i = strlen(name) - 2;
    if (i >= 0 and name[i] == '.' and name[i+1] == 'o') {
	nobj_seen++;
        mname = strdup(name);
	mname[i] = '\0';
	--i;
	while (mname[i] != '/' and i >= 0) {
	    --i;
	}
	s = insert(identname(&mname[i+1], true));
	s->language = findlanguage(".s");
	s->class = MODULE;
	s->symvalue.funcv.beginaddr = codestart;
	s->symvalue.funcv.inline = false;
	s->symvalue.funcv.outofdate = false;
/*	findbeginning(s); */
	while (curblock->class != PROG) {
	    exitblock();
	}
	enterblock(s);
	curmodule = s;
	compile_dir = nil;
	ignore_debug_info = is_sources_sel_list_active() 
			&& !isin_sources_sel_list(name);
	if(ignore_debug_info == false) nobj_used++;
    } else {
		Symbol t;
		Name n;
		n = identname(name, false);
		find(t, n) where 
	    	t->class == FUNC  or t->class == PROC  
		endfind(t); 
		if(t == nil )
			check_local(name, np, n_value_offset);
    }
}

/*
 * Check to see if a symbol is about to be defined within an unnamed block.
 * If this happens, we create a procedure for the unnamed block, make it
 * "inline" so that tracebacks don't associate an activation record with it,
 * and enter it into the function table so that it will be detected
 * by "whatblock".
 */

/*****************************************************
private unnamed_block()
{
    register Symbol s;
    static int bnum = 0;
    char buf[100];

    ++bnum;
    sprintf(buf, "$b%d", bnum);
    s = insert(identname(buf, false));
    s->class = PROG;
    s->symvalue.funcv.src = false;
    s->symvalue.funcv.inline = true;
    s->symvalue.funcv.beginaddr = addrstk[nesting];
    enterblock(s);
    newfunc(s, addrstk[nesting]);
    addrstk[nesting] = NOADDR;
}
*****************************************************/

/*
 * Compilation unit.  C associates scope with filenames
 * so we treat them as "modules".  The filename without
 * the suffix is used for the module name.
 *
 * Because there is no explicit "end-of-block" mark in
 * the object file, we must exit blocks for the current
 * procedure and module.
 */
char mine[100] = "../common/getshlib.c";
private enterSourceModule(n, addr)
Name n;
Address addr;
{
    register Symbol s;
    Name nn;
    String mname, suffix;
    struct Modnode *m;

    mname = strdup(idnt(n));
	if(strcmp(mname, mine) == 0)
		mname = mname;
    if (rindex(mname, '/') != nil) {
	mname = rindex(mname, '/') + 1;
    }
    suffix = rindex(mname, '.');
    curlang = findlanguage(suffix);
    if (curlang == findlanguage(".p")) {
	pascal_module = true;
	pascal_pgm = true;
    } else {
	pascal_module = false;
    }
    if (curlang == findlanguage(".f")) {
	strip_ = true;
    } 
    if (curlang == findlanguage(".mod")) {
	modula2_pgm = true;
	for ( m = modlist; m != nil; m = m->link ) {
		m->unitno = -1;
	}
    }
    if (suffix != nil) {
	*suffix = '\0';
    }
    while (curblock->class != PROG) {
	exitblock();
    }
    nn = identname(mname, true);
    if (curmodule == nil  and  curmodule->name != nn) {
	/* This is a new module */
	s = insert(nn);
	s->class = MODULE;
	s->symvalue.funcv.beginaddr = codestart;
	s->symvalue.funcv.inline = false;
	s->symvalue.funcv.outofdate = false;
    } else {
	s = curmodule;
    }
    s->language = curlang;
    enterblock(s);
    curmodule = s;
    if (program->language == nil) {
	program->language = curlang;
    }
    warned = false;
	enterfile(n, addr);
    bzero(typetable, sizeof(typetable));
    bzero(scopetable, sizeof(scopetable));
    if (curfunc == nil) {
	setcurfunc(s);		/* first module on the load list */
	endfunc = s;
    }
}

/*
 * Put an nlist into the symbol table.
 * If it's already there just add the associated information.
 *
 * Type information is encoded in the name following a ":".
 */

private Symbol constype();
private Char *curchar;
private struct Modnode *getmod();
private Symbol getit();

private char msgstring[800];	/* for sprintf'ing a long error message */
#define skipchar(ptr, ch) { \
    if (*ptr != ch) { \
	objscan_errmsg(msgstring, ptr, ch); \
	panic_and_where(msgstring); \
    } \
    ++ptr; \
}

/* This routine provides complete instructions to the user on what to do */
/* when dbx gives up because of corrupt debug symbol information         */
/* This expands upon the previous cryptic error message.                 */

private objscan_errmsg(msgstring, ptr, ch)
char *msgstring;
char *ptr;
char ch;
{
	char c1[10];
	char c2[10];

	c1[0] = ch;
	c1[1] = 0;
	if (c1[0] == 0) {
		strcpy(c1, "<nul>");
	}
	c2[0] = *ptr;
	c2[1] = 0;
	if (c2[0] == 0) {
		strcpy(c2, "<nul>");
	}
	sprintf(msgstring,
		"\n%s\n%s '%s', %s '%s'.\n\n%s%s%s%s%s%s'%.16s'.%s%s",

		"Your executable file contains corrupt symbol information:",
		"The debugger expected to find char", c1, "but found char", c2,

		"This problem originated in compilation, assembly, archiving, or linking.",
		"\nRebuilding your program often clears this problem.\n",

		"\nTo investigate further, try shell command nm(2) to see symbols:\n",
		"   nm -opa yourprog >somefile\n",
		"'yourprog' is your executable file or object file or archive.\n",
		"Edit somefile, and search for ", ptr,

		"\n\nDebugger unable to proceed.\n",
		"recompile, reassemble, rearchive, or relink, and try again.\n"
	);
}

private entersym(str, npp)
String str;
struct nlist **npp;
{
    register Symbol s, r;
    register char *p;
    register int c;
    register Name n;
    Boolean knowtype, isnew;
    Symclass class;
    Integer newlevel;
    Symbol parent, pblock;
    String unqual_name;
    int	fileindex;
    int typeindex;
    struct Modnode *m;
    int unitno;
    int scopeno;
    Symbol ss;

    p = index(str, ':');
    if (p==nil)
	panic("missing ':' at %s %s", str, where_in_obj());
    *p = '\0';
    c = *(p+1);

    if (pascal_module and (rindex(str, '.') != nil) and
      ((c == 'f') or (c == 'X'))) {
	isnew = false;			/* Nested Pascal routine */
	/* going to find the parent right above the suffix entry */
        unqual_name = index(str, '.');
	if (unqual_name==nil)
	    panic("missing '.' at %s %s", str, where_in_obj());
        *unqual_name++ = '\0';
        n = identname(str, true);

	/*
	 * Find remotest ancestor -- either global proc/function or
	 * "private" proc/function.  At this point, we've seen the
	 * DBX symbol for the parent; if it is a "private" proc/func,
	 * its level has been set to curmodule->level +1.
	 */
	find(parent, n) where
	    (   (char)parent->level == (char)program->level
	     or (char)parent->level == (char)curmodule->level +1
	    )
	    and (parent->class == PROC or parent->class == FUNC)
	endfind(parent);
 	if (parent == nil) {
	    panic_and_where("entersym: nested Pascal proc error");
	}
	while (index(unqual_name, '.') != nil) {
	    str = unqual_name;
	    pblock = parent;
	    unqual_name = index(str, '.');
	    if (unqual_name==nil)
		panic("missing '.' at %s %s", str, where_in_obj());
	    *unqual_name++ = '\0';
	    n = identname(str, true);
	    find(parent, n) where
		parent->block == pblock and
		(parent->class == PROC or parent->class == FUNC)
	    endfind(parent);
	    if (parent == nil) {
		panic_and_where("entersym nested routine error");
	    }
	}
	str = unqual_name;
	n = identname(str, true);
	find(s, n) where
	  s->block == parent and
	  (s->class == PROC or s->class == FUNC)
	endfind(s);
	if (s == nil) {
#ifdef i386
		/* 
		 * ivan Thu May 25 1989 1020219
		 * The original code assumes that the non-stab symtab entry 
		 * occurs before the stab, whereas on the 386 that's not the
		 * case. So we install a stub.
		 */
		s = insert(n);

		s->class = VAR;
		s->block = curblock;
		s->level = curlevel;
#else
		panic("entersym(2): nested Pascal proc error for '%s' %s", 
			str, where_in_obj());
#endif
	}
    } else {
		if (*str == '\0') {
			n == nil;
		} else  {
			n = identname(str, true);
		}

		if (index("FfGVS", c) != nil) {
			Symbol blok;

			if (c == 'F' or c == 'f') {
				class = FUNC;
			} else {
				class = VAR;
			}

			if( c == 'f'  or  c == 'S' ) {
				blok = curmodule;
			} else {
				blok = program;
			}

			find(s, n) where
			  (char)s->level == (char)blok->level and s->class == class
			  and s->block == blok
			endfind(s);

			if (s == nil){
				find(s, n) where
				  (char)s->level == (char)blok->level+1 and s->class == class
				  and s->block == blok
				endfind(s);
			}

			/* try to find if there is any faked one */
			if (s == nil){
				find(s, n) where
					(char)s->level == (char)program->level and s->class == FUNC
				endfind(s);
			}

			/*
			 * one more attempt ...	ivan     Jan 23 1989
			 *
			 * In 'check_local()' things like 'foo.bar' are split apart and 
			 * stubs are made for 'foo'. Except that at the time there is no
			 * way to determine if 'foo' is a global or a private. Therefore
			 * 'check_local()' installs it as a local (level 3 with the file as 
			 * a parent).
			 * The following search attempts to find such an occurance. The code
			 * in the following switch adjusts things. The adjustment is
			 * somewhat illogical: foos level becomes 1 but it's parent is
			 * still a level 2 file symbol.
			 * Nevertheless this works!
			 */
			if (pascal_module and s == nil) {
				find (s,n) where
					(char)s->level == (char)3 and
					s->class == FUNC and 
					s->block == curmodule
				endfind(s);
			}

			if (s == nil) {
				isnew = true;
			} else {
				isnew = false;
			}
		} else if (c == 'E') {
			curchar = p + 2;
			if (curblock != scopetable[getint()]) {
				panic("sanity check failed on %s %s", str,
					    where_in_obj());
			}
			exitblock();
			return;
		} else {
			isnew = true;
		}
		/*
		 * Default attributes.
		 */
		if (isnew) {
			if (n == nil) 
				s = symbol_alloc();
			else
				s = insert(n);
		}

		/* These guesses are updated in the switch below */
		s->class = VAR;
		s->block = curblock;
		s->level = curlevel;
    }

/*****************************************************
    if (nesting > 0 and addrstk[nesting] != NOADDR) {
		unnamed_block();
    }
*****************************************************/

    s->language = (curlang == nil) ? findlanguage(".s") : curlang;
    curchar = p + 2;
    knowtype = false;
    switch (c) {
	case 't':	/* type name */
	    s->class = TYPE;
	    gettype_indices(&fileindex, &typeindex);
	    if (typeindex == 0) {
		panic("bad input on type \"%s\" at \"%s\" %s", symname(s),
		    curchar, where_in_obj());
	    }
	    /*
	     * A hack for C typedefs that don't create new types,
	     * e.g. typedef unsigned int Hashvalue;
	     *  or  typedef struct blah BLAH;
	     */
	    if (*curchar == '\0') {
		s->type = gettype(fileindex, typeindex);
		if (s->type == nil) {
		    s->type = symbol_alloc();
		    settype(fileindex, typeindex, s->type);
		}
		knowtype = true;
	    } else {
		if (gettype(fileindex, typeindex) == nil) {
		    settype(fileindex, typeindex, s);
		/*
		 * else it's a Pascal forward reference, e.g.
		 *	type idptr = ^id;
		 *	     id = record ... end;
		 * (we're processing type "id" and there's a placeholder
		 *  for it already in the type table.)
		 */
		} else {		/* Pascal forward reference */
		    if (curlang != findlanguage(".p") && 
			curlang != findlanguage(".mod")) {
			panic("forward reference error, fileindex %d typeindex %d, curchar \"%s\" %s",
			  fileindex, typeindex, curchar, where_in_obj());
		    }
		    r = gettype(fileindex, typeindex);
		    r->class = TYPE;
		    r->language = curlang;
		    r->block = s->block;
		    r->level = s->level;
		    r->symvalue.offset = s->symvalue.offset;
		    if (r->name != s->name) {
		    	insert_forw_ref(s->name, r);
		    }
		    if (n != nil) unhash(s);
		    s = r;
		}
		skipchar(curchar, '=');
	    }
	    break;

	case 'T': {	/* type id, e.g. struct s { ... }; */
	    Symbol type1;

	    s->class = TYPEID;
	    gettype_indices(&fileindex, &typeindex);
	    if (typeindex == 0) {
		panic("bad input on type id \"%s\" at \"%s\" %s", symname(s),
		    curchar, where_in_obj);
	    }
	    if ((type1 = gettype(fileindex, typeindex)) != nil) {
		unhash(s);
		if (type1->name != nil) {
		    unhash(type1);
		}
		*type1 = *s;
		insert_forw_ref(type1->name, type1);
		s = type1;
	    } else {
		settype(fileindex, typeindex, s);
	    }
	    skipchar(curchar, '=');
	    break;
	}

	case 'U':
	    s->class = IMPORT;
	    s->symvalue.importv.class = IMPORT;	/* import compilation unit */
	    unitno = getint();
	    s->symvalue.importv.unit = getmod(unitno);
	    knowtype = true;
	    break;

	case 'u':
	    s->class = IMPORT;
	    s->symvalue.importv.class = MODUS;	/* import from a modus */
	    unitno = getint();
	    s->symvalue.importv.unit = getmod(unitno);
	    knowtype = true;
	    break;

	case 'd':
	    s->class = IMPORT;
	    s->symvalue.importv.class = FUNC;	/* import from a scope */
	    s->symvalue.importv.unit = nil;
	    scopeno = getint();
	    /* if scope is known, then chain to the real */
	    s->chain = getit(s, scopeno);
	    knowtype = true;
	    break;

	case 'M':	/* enter modus */
	    s->class = MODUS;
	    enterblock(s);
	    s->level = (*npp)->n_desc;
	    scopeno = getint();
	    if (scopeno == 0) {		/* compiled unit */
		for ( m = modlist; m != nil; m = m->link ) 
		    if (m->unitno == 0) {	/* compiled unit */
			m->sym = s;
			break;
		    }
	    }
	    add_scptab(s, scopeno);
	    ss = insert(s->name);
            ss->class = FUNC;
	    ss->language = s->language;
	    ss->block = s;
	    ss->symvalue.funcv.beginaddr = (*npp)->n_value;
	    ss->level = (*npp)->n_desc;
	    ss->symvalue.funcv.inline = false;
	    ss->symvalue.funcv.src = true;
	    newfunc(ss, codeloc(ss));
	    findbeginning(ss);
	    knowtype = true;
	    break;

	case 'Q':	/* enter function and procedure */
	    s->class = FUNC;
	    enterblock(s);
	    s->level = (*npp)->n_desc;
	    s->symvalue.funcv.beginaddr = (*npp)->n_value;
	    add_scptab(s, getint());
	    skipchar(curchar, ';');

	    curparam = s;
	    s->symvalue.funcv.inline = false;
	    s->symvalue.funcv.src = false;
	    s->symvalue.funcv.beginaddr = (*npp)->n_value;
	    newfunc(s, codeloc(s));
	    findbeginning(s);

	    break;

	case 'F':	/* public function */
	case 'f':	/* private function */
	    s->class = FUNC;
	    if (pascal_module and c == 'f') {
		newlevel = (*npp)->n_desc;
		if (  (char)s->level != (char)newlevel
		  and (char)s->level != (char)curlevel ) {
		    /* level was set in check_local */
		    panic("level error in symbol entry for %s %s", 
			    symname(s), where_in_obj());
		}
		if (curlevel < newlevel) { /* entering a new level of nesting */
		    if ((curlevel + 1) != newlevel) {
			error("\nPascal symbol table error for %s %s", 
				    symname(s), where_in_obj());
		    }
		} else if (curlevel == newlevel) {
		    exitblock();
		} else {		/* backing out some layers */
		    if (newlevel < 3) { /* 1 = pgm, 2 = module, 3 = gbl proc */
			error("\nPascal symbol table error (level) for %s %s",
			    symname(s), where_in_obj());
		    }
		    while (curlevel >= newlevel) {
			exitblock();
		    }
		    s->level = newlevel;
		    s->block = curblock;
		}
	    } else  {
		while (curblock->class == FUNC or curblock->class == PROC) {
		    exitblock();
		}
	    }
	    enterblock(s);
	    if (c == 'F') {
		s->level = program->level;
		isnew = false;
	    }
	    curparam = s;
	    s->symvalue.funcv.inline = false;
	    s->symvalue.funcv.src = false;
	    if (isnew) {
		s->symvalue.funcv.beginaddr = (*npp)->n_value;
		newfunc(s, codeloc(s));
		findbeginning(s);
	    }
	    break;

	case 'G':	/* public variable */
	    s->level = program->level;
	    s->block = program;
	    break;

	case 'S':	/* a static global variable */
	    if (curlang == findlanguage(".mod")) {
			s->level = 2;	/* for isglobal() */
	    } else {
	        s->level = curmodule->level;
	        s->block = curmodule;
	    }
	    s->symvalue.offset = (*npp)->n_value;
	    break;

/*
 *  keep global BSS variables chained so can resolve when get the start
 *  of common; keep the list in order so f77 can display all vars in a COMMON
*/
	case 'V': /* own variable (C: static local var) (Fortran: common var) */
	    s->level = 2;
	    s->symvalue.offset = (*npp)->n_value;
	    if (curcomm) {
	      if (commchain != nil) {
 		  commchain->symvalue.common.chain = s;
	      }			  
	      else {
		  curcomm->symvalue.common.offset = (int) s;
	      }			  
              commchain = s;
              s->symvalue.common.offset = (*npp)->n_value;
              s->symvalue.common.chain = nil;
	    }
	    break;

	case 'r':	/* register variable */
	    s->level = -(s->level);
            s->symvalue.offset = (*npp)->n_value;
	    break;

	case 'p':	/* value parameter */
            s->symvalue.offset = (*npp)->n_value;
	    curparam->chain = s;
	    curparam = s;
	    /*
	     * There are two ST entries for a register parameter;
	     * the first is the register entry and the second is
	     * the stack entry.  The latter is needed for printing
	     * parameter info when the proc is entered, but this
	     * can be retrieved from the proc's chain and needn't
	     * be in the symbol table.
	     */
	    find(r, s->name) where
		s->block == r->block and (char)s->level == -(char)(r->level)
	    endfind(r);
	    if (r != nil) {
		unhash(s);
	    }
	    break;

	case 'v':	/* reference parameter */
	    /*
	     * Should this be handled like a regular register parameter 
	     * (as in case above), i.e. should we unhash the 
	     * stack ST entry for it? [No.]  REF class not handled correctly
	     * if it can be in a register too.
	     */
	    curparam->chain = s;
	    curparam = s;
	    /* Fall thru */
	case 'b':	/* based variables are almost like reference parameters */
	    s->class = ( c == 'v' ? REF : BASED );
	    s->symvalue.offset = (*npp)->n_value;
	    break;

	case 'i':	/* modula-2 dynamic array value parameter */
	case 'x':	/* Pascal conformant array value parameter */
	    s->class = CVALUE;
	    s->symvalue.offset = (*npp)->n_value;
	    curparam->chain = s;
	    curparam = s;
	    break;

	case 'X':	/* Pascal or Fortran function variable */
	    s->class = FVAR;
	    s->symvalue.funcv.offset = (*npp)->n_value;
	    break;

	case 'C':/* Pascal read-only param (dimension of a conformant array) */
	    s->class = READONLY;
	    s->symvalue.offset = (*npp)->n_value;
	    break;
	    
	default:	/* local variable */
	    --curchar;
	    s->symvalue.offset = (*npp)->n_value;
	    break;
    }
    if (not knowtype) {
	s->type = constype(nil, npp);
	if (s->class == TYPEID) {
	    addtypeid(s);
	}
    }
    if (tracesyms) {
	printdecl(s);
	fflush(stdout);
    }
}

/*
 * Get next symbol table element (this is continuation element) and
 * make sure the type is the same.  Then get new pointer into string
 * table.  Finally, update *npp so readsyms will be notified of change.
 */

contin_type_descrip(npp)
struct nlist **npp;
{
    register struct nlist *np;
    register int ix;
    unsigned char oldtype;

    np = *npp;
    oldtype = np->n_type;
    np++;
    if (oldtype != np->n_type) {
	panic_and_where("bad symbol table (type continuation char)");
    }
    ix = np->n_un.n_strx;
    if (ix == 0) {
	panic_and_where("bad string table (type continuation char)");
    }
    curchar = &stringtab[ix - 4];
    *npp = np;
}

/* use CONT_CHAR wherever it is legal to accept a backslash continuation */

#define CONT_CHAR ((*curchar == '\\')?contin_type_descrip(npp):0)

/* this could be:  (while ((*curchar == '\\') and (*(curchar+1) == '\0'))...) */

/*
 * Read in the fields of a record/structure
 */

readin_fields(u, npp, b)
register Symbol u;
struct nlist **npp;
Integer b;
{
    register Char *p;
    register Symbol prevu;
    register Name name;

    while (CONT_CHAR, *curchar != ';' and *curchar != '\0') {
        CONT_CHAR;		/* before fieldname */
	p = index(curchar, ':');
	if (p == nil) {
	    panic("index(\"%s\", ':') failed %s", curchar, where_in_obj());
	}
	*p = '\0';
	name = identname(curchar, true);
	u->chain = newSymbol(name, b, FIELD, nil, nil);
	curchar = p + 1;
	prevu = u;
	u = u->chain;
	u->language = curlang;
	u->type = constype(nil, npp);
	if ((u->type)->class == VARIANT) { /* Pascal variant part */
	    u->class = TAG;
	    if ((u->type)->symvalue.variant.tagfield == true) {
	        u->symvalue.tag.tagfield = prevu;
	    } else {
	        u->symvalue.tag.tagfield = nil;
	    }
	    u->symvalue.tag.tagcase = u->type;
	    u->type = (u->type)->symvalue.variant.tagtype;
	    skipchar(curchar, ',');
	    getint();
	    skipchar(curchar, ',');
	    u->symvalue.tag.varsize = getint();
	    skipchar(curchar, ';');
	    break;
	} else {
	    skipchar(curchar, ',');
	    u->symvalue.field.offset = getint();
	    skipchar(curchar, ',');
	    u->symvalue.field.length = getint();
	    skipchar(curchar, ';');
	}
    }
}

/*
 * Construct a type out of a string encoding.
 *	note: '\' indicate legal place to break string to new line.
 *	<#> ::== [-][0-9]*
 *	<name> ::== string of not one of: [:;,]
 *	<FieldList> ::== [\<name>:<type>,<#>,<#>\;]*
 * The forms of the string are
 *
 *	<#>
 *	<#>=<type>
 *	r<type>;[<rangetype>]<#>;[<rangetype>]<#>    $ subrange
 *	a<type>;<type>				     $ array <type> of <type>
 *	s<#><FieldList>[;]		     	     $ C structure
 *	u<#><FieldList>[;]	     		     $ union
 *	v<0|1>:<type>;[\<#>[,<#>]*:<FieldList>;]\;   $ variant
 *	s<#>[\<name>:<type>,<#>,<#>;]*\[;]	     $ C structure
 *	u<#>[\<name>:<type>,<#>,<#>;]*\[;]	     $ union
 *	v<0 or 1>:<type>;[\<#>[,<#>]*:[\<name>:<type>,<#>,<#>;]*;]\; $ variant
 *	S<type>					     $ set
 *	*<type>					     $ pointer
 *	f<type>				$ function parameter (C & Pascal)
 *	F<type>				$ function parameter (Fortran)
 *	P<type>				$ procedure parameter (Pascals
 *	L<type>				$ file type (Pascal)
 *	e[\<name>:<#>,\]*[;]		$ enum type
 *	Q<type>,<#>[,\<type>]*		$ function or procedure type
 *	I<type>				$ imported type
 *	q<type>(was v<type>)		$ reference parameter
 *	i<type>				$ cvalue parameter
 *	p<type>				$ value parameter
 * VMS & Apollo extensions ...
 *	b<type>				$ Fortran based variable
 *	y;<length>			$ Pascal varying string 
 *
 * (See papers "stab.info" and "pascal.stab.info" for more on this.)
 */

private Rangetype getrangetype();

private Symbol constype(type, npp)
Symbol type;
struct nlist **npp;
{
    register Symbol t, u;
    register Char *p;
    register Integer n;
    Integer b;
    Name name;
    Char class;
    Char *cp, *cp2;
    int	fileindex;
    int	typeindex;

    b = curlevel;
    if (isdigit(*curchar) || *curchar == '(') {
	gettype_indices(&fileindex, &typeindex);
	if (typeindex == 0) {
	    panic("bad type number at \"%s\" %s", curchar, where_in_obj());
	}
	if (*curchar == '=') {
	    if (gettype(fileindex, typeindex) != nil) {
		t = gettype(fileindex, typeindex);
	    } else {
		t = symbol_alloc();
		settype(fileindex, typeindex, t);
	    }
	    ++curchar;
	    constype(t, npp);
	} else {
	    t = gettype(fileindex, typeindex);
	    if (t == nil) {
		t = symbol_alloc();
		settype(fileindex, typeindex, t);
	    }
	}
    } else {
	if (type == nil) {
	    t = symbol_alloc();
	} else {
	    t = type;
	}
	t->language = curlang;
	t->level = b;
	t->block = curblock;
	class = *curchar++;
	switch (class) {
	    case 'r':
		t->class = RANGE;
		t->type = constype(nil, npp);
		skipchar(curchar, ';');
		t->symvalue.rangev.lowertype = getrangetype();
		t->symvalue.rangev.lower = getint();
		skipchar(curchar, ';');
		t->symvalue.rangev.uppertype = getrangetype();
		t->symvalue.rangev.upper = getint();
		if (curlang == findlanguage(".mod")) {
			t->symvalue.rangev.rngsize = (*npp)->n_desc;
		} else {
			t->symvalue.rangev.rngsize = 0;
		}
		break;

	    case 'C':			/* Pascal conformant array range */
		t->class = CRANGE;
		p = index(curchar, ',');
		if (p == nil) {
		    panic("conformant array: index(\"%s\", ':') failed %s",
		    curchar, where_in_obj());
		}
		*p = '\0';
        	name = identname(curchar, true);
	    	find(u, name) where
		    u->class == READONLY and u->block == curblock
	    	endfind(u);
		if (u == nil) {
		    panic_and_where("conformant arrays: error on subscript");
		}
		t->symvalue.rangev.lower = (long) u;
		curchar = ++p;
		p = index(curchar, ',');
		if (p == nil) {
		    panic("conformant array: index(\"%s\", ':') failed %s",
		    curchar, where_in_obj());
		}
		*p = '\0';
        	name = identname(curchar, true);
	    	find(u, name) where
		    u->class == READONLY and u->block == curblock
	    	endfind(u);
		if (u == nil) {
		    panic_and_where("conformant arrays: error on subscript");
		}
		t->symvalue.rangev.upper = (long) u;
		curchar = ++p;
		t->type = constype(nil, npp);
		break;

	    case 'a':
		t->class = ARRAY;
		t->chain = constype(nil, npp);
		skipchar(curchar, ';');
		t->type = constype(nil, npp);
		break;

	    case 'y':			/* Pascal varying string */
		skipchar(curchar, ';');
		t->class = VARYING;
		t->symvalue.varying.length = getint();
  		break;
  
	    case 'c':
		t->class = CARRAY;	/* Pascal conformant array */
		t->chain = constype(nil, npp);
		skipchar(curchar, ';');
		t->type = constype(nil, npp);
		break;

	    case 'S':
		t->class = SET;
		if (curlang == findlanguage(".mod")) {
			t->symvalue.setv.setsize = (*npp)->n_desc;
		} else {
			t->symvalue.setv.setsize = 0;
		}
		t->type = constype(nil, npp);
		t->symvalue.setv.startbit = 0;
		break;

	    case 'Q': {
		int paramno;
		int i;

		t->class = FUNCT;
		gettype_indices(&fileindex, &typeindex);
		if (typeindex == 0) {
			panic_and_where("return type of FUNCT is 0");
		}
		t->type = gettype(fileindex, typeindex);
		if (t->type == nil) {
			panic("return type %d of FUNCT is not defined %s", 
				typeindex, where_in_obj());
		}
		skipchar(curchar, ',');
		paramno = getint();
		u = t;
		for ( i = 0; i < paramno; i++ ) {
			skipchar(curchar, ';');
		        CONT_CHAR; 	/* before parameter type */
			/* distinguish variant and call by reference */
			if ( *curchar == 'v' ) *curchar = 'q'; 
			u->chain = constype(nil, npp);
			u = u->chain;
		}
		break;
	    }

	    case 'p':
		t->class = VAR;
		t->type = constype(nil, npp);
		break;

	    case 'q':
		t->class = REF;
		t->type = constype(nil, npp);
		break;

	    case 'i':
		t->class = CVALUE;
		t->type = constype(nil, npp);
		break;

	    case 'I':  {
		int unitno;

		t->class = IMPORT;
		t->symvalue.importv.class = MODUS;
		unitno = getint();
		t->symvalue.importv.unit = getmod(unitno);
		skipchar(curchar, ',');
		t->name = identname(curchar, true);
		break;
	    }

	    case 'L':
		t->class = FILET;
		t->type = constype(nil, npp);
		break;

	    case 's':
	    case 'u':
		t->class = (class == 's') ? RECORD : UNION;
		t->symvalue.offset = getint();
		u = t;
		readin_fields(u, npp, b);
		if (*curchar == ';') {
		    ++curchar;
		}
		break;

	    case 'x':
                if (t->class == BADUSE) {
		    t->class = FORWARD;
		    switch (*curchar++) {
		    case 's':
		        t->symvalue.forwardref.class = RECORD;
		        break;
		    case 'u':
		        t->symvalue.forwardref.class = UNION;
		        break;
		    case 'e':
		        t->symvalue.forwardref.class = SCAL;
		        break;
		    default:
		        panic("Bad type of forward reference, %c %s", 
				    *(curchar - 1), where_in_obj());
		    }
		    p = index(curchar, ':');
		    *p = '\0';
		    t->symvalue.forwardref.name = identname(curchar, true);
		    curchar = p + 1;
		} else {
		    curchar = index(curchar, ':') + 1;
		}
		break;

	    case 'v':
		t->class = VARIANT;
		n = getint(); /* tag designator: 1 means tagfield present */
		if (n == 1) {
		    t->symvalue.variant.tagfield = true;
		} else if (n == 0) {
		    t->symvalue.variant.tagfield = false; 
		} else {
		    panic_and_where("variant record type info bad");
		}
		skipchar(curchar, ':');
		t->symvalue.variant.tagtype = constype(nil, npp);
		skipchar(curchar, ';');
		CONT_CHAR;		/* before case */
		t->symvalue.variant.casevalue = getint();
		u = t;
		do {
		    cp = curchar;    		/* either points to ',' or ':' */
		    while (*curchar++ != ':');	/* get to first field */
		    readin_fields(u, npp, b);
		    u->type = u->chain;
		    u->chain = nil;
		    cp2 = ++curchar;   /* skip second ';' after field list */
		    curchar = cp;
		    while (*curchar == ',') {/* handle multiple tags for 1 case */
		    	curchar++;
		    	n = getint();
		    	u->chain = symbol_alloc();
		    	(u->chain)->type = u->type;
		    	(u->chain)->symvalue.variant.tagfield =
			  u->symvalue.variant.tagfield;
		    	(u->chain)->symvalue.variant.tagtype =
			  u->symvalue.variant.tagtype;
		    	u = u->chain;
		    	u->class = VARIANT;
		    	u->language = curlang;
		    	u->symvalue.variant.casevalue = n;
		    }
		    if (*curchar != ':') {
		    	panic_and_where("variant case error while reading symbols");
		    }
		    curchar = cp2;
		    CONT_CHAR;		/* end of case */
		    if (*curchar != ';') { /* end of variant? */
		    	n = getint();
		    	u->chain = symbol_alloc();
		    	(u->chain)->symvalue.variant.tagfield =
			  u->symvalue.variant.tagfield;
		    	(u->chain)->symvalue.variant.tagtype =
			  u->symvalue.variant.tagtype;
		    	u = u->chain;
		    	u->class = VARIANT;
		    	u->language = curlang;
		    	u->symvalue.variant.casevalue = n;
		    }
		} while (*curchar != ';');
		++curchar;		/* end of variant semi */
		break;
		    
	    case 'e':
		t->class = SCAL;
		if (curlang == findlanguage(".mod")) {
			t->symvalue.enumv.enumsize = (*npp)->n_desc;
		} else {
			t->symvalue.enumv.enumsize = 0;
		}
		u = t;
		if (curchar == nil)
		    panic_and_where("curchar==nil when processing enumeration type");
		while (CONT_CHAR, *curchar != ';' and *curchar != '\0') {
		    CONT_CHAR;		/* before enum name */
		    p = index(curchar, ':');
		    if (p == nil)
			panic("unexpected end processing enumeration type at %s %s",
				curchar, where_in_obj());
		    *p = '\0';
		    u->chain = insert(identname(curchar, true));
		    curchar = p + 1;
		    u = u->chain;
		    u->language = curlang;
		    u->class = CONST;
		    u->level = b;
		    u->block = curblock;
		    u->type = t;
		    u->symvalue.iconval = getint();
		    skipchar(curchar, ',');
		}
		if (*curchar == ';') {
		    ++curchar;
		}
		break;

	    case '*':
		t->class = PTR;
		t->type = constype(nil, npp);
		break;

	    case 'f':
		t->class = FUNC;
		t->type = constype(nil, npp);
		break;

	    case 'F':
		t->class = FFUNC;
		t->type = constype(nil, npp);
		break;

	    case 'P':
		t->class = FPROC;
		t->type = constype(nil, npp);
		break;

	    default:
		badcaseval(class);
	}
    }
    return t;
}

/*
 * Get a range type.
 *
 * Special letters indicate a dynamic bound, i.e. what follows
 * is the offset from the fp which contains the bound.
 * J is a special flag to handle fortran a(*) bounds.
 */

private Rangetype getrangetype()
{
    Rangetype t;

    switch (*curchar) {
	case 'A':
	    t = R_ARG;
	    curchar++;
	    break;

	case 'T':
	    t = R_TEMP;
	    curchar++;
	    break;

	case 'J': 
	    t = R_ADJUST;
	    curchar++;
	    break;

	default:
	    t = R_CONST;
	    break;
    }
    return t;
}

/*
 * Read an integer from the current position in the type string.
 */

private Integer getint()
{
    register Integer n;
    register char *p;
    register Boolean isneg;

    n = 0;
    p = curchar;
    if (*p == '-') {
	isneg = true;
	++p;
    } else {
	isneg = false;
    }
    while (isdigit(*p)) {
	n = 10*n + (*p - '0');
	++p;
    }
    curchar = p;
    return isneg ? (-n) : n;
}

/*
 * Add a type id name, e.g. in the following, "s" and "u" are type id's
 *	struct s {
 *	    ...
 *	    union u {
 *		...
 *	    }
 *	};
 * This is a kludge to be able to refer to type id's that have the same name
 * as some other symbol in the same block.
 */

private addtypeid(s)
register Symbol s;
{
    register Symbol t;
    char buf[100];

    sprintf(buf, "$$%.90s", idnt(s->name));
    t = insert(identname(buf, false));
    t->language = s->language;
    t->class = TYPEID;
    t->type = s->type;
    t->block = s->block;
}

/*
 * Allocate file and line tables and initialize indices.
 */

private allocmaps(nf, nl)
Integer nf, nl;
{
    if (filetab != nil) {
	dispose(filetab);
    }
    if (linetab != nil) {
	linetab--;
	dispose(linetab);
    }
    filetab = newarr(Filetab, nf);
    if (filetab == 0)
	fatal("No memory available. Out of swap space?");

    /*

    ivan 27 Mar 1989    ( A 6 year old bug)

    If you look in 'enterline()' you'll see that there is a 'linep-1' in there
    Which is completely preposterous. In the case of the first call to
    'enterline()', 'linep-1' is actually pointing to 'linep[-1]', and in some
    rare cases a line _will_ be allocated there. We end up with the first line
    map entry being allocated in 'linep[-1]'. 
    Instead of adding extra checks to 'enterline()' we "pad" 'linetab' as
    follows. Hopefully the addr=0,line=0 combination will always satisfy
    the check leading to the 'lp++' in 'enterline()'.

    Note the -1 in the linetab disposal above.

    linetab = newarr(Linetab, nl);

    */

    linetab = newarr(Linetab, nl+1);
    if (linetab == 0)
	fatal("No memory available. Out of swap space?");
    linetab->addr = 0;
    linetab->line = 0;
    linetab++;


    filep = filetab;
    linep = linetab;
}

/*
 * Add a file to the file table.  Note that a partial version
 * of this may be done in enterSourceModule.
 *
 * Because the SLINEs may come before the N_SOurce file .stab,
 * we may provisionally enter a "foo.o" file, and then later
 * change it to be a "for_real" file (when we get the N_SO for
 * it).  However, we need to wipe out not-for_real files that
 * don't get N_SO .stabs.
 */

private Linetab *oldlinep = (Linetab *) 0;

private enterfile(filename, addr)
Name filename;
Address addr;
{
    if (addr != curfaddr or oldlinep == linep) {
	filep->addr = addr;
	filep->filename = idnt(filename);
	filep->modname = strdup(idnt(curmodule->name));
	filep->lineindex = linep - linetab;
	++filep;
	curfaddr = addr;
    }
    oldlinep = linep;
}


/*
 * Since we only estimated the number of lines (and it was a poor
 * estimation) and since we need to know the exact number of lines
 * to do a binary search, we set it when we're done.
 */

public setnlines()
{
    nlhdr.nlines = linep - linetab;
}

/*
 * Similarly for nfiles ...
 */

public setnfiles()
{
    nlhdr.nfiles = filep - filetab;
}

#define INCL_HASH	64		/* Size of include file hash table */
#define NINCL_STACK	30		/* Size of header file stack */
#define TYPE_INCR	20		/* Amount to grow type tables by */
#define FILE_INCR	10		/* Amount to grow file table by */

typedef struct Incl *Incl;
struct Incl {
    char *name;				/* Header file name */
    int checksum;			/* Checksum for header file */
    Symbol *typetable;			/* Types generated by header file */
    int tablesize;			/* Size of type table */
    Incl next;				/* Linked list for hashing */
};

private Incl incl_stack[NINCL_STACK];	/* Header file stack */
private Incl *stkp = incl_stack;	/* Stack pointer */
private Incl incl_table[INCL_HASH];	/* Lookaside table */
private Incl cur_incl;			/* Current header file */
private Incl *file_table;		/* Active file table */
private int nfiles;			/* Size of file_table */
private int nextfile;			/* Index for next header file */

/*
 * Entering a new source file.
 * Enter it as the first file in the active file table.
 * The stack integrity test uses 1, because the previous
 * source file was never exited.
 */
private new_srcfile(name, np)
char *name;
struct nlist *np;
{
    if (file_table == nil) {
	file_table = (Incl *) malloc(FILE_INCR * sizeof(Incl));
	if (file_table == 0)
		fatal("No memory available. Out of swap space?");
        bzero((char *) file_table, FILE_INCR * sizeof(Incl));
	nfiles = FILE_INCR;
    }
    nextfile = 0;
    cur_incl = nil;
    stkp = incl_stack;
    begin_incl(name, np);
}

/*
 * Begin an include file.
 * Add it into the current file table.
 * The file table may need to be expanded.
 */
private begin_incl(name, np)
char *name;
struct nlist *np;
{
    /* 880817 PEB added include file nesting limit check (bug 1011943) */
    if (stkp - incl_stack >= NINCL_STACK - 1) 
	fatal("include files nested too deep (%d max)", NINCL_STACK);
    *stkp++ = cur_incl;
    cur_incl = new(Incl);
    if (cur_incl == 0)
	fatal("No memory available. Out of swap space?");
    cur_incl->name = name;
    cur_incl->checksum = np->n_value;
    cur_incl->typetable = (Symbol *) malloc(TYPE_INCR * sizeof(Symbol));
    if (cur_incl->typetable == 0)
	fatal("No memory available. Out of swap space?");
    cur_incl->tablesize = TYPE_INCR;
    cur_incl->next = nil;
    bzero((char *) cur_incl->typetable, TYPE_INCR * sizeof(Symbol));
    insert_file(cur_incl);
}

/*
 * Insert a file into the active file table.
 */
private insert_file(ip)
Incl ip;
{
    if (nextfile >= nfiles) {
	nfiles += FILE_INCR;
	file_table = (Incl *) realloc(file_table, nfiles * sizeof(Incl));
	if (file_table == 0)
		fatal("No memory available. Out of swap space?");
	bzero((char *) &file_table[nfiles - FILE_INCR], 
	  FILE_INCR * sizeof(Incl));
    }
    file_table[nextfile] = ip;
    nextfile++;
}

/*
 * End an include file.
 * Put it in the lookaside hash table, and
 * pop one off the stack.
 */
private end_incl()
{
    int h;
    
    h = hash(cur_incl->name);
    cur_incl->next = incl_table[h];
    incl_table[h] = cur_incl;
    cur_incl = *--stkp;
}

/*
 * We have an excluded header file.
 * Find it in the lookaside file table and insert it into
 * the active file table.  If not found, complain unless it's
 * already active (e.g., a lexically recursive header file).
 */
private excl_incl(name, np)
char *name;
struct nlist *np;
{
    Incl ip;
    int h, nf;

    h = hash(name);
    for (ip = incl_table[h]; ip != nil; ip = ip->next) {
	if (streq(name, ip->name) && np->n_value == ip->checksum) {
	    insert_file(ip);    
	    return;
	}
    }
    for( nf = nextfile-1; nf >= 0; --nf ) {
	ip= file_table[nf];
	if (streq(name, ip->name) && np->n_value == ip->checksum) {
	    insert_file(ip);
	    return;
	}
    }
    error("Did not find excluded file %s in lookaside table %s", name,
	    where_in_obj());
}

/*
 * Compute a hash from a string.
 * Simply sum the characters and mod.
 */
private int hash(str)
char    *str;
{
    int     sum;
    char    *cp;

    sum = 0;
    for (cp = str; *cp; cp++) {
	sum += *cp;
    }
    return(sum % INCL_HASH);
}

private add_modlist(name, unitno, key)
Name name;
short unitno;
char *key;
{
    struct Modnode *m;

    for (m = modlist; m != nil; m = m->link) {
	if (strncmp(m->key, key, 24) == 0) {
	    m->unitno = unitno;
	    return;
	}
    }
    m = (struct Modnode *)malloc(sizeof(struct Modnode));
    if (m == 0)
	fatal("No memory available. Out of swap space?");
    m->name = name;
    m->unitno = unitno;
    strncpy(m->key, key, 24);
    m->sym = nil;
    m->link = modlist;
    modlist = m;
}

private struct Modnode *getmod(unitno)
int unitno;
{
    struct Modnode *m;

    for (m = modlist; m != nil; m = m->link) {
	if (m->unitno == unitno) return m;
    }
    panic("modus %d is not defined yet %s", unitno, where_in_obj());
}

private Symbol getit(s, scopeno)
Symbol s;
int scopeno;
{
    Symbol t;

    if (scopeno < 0 or scopeno >= NSCOPE) {
	panic("scopeno %d is out of range %s", scopeno, where_in_obj());
    }
    if (scopetable[scopeno] == nil) return nil;
    find(t, s->name) where
	t->block == scopetable[scopeno] and
	t->class != FIELD and
	t->class != TYPEID
    endfind(t);
    return t;
}


private add_scptab(s, scopeno)
Symbol s;
int scopeno;
{
    if (scopeno < 0 or scopeno >= NSCOPE) {
	panic("scopeno %d is out of range %s", scopeno, where_in_obj());
    }
    scopetable[scopeno] = s;
}

/*
 * Read type indices from the current position.
 * There is a new and an old format for type indices.
 * The old format is a simple integer, the new format
 * is "(f,t)" where f is the file index and t is the 
 * type index.  The old format is converted into the new
 * format by setting f to -1.
 */
private	gettype_indices(filp, typep)
int *filp;
int *typep;
{
    if (*curchar == '(') {
	curchar++;
	*filp = getint();
	if (*curchar != ',') {
	    error("bogus type info %s", where_in_obj());
	}
	curchar++;
	*typep = getint();
	if (*curchar != ')') {
	    error("bogus type info %s", where_in_obj());
	}
	curchar++;
    } else {
	*filp = -1;
	*typep = getint();
    }
}

/*
 * Given file and type indices, return the symbol
 * associated with these values.
 */
private Symbol gettype(filei, typei)
int filei;
int typei;
{
    Incl file;

    if (filei == -1) {
	if (typei >= NTYPES) {
	    fatal("gettype: type index too large in object file symbol info: fileindex %d, typeindex %d; at \"%s\"; Compiler or archiver error.", filei, typei, curchar);
	}
	return(typetable[typei]);
    } else {
	if (filei >= nextfile) {
	    error("gettype: file index too large, fileindex %d, nextfile %d; at \"%s\", Compiler or archiver error. %s", 
	      filei, nextfile, curchar, where_in_obj());
	}
	file = file_table[filei];
	while (typei >= file->tablesize) {
	    enlarge_typetable(file);
	}
	return(file->typetable[typei]);
    }
}

/*
 * Given file and type indices, set the type 
 * associated with these values.
 * Grow the type table if necessary.
 */
private settype(filei, typei, sym)
int filei;
int typei;
Symbol sym;
{
    Incl file;

    if (filei == -1) {
	typetable[typei] = sym;
    } else {
	if (filei >= nextfile) {
	    error("settype: file index too large, fileindex %d, nextfile %d %s",
	      filei, nextfile, where_in_obj());
	}
	file = file_table[filei];
	if (typei >= file->tablesize) {
	    enlarge_typetable(file);
	}
	file->typetable[typei] = sym;
    }
}

/*
 * Increase the size of a typetable associated with a file.
 * Use realloc() to enlarge it.  There are no pointers pointing
 * within the typetable so it is ok if realloc() moves it.
 * Must clear the new portion.
 */
private enlarge_typetable(file)
Incl file;
{
    int	newsize;

    newsize = (file->tablesize + TYPE_INCR) * sizeof(Symbol);
    file->typetable = (Symbol *) realloc(file->typetable, newsize);
    if (file->typetable == 0)
	fatal("No memory available. Out of swap space?");
    bzero((char *) &file->typetable[file->tablesize], 
      TYPE_INCR * sizeof(Symbol));
    file->tablesize += TYPE_INCR;
}

#ifdef TIMING
prtime(after, before, str)
struct	timeval	*after;
struct	timeval	*before;
char	*str;
{
	float	usecs;

	usecs = after->tv_sec - before->tv_sec;
	usecs = usecs + (float)((after->tv_usec - before->tv_usec) * 0.000001);
	printf("%4.2f seconds %s\n", usecs, str);
}
#endif

panic_and_where(s)
char *s;
{
char *ebuf;
    ebuf = (char*)malloc(strlen(s)*2+MAXPATHLEN*2);
    if (!ebuf)
	panic(s);
    sprintf(ebuf, "%s %s", s, where_in_obj());
    panic(ebuf);
}

private char *
where_in_obj()
{
static char *ewbuf;
char ewbuf2[256];
    ewbuf = (char*)malloc(MAXPATHLEN*2);
    if (!ewbuf)
	return (" ");
    if (cur_incl && cur_incl->name) {
	sprintf(ewbuf, 
		"\n\twhen processing stabs from source %s ",
		cur_incl->name);
    }
    if (objname) {
	sprintf(ewbuf2, "in %s \n", objname);
        strncat(ewbuf, ewbuf2, MAXPATHLEN*2);
    }
    return ewbuf;    
}
