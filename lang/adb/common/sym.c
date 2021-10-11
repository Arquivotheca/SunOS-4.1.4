#ifndef lint
static	char sccsid[] = "@(#)sym.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * adb - symbol table routines
 */
#include "adb.h"
#include <stdio.h>
#include "fio.h"
#include "symtab.h"
#include <link.h>
#include <setjmp.h>

#ifdef KADB
#include <sun/vddrv.h>
#endif

#define INFINITE 0x7fffffff
static int SHLIB_OFFSET;
int use_shlib = 0;
addr_t	ld_debug_addr = 0;
struct	ld_debug ld_debug_struct;
struct	link_dynamic dynam;
addr_t	dynam_addr = 0;
int	address_invalid = 1;
static char *get_all();
static jmp_buf shliberr_jmpbuff;

struct	asym *curfunc, *curcommon;
struct  asym *nextglob;	/* next available global symbol slot */
struct  asym *global_segment; /* current segment of global symbol table */
#define GSEGSIZE 150	/* allocate global symbol buckets 150 per shot */
int	curfile;

struct	asym *enter();
struct	afield *field();
static struct asym *new_lookup();

#define	HSIZE	255
struct	asym *symhash[HSIZE];
struct	afield *fieldhash[HSIZE];

int	pclcmp(), lpccmp();
int	symvcmp();

sort_globals(flag)
{
	int i;

	if (nlinepc){
		qsort((char *)linepc, nlinepc, sizeof (struct linepc),lpccmp );
		pcline = (struct linepc*) calloc((nlinepc == 0 ? 1 : nlinepc),
			sizeof(struct linepc));
		if (pcline == 0)
			outofmem();
		for (i = 0; i < nlinepc; i++)
			pcline[i] = linepc[i];
		qsort((char *)pcline, nlinepc, sizeof (struct linepc),pclcmp );
	}
	if (nglobals == 0)
		return;
	globals = (struct asym **)
	   	    realloc((char *)globals, nglobals * sizeof (struct asym *));
	if (globals == 0)
		outofmem();
	/* arrange the globals in ascending value order, for findsym()*/
	qsort((char *)globals, nglobals, sizeof(struct asym *),symvcmp);
	/* prepare the free space for shared libraries */
	if(flag == AOUT)
		globals = (struct asym **)
		    realloc((char*)globals, NGLOBALS * sizeof(struct asym*));
}

stinit(fsym, hp, flag)
	int fsym;
	struct exec *hp;
{
	struct nlist nl[BUFSIZ / sizeof (struct nlist)];
	register struct nlist *np;
	register struct asym *s;
	int ntogo = hp->a_syms / sizeof (struct nlist), ninbuf = 0;
	off_t loc; int ssiz; char *strtab;
	register int i;

	if (hp->a_syms == 0)
		return;
	loc = N_SYMOFF(*hp);

	/* allocate and read string table */
	(void) lseek(fsym, (long)(loc + hp->a_syms), 0);
	if (read(fsym, (char *)&ssiz, sizeof (ssiz)) != sizeof (ssiz)) {
		printf("old format a.out - no string table\n");
		return;
	}
	if (ssiz == 0) {	/* check whether there is a string table */
		fprintf(stderr, "Warning -- empty string table. No symbols.\n");
		fflush(stderr);
		return;
	}
	strtab = (char *)malloc(ssiz + 8);	/* TEMP!  kluge to work around kernel bug */
	if (strtab == 0)
		outofmem();
	*(int *)strtab = ssiz;
	ssiz -= sizeof (ssiz);
	if (read(fsym, strtab + sizeof (ssiz), ssiz) != ssiz)
		goto readerr;
	/* read and process symbols */
	(void) lseek(fsym, (long)loc, 0);
	while (ntogo) {
		if (ninbuf == 0) {
			int nread = ntogo, cc;
			if (nread > BUFSIZ / sizeof (struct nlist))
				nread = BUFSIZ / sizeof (struct nlist);
			cc = read(fsym,(char *)nl,nread*sizeof (struct nlist));
			if (cc != nread * sizeof (struct nlist))
				goto readerr;
			ninbuf = nread;
			np = nl;
		}
		if (np->n_un.n_strx)
			np->n_un.n_name = strtab + np->n_un.n_strx;
		if (dosym(np++, strtab, strtab + ssiz + sizeof(ssiz), flag))
			break;
		ninbuf--; ntogo--;
	}
	if (flag == AOUT){
		/* make 0 pctofilex entries? */
		sort_globals(flag);
	}
	return;
readerr:
	printf("error reading symbol or string table\n");
	exit(1);
}

static
dosym(np, start, end, flag)
	char *start, *end;
	register struct nlist *np;
{
	register struct asym *s;
	register struct afield *f;
	register char *cp;
	int h;

	/*
	 * Discard symbols containing a ":" -- they
	 * are dbx symbols, and will confuse us.
	 * Same with the "-lg" symbol and nameless symbols
	 */
	if (cp=np->n_un.n_name) {
		if ((cp < start) || (cp > end)) {
			printf("Warning: string table corrupted\n");
			return 1;
		}
		if( strcmp( "-lg", cp ) == 0  )
			return 0;
		for (; *cp; cp++)
			if (*cp == ':')
				return 0;
	} else {
		return 0;
	}

	switch (np->n_type) {

	case N_PC:
		/*
		 * disregard N_PC symbols; these are emitted
		 * by pc to detect inconsistent definitions
		 * at link time.
		 */
		return 0;

	case N_TEXT:
		/*
		 * disgard symbols containing a .
		 * this may be a short-term kludge, to keep
		 * us from printing "routine" names like
		 * "scan.o" in stack traces, rather that
		 * real routine names like "_scanner".
		 */
		 for (cp=np->n_un.n_name; *cp; cp++)
		     if (*cp == '.') return 0;
		/* fall thru */
	case N_TEXT|N_EXT:
	case N_DATA:
	case N_DATA|N_EXT:
	case N_BSS:
	case N_BSS|N_EXT:
	case N_ABS:
	case N_ABS|N_EXT:
	case N_UNDF:
	case N_UNDF|N_EXT:
	case N_GSYM:
	case N_FNAME:
	case N_FUN:
	case N_STSYM:
	case N_LCSYM:
	case N_ENTRY:
		s = new_lookup(np->n_un.n_name);
		if (s)
			if(flag == AOUT)
				mergedef(s, np);
			else 
				re_enter(s, np, flag);
		else
			s = enter(np, flag);
		if (np->n_type == N_FUN) {
			if (curfunc && curfunc->s_f) {
				curfunc->s_f = (struct afield *)
				    realloc((char *)curfunc->s_f,
					curfunc->s_fcnt * 
					  sizeof (struct afield)); 
				if (curfunc->s_f == 0)
					outofmem();
				curfunc->s_falloc = curfunc->s_fcnt;
			}
			curfunc = s;
		}
		switch (np->n_type) {

		case N_FUN:
		case N_ENTRY:
		case N_GSYM:
		case N_STSYM:
		case N_LCSYM:
			s->s_fileloc = FILEX(curfile, np->n_desc);
			goto sline;
		}
		break;

	case N_RSYM:
	case N_LSYM:
	case N_PSYM:
		s = curfunc;
		if(s) (void) field(np, &s->s_f, &s->s_fcnt, &s->s_falloc);
		break;

	case N_SO:
	case N_SOL:
		curfile = fenter(np->n_un.n_name);
		break;

	case N_SSYM:
		if (s = curcommon) {
			(void) field(np, &s->s_f, &s->s_fcnt, &s->s_falloc);
			break;
		}
		f = globalfield(np->n_un.n_name);
		if (f == 0) {
			h = hashval(np->n_un.n_name);
			/*
			  We must allocate storage here ourselves, as
			  field() will attempt to do a realloc(), and
			  we cannot tolerate that as we are hashing.
			*/
			if (nfields >= NFIELDS) {
				NFIELDS += 50; /* up the ante by 50 each time */
				if ((fields = (struct afield *)calloc(NFIELDS, sizeof(struct afield))) == NULL)
					outofmem();
				nfields = 0;
			}
			f = field(np, &fields, &nfields, &NFIELDS);
			f->f_link = fieldhash[h];
			fieldhash[h] = f;
		}
		break;

	case N_LBRAC:
	case N_RBRAC:
	case N_LENG:
		break;

	case N_BCOMM:
		curcommon = enter(np, flag);		/* name ? */
		break;

	case N_ECOMM:
	case N_ECOML:
		curcommon = 0;
		break;


sline:
	case N_SLINE:
		lenter(FILEX(curfile, (unsigned short)np->n_desc), (int)np->n_value);
		break;
	}
	return 0;
}

static
mergedef(s, np)
	struct asym *s;
	struct nlist *np;
{

	switch (s->s_type) {

	case N_TEXT:
	case N_TEXT|N_EXT:
	case N_DATA:
	case N_DATA|N_EXT:
	case N_BSS:
	case N_BSS|N_EXT:
		/* merging fake symbol into real symbol -- very unlikely */
		s->s_type = np->n_type;
		break;
	case N_GSYM:
	case N_FUN:
		/* merging real symbol into fake symbol -- more likely */
		s->s_name = np->n_un.n_name; /* want _name, really */
		s->s_value = np->n_value;
		break;
	}
}

static
symvcmp(s1, s2)
	struct asym **s1, **s2;
{

	return ((unsigned int)(*s1)->s_value - (unsigned int)(*s2)->s_value);
}

static
hashval(cp)
	register char *cp;
{
	register int h = 0;

	while (*cp == '_')
		cp++;
	while (*cp && (*cp != '_' || cp[1])) {
		h *= 2;
		h += *cp++;
	}
	h %= HSIZE;
	if (h < 0)
		h += HSIZE;
	return (h);
}

struct asym *
enter(np, flag)
	struct nlist *np;
{
	register struct asym *s;
	register int h;

	/*
	 * if this is the first global entry,
	 * allocate the global symbol spaces
	 */
	if (NGLOBALS == 0) {
		NGLOBALS = GSEGSIZE;
		globals = (struct asym **)
		    malloc(GSEGSIZE * sizeof (struct asym *));
		if (globals == 0)
			outofmem();
		global_segment = nextglob = (struct asym *) 
		    malloc(GSEGSIZE * sizeof(struct asym));
		if (nextglob == 0)
			outofmem();
	}
	/*
	 * if we're full up, reallocate the pointer array,
	 * and give us a new symbol segment
	 */
	if (nglobals == NGLOBALS) {
		NGLOBALS += GSEGSIZE;
		globals = (struct asym **)
		    realloc((char *)globals, NGLOBALS*sizeof (struct asym *));
		if (globals == 0)
			outofmem();
		global_segment = nextglob = (struct asym *)
		    malloc( GSEGSIZE*sizeof (struct asym));
		if (nextglob == 0)
			outofmem();
	}
	globals[nglobals++] = s = nextglob++;
	s->s_name = np->n_un.n_name;
	s->s_type = np->n_type;
	s->s_f = 0; s->s_fcnt = 0; s->s_falloc = 0;
	s->s_flag = flag;
	if (flag == AOUT)
		s->s_value = np->n_value;
	else if (s->s_type != (N_EXT | N_UNDF))
		/* flag == SHLIB and not COMMON */
		s->s_value = np->n_value + SHLIB_OFFSET;
	s->s_fileloc = 0;
	if(!use_shlib && strcmp(s->s_name, "__DYNAMIC")==0){
		dynam_addr = (addr_t)s->s_value;
		if (dynam_addr){
			use_shlib++;
		}
	}
	h = hashval(np->n_un.n_name);
	s->s_link = symhash[h];
	symhash[h] = s;
	return (s);
}

/*
 * whenever rerun the program, refresh the sysmbols defined in the
 * shared library to reflect the true address
 */
re_enter(s, np, flag)
	struct asym *s;
	struct nlist *np;
{
	if (flag != AOUT && s->s_value == 0 && s->s_type != (N_EXT | N_UNDF))
		s->s_value = np->n_value + SHLIB_OFFSET;
}

static
char *
get_all(addr, leng)
int *addr;
{
	int *copy, *save;
	char *saveflg = errflg;

	errflg =0;
	/* allocate 4 more, let get to clobber it */
	save = copy = (int*)malloc(leng+4);
	for(; leng > 0; addr++, copy++){
			*copy = get(addr,DSP);
			leng = leng - 4;
	}
	if(errflg) {
		printf("error while reading shared library: %s\n",errflg);
		errflg = saveflg;
		longjmp(shliberr_jmpbuff,1);
	}
	errflg = saveflg;
	return (char*)save;
}

static char*
get_string(addr)
int *addr;
{
	int *copy;
	char *c, *save = 0;
	int done = 0;
	char *saveflg = errflg;

	if (addr == NULL){
		printf("nil address found while reading shared library\n");
		return 0;
	}
	/* assume the max path of shlib is 1024 */
	copy = (int*)malloc(1024);
	if (copy == 0)
		outofmem();
	save = (char*)copy;
	errflg =0;
	while (!done){
		*copy = get(addr, DSP);
		c = (char*)copy;
		if(*c == '\0' || *c++ == '\0' || *c++ == '\0' || *c++ == '\0')
			done++;
		copy++;
		addr++;
	}
	if(errflg) {
		printf("error while reading shared library:%s\n", errflg);
		errflg = saveflg;
		longjmp(shliberr_jmpbuff,1);
	}
	errflg = saveflg;
	return save;
}

/*
 * read the ld_debug structure
 */
char *
read_ld_debug()
{
	if(setjmp(shliberr_jmpbuff)) {
		return 0;
	}
	if (dynam_addr == 0){
		printf("__DYNAMIC is not defined for shlib\n");
		return;
	}
	dynam = *(struct link_dynamic *)get_all(dynam_addr, sizeof(struct link_dynamic));
	ld_debug_addr = (addr_t)dynam.ldd;
	return get_all(ld_debug_addr, sizeof(struct ld_debug));
}

/* extend the ? map by adding new ranges */
extend_text_map()
{
	struct	ld_debug *check;
	check = (struct ld_debug*)read_ld_debug();
#ifndef KADB
		read_in_shlib();
#endif
}

#ifndef KADB
/*
 * before refresh the symbol table, zero the values of those symbols
 * defined in the shared lib, to avoid multiply defined symbols in
 * different lib
 */
static
clean_up()
{
	register struct asym **p, *q;
	register int i;

	for (p = globals, i = nglobals; i > 0; p++, i--){
		if ((q = *p)->s_flag != AOUT)
			q->s_value = 0;
	}
	free_shlib_map_ranges(&txtmap);
	free_shlib_map_ranges(&datmap);
}

/*
 * read in the symbol table information from the shared lib 
 */
read_in_shlib(){
	struct link_map *lp;
	struct link_map entry;
	struct rtc_symb *cp, *p;
	struct nlist *np;
	struct asym *s;
	struct	ld_debug *check;

	clean_up();
	/* get the mapping between the address and shlib */
	if(check = (struct ld_debug*)read_ld_debug())
		ld_debug_struct = *check;
	if(setjmp(shliberr_jmpbuff)) {
		return;
	}
	lp = ((struct link_dynamic_1*)get_all(dynam.ld_un.ld_1, sizeof(struct link_dynamic_1)))->ld_loaded;
	for ( ; lp; lp = entry.lm_next){
		struct exec exec;
		int file;
		char *lib;

		entry = *(struct link_map*)get_all(lp, sizeof(struct link_map));
		lib = get_string(entry.lm_name);
		if(!lib)
			return;
		file = getfile(lib, INFINITE);
		if(read(file, (char*)&exec, sizeof exec) != sizeof exec
		   || N_BADMAG(exec)){
			printf("can't open shared library, %s\n", lib);
			return;
		}
		SHLIB_OFFSET = (int)entry.lm_addr;
		stinit(file, &exec, SHLIB);
		if(!pid) {
			add_map_range(&txtmap, 
				SHLIB_OFFSET, SHLIB_OFFSET+exec.a_text, N_TXTOFF(exec),
				lib);
		}
		(void) close(file);
	}
	cp = ld_debug_struct.ldd_cp;
	while (cp){
		char *sp;

		p = (struct rtc_symb*)get_all(cp, sizeof(struct rtc_symb));
		np = (struct nlist*)get_all(p->rtc_sp, sizeof(struct nlist));
		sp = get_string(np->n_un.n_name);
		if(!sp)
			return;
		s = new_lookup(sp);
		if (!s) {
			s = enter(np, SHLIB);
			s->s_value = np->n_value;
		} else if (s->s_type == (N_EXT|N_UNDF) || s->s_type == N_COMM){
			s->s_type = np->n_type;
			s->s_value = np->n_value;
		} else {
			break;
		}
		cp = p->rtc_next;
	}
	sort_globals(SHLIB);
	address_invalid = 0;
}
#endif !KADB

struct asym *
lookup(cp)
	char *cp;
{
	register struct asym *s;
	register int h;

#ifdef KADB
	extern struct asym *lookupvdsym();
#endif

	h = hashval(cp);
	for (s = symhash[h]; s; s = s->s_link)
		if (!strcmp(s->s_name, cp)) {
			cursym = s;
			return (s);
		}
	for (s = symhash[h]; s; s = s->s_link)
		if (eqsym(s->s_name, cp, '_')) {
			cursym = s;
			return (s);
		}
#ifdef KADB
        /*
         * Perhaps we'll find the symbol in a loadable module.
         * Let's scan the kernel's loadable module table.
         * If we're debugging something other than the kernel,
         * then we won't find the symbols "vddrv" or "vdloaded" or "vdcurmod"
         * so we'll skip all this.
         */
        if (strcmp(cp, "vddrv_list") != 0 && strcmp(cp, "vdloaded") != 0 &&
            strcmp(cp, "vdcurmod")   != 0 && strcmp(cp, "Sysbase")  != 0) {
                if (cursym = lookupvdsym(cp)) {
                        return (cursym);
                }
        }
#endif

	cursym = 0;
	return (0);
}

static struct asym *
new_lookup(cp)
	char *cp;
{
	/*
	  new_lookup() is for looking up new symbols in the symbol
	  table before entering them (as opposed to looking them up
	  to find their values). Its main claim to existance is that it
	  will, under the proper circumstances, find an equivalent
	  name when regular lookup() will not. For example, we want
	  the external name "_main" to find the function name "main",
	  so that we may merge them. Regular lookup won't, as it
	  will only compare "_main" with "__main".
	*/
	register struct asym *s;
	register int h;

	h = hashval(cp);
	for (s = symhash[h]; s; s = s->s_link){
		if( eqsym(s->s_name, cp, '_')) {
			cursym = s;
			return (s);
		}
		switch(s->s_type){
		case N_FUN:
		case N_GSYM:
			/* only known cases */
			if (eqsym(cp, s->s_name,  '_')) {
				cursym = s;
				return (s);
			}
		}
	}
	cursym = 0;
	return (0);
}

/*
 * The type given to findsym should be an enum { NSYM, ISYM, or DSYM },
 * indicating which instruction symbol space we're looking in (no space,
 * instruction space, or data space).  On VAXen, 68k's and SPARCs,
 * ISYM==DSYM, so that distinction is vestigial (from the PDP-11).
 */
findsym(val, type)
	int val, type;
{
	register struct asym *s;
	register int i, j, k;
	register int off;

	extern u_int Sysbase;

	cursym = 0;
	if (type == NSYM)
		return (MAXINT);
#ifdef KADB
	/*
	 * If you have the lowest address where a loadable module can
	 * start, then this is an optimization to only check a loadable
	 * module if the address is within the loadable module range.
	 */
        if ((u_int)val >= Sysbase) {
                off = findvdsym(val);
		if (off != MAXINT)
			return (off);
	}
#endif

	i = 0; j = nglobals - 1;
	while (i <= j) {
		k = (i + j) / 2;
		s = globals[k];
		if (s->s_value == val)
			goto found;
		if (s->s_value > val)
			j = k - 1;
		else
			i = k + 1;
	}
	if (j < 0)
		return (MAXINT);
	s = globals[j];
found:
	cursym = s;
	return (val - s->s_value );
}

/*
 * Given a value v, of type type, find out whether it's "close" enough
 * to any symbol; if so, print the symbol and offset.  The third
 * argument is a format element to follow the symbol, e.g., ":%16t".
 * SEE ALSO:  ssymoff, below.
 */
psymoff(v, type, s)
	int v, type;
	char *s;
{
	int w;

	if (v) 
		w = findsym(v, type);

	if (cursym != NULL) {
		switch (cursym->s_type) {
		case N_ABS:
		case N_ABS|N_EXT:
		case N_COMM:
		case N_COMM|N_EXT:
			w = maxoff;
			break;
		}
	}

	/* NBPG used to be maxoff, but I like using a big maxoff */
	if(((unsigned) v) >= KERNELBASE && KVTOPH(v) < NBPG  &&  w ) {
		printf("%Z", v);
	} else if (v == 0 || w >= maxoff ) {
		printf("%Z", v);
	} else {
		printf("%s", cursym->s_name);
		if (w)
			printf("+%Z", w);
	}
	printf(s);
}


/*
 * ssymoff is like psymoff, but uses sprintf instead of printf.
 * It's copied & slightly modified from psymoff.
 *
 * ssymoff returns the offset, so the caller can decide whether
 * or not to print anything.
 *
 * NOTE:  Because adb's own printf doesn't provide sprintf, we
 * must use the system's sprintf, which lacks adb's special "%Z"
 * and "%t" format effectors.
 */
ssymoff(v, type, buf)
	int v, type;
	char *buf;
{
	int w, len_sofar;

	if (v) 
		w = findsym(v, type);

	if (cursym != NULL) {
		switch (cursym->s_type) {
		case N_ABS:
		case N_ABS|N_EXT:
		case N_COMM:
		case N_COMM|N_EXT:
			w = maxoff;
			break;
		}
	}

	/* NBPG used to be maxoff, but I like using a big maxoff */
	len_sofar = 0;
	if( v != 0 && w < maxoff
	  &&  ((unsigned)v < KERNELBASE ||  KVTOPH(v) >= NBPG || !w)) {
		sprintf( buf, "%s", cursym->s_name);
		if( w == 0 )
			return w;
		len_sofar = strlen( buf );
		strcpy( &buf[ len_sofar ], " + " );
		len_sofar += 3;
		v = w;
	}
	if( v < 10 )	sprintf( &buf[ len_sofar ], "%x", v );
	else		sprintf( &buf[ len_sofar ], "0x%x", v );
	return w;
}


struct afield *
field(np, fpp, fnp, fap)
	struct nlist *np;
	struct afield **fpp;
	int *fnp, *fap;
{
	register struct afield *f;

	if (*fap == 0) {
		*fpp = (struct afield *)
		    calloc(10, sizeof (struct afield)); 
		if (*fpp == 0)
			outofmem();
		*fap = 10;
	}
	if (*fnp == *fap) {
		*fap *= 2;
		*fpp = (struct afield *)
		    realloc((char *)*fpp, *fap * sizeof (struct afield));
		if (*fpp == 0)
			outofmem();
	}
	f = *fpp + *fnp; (*fnp)++;
	f->f_name = np->n_un.n_name;
	f->f_type = np->n_type;
	f->f_offset = np->n_value;
	return (f);
}

struct afield *
fieldlookup(cp, f, nftab)
	char *cp;
	register struct afield *f;
	int nftab;
{

	while (nftab > 0) {
		if (eqsym(f->f_name, cp, '_'))
			return (f);
		f++; nftab--;
	}
	return (0);
}

struct afield *
globalfield(cp)
	char *cp;
{
	register int h;
	register struct afield *f;

	h = hashval(cp);
	for (f = fieldhash[h]; f; f = f->f_link)
		if (!strcmp(cp, f->f_name))
			return (f);
	return (0);
}

static
lenter(afilex, apc)
	unsigned afilex, apc;
{

	if (NLINEPC == 0) {
		NLINEPC = 1000;
		linepc = (struct linepc *)
		    malloc(NLINEPC * sizeof (struct linepc));
		linepclast = linepc;
	}
	if (nlinepc == NLINEPC) {
		NLINEPC += 1000;
		linepc = (struct linepc *)
		    realloc((char *)linepc, NLINEPC * sizeof (struct linepc));
		linepclast = linepc+nlinepc;
	}
	if( XLINE(afilex) == 0xffff ) afilex = 0; /* magic */
	linepclast->l_fileloc = afilex;
	linepclast->l_pc = apc;
	linepclast++; nlinepc++;
}

/* 
 * implement the $p directive by munging throught the global
 * symbol table and printing the names of any N_FUNs we find
 */
printfuns()
{
	register struct asym **p, *q;
	register int i;

	for (p = globals, i = nglobals; i > 0; p++, i--) {
		if ((q = *p)->s_type == N_FUN)
			printf("\t%s\n", q->s_name);
	}
}

static
pclcmp(l1, l2)
	struct linepc *l1, *l2;
{

	return (l1->l_pc - l2->l_pc);
}

static
lpccmp(l1, l2)
	struct linepc *l1, *l2;
{

	return (l1->l_fileloc - l2->l_fileloc);
}

pctofilex(apc)
	int apc;
{
	register int i, j, k;
	register struct linepc *lp;

	i = 0; j = nlinepc - 1;
	while (i <= j) {
		k = (i + j) / 2;
		lp = pcline + k;
		if (lp->l_pc == apc) {
			j = k;
			goto found;
		}
		if (lp->l_pc > apc)
			j = k - 1;
		else
			i = k + 1;
	}
	if (j < 0) {
		filex = 0;
		return (-1);
	}
found:
	lp = pcline + j;
	filex = lp->l_fileloc;
	if (filex == 0 )
		return (-1);		/* how do we get these? */
					/* from libg.a...       */
	return (apc - lp->l_pc);
}

filextopc(afilex)
	int afilex;
{
	register int i, j, k;
	register struct linepc *lp;

	i = 0; j = nlinepc - 1;
	while (i <= j) {
		k = (i + j) / 2;
		lp = linepc + k;
		if (lp->l_fileloc == afilex)
			return (lp->l_pc);
		if (lp->l_fileloc > afilex)
			j = k - 1;
		else
			i = k + 1;
	}
	if (j < 0 || i >= nlinepc)
		return (0);
	lp = linepc + i;
	if (XFILE(lp->l_fileloc) != XFILE(afilex))
		return (0);
	return (lp->l_pc);
}


outofmem()
{

	printf("ran out of memory for symbol table.\n");
	exit(1);
}

#ifdef KADB
/*
 * Support for loadable modules
 */
static struct vddrv **vddrv_list;/* pointer to kernel loadable module table */
static int *vdloaded;		/* number of loaded modules */
static int *vdcurmod;		/* current module to search first */
static u_int Sysbase;		/* lowest address of a loadable module */

/*
 * Get pointers to kernel data structures for loadable modules
 */
static int
vdinit()
{
	register struct asym *s;
	extern scbsyncdone;

	if (scbsyncdone == 0)
		return (1);	/* kernel hasn't been single stepped yet! */

	/*
	 * If we don't already know the addresses of the kernel
	 * data structures for loadable modules, look them up now.
	 */
	if (vddrv_list == (struct vddrv **)NULL) {
		if ((s = lookup("vddrv_list")) == (struct asym *)NULL)
			return (1);

		vddrv_list = (struct vddrv **)(s->s_value);
	}
	if (vdloaded == (int *)NULL) {
		if ((s = lookup("vdloaded")) == (struct asym *)NULL)
			return (1);

		vdloaded = (int *)s->s_value;
	}
	if (vdcurmod == (int *)NULL) {
		if ((s = lookup("vdcurmod")) == (struct asym *)NULL)
			return (1);

		vdcurmod = (int *)s->s_value;
	}
	if (Sysbase == 0) {
		if ((s = lookup("Sysbase")) == (struct asym *)NULL)
			return (1);

		Sysbase = (u_int)s->s_value;
	}
	if (*vdcurmod == -1 || *vdcurmod == 0) {
		return (1);		/* user doesn't want us to look
				   	   for symbols in loadable modules */
	}
	if (*vdloaded == 0) {
		return (1);		/* there aren't any loaded modules */
	}
	return (0);
}

/*
 * given the string pointer "cp", look through all of the loaded modules
 * and see if there is a matching string in the symbol table.
 */
struct asym *
lookupvdsym(cp)
	char *cp;
{
	register int i;
	register struct vddrv *vdp;
	register struct asym *s;

	extern struct asym *searchvdsym();
	extern struct vddrv *vddrv_lookup();
	extern struct vddrv *vddrv_lookup_next();

	if (vdinit())
		return ((struct asym *)NULL); /* no symbols in loadable mods */

	/*
	 * Start searching from the current module so that if there are
	 * duplicate symbols, we'll be able to find the one the user wants
	 */
	if (*vdcurmod >= 0) {
		vdp = vddrv_lookup(*vdcurmod);
		if (vdp != NULL) {
			s = searchvdsym(cp, vdp);
			if (s)
			    return (s);
		}
	}

	for (vdp = vddrv_lookup_next(0); vdp != NULL; 
	     vdp = vddrv_lookup_next(vdp->vdd_id)) {
		if (vdp->vdd_id == *vdcurmod)
			continue;  /* skip this, we already checked */

		s = searchvdsym(cp, vdp);	
		if (s)
		    return (s);
	}
	return ((struct asym *)NULL); 
}

static struct vddrv *
vddrv_lookup(id)
        int id;
{
        register struct vddrv *vdp;

        for (vdp = *vddrv_list; vdp != NULL; vdp = vdp->vdd_next) {
                if (id == vdp->vdd_id)
                        return (vdp);
        }
        return (NULL);
}

static struct vddrv *
vddrv_lookup_next(id)
        int id;
{
        register struct vddrv *vdp;

        for (vdp = *vddrv_list; vdp != NULL; vdp = vdp->vdd_next) {
		if (id == 0)
			return (vdp);

                if (id == vdp->vdd_id)
			return (vdp->vdd_next);
        }
        return (NULL);
}

/*
 * Search for a symbol by name in a given loadable module.
 */
static struct asym vdasym1;	/* structure for symbol in a loadable module */

static struct asym *
searchvdsym(cp, vdp)
	char *cp;
	struct vddrv *vdp;
{
	register int i;
	register int nsyms;
	register struct vdsym *vds;

	if ((nsyms = vdp->vdd_nsyms) == 0)
		return (struct asym *)NULL;	/* no symbols */

	vds = (struct vdsym *)vdp->vdd_symvaddr;  /* start of symbol table */

	for (i = 0; i < nsyms; i++) {
		if (eqsym((char *)vds + vds[i].vdsym_name, cp, '_')) {
			vdasym1.s_name = (char *)((char *)vds + 
						  vds->vdsym_name);
			vdasym1.s_type = N_GSYM;
			vdasym1.s_value = vds[i].vdsym_value;	
			return (&vdasym1);	/* found it! */
		}
	}	
	return (struct asym *)NULL;		/* didn't find it */
}

/*
 * Find the symbol in a loadable module that's near "val".
 */
findvdsym(val)
	int val;
{
	register struct vddrv *vdp;
	register int i;	
	register int symdiff;

	if (vdinit())
		return (MAXINT);

	for (vdp = vddrv_lookup_next(0); vdp != NULL; 
	     vdp = vddrv_lookup_next(vdp->vdd_id)) {
		symdiff = searchvdval(val, vdp);	
		if (symdiff != MAXINT)
			return (symdiff);
	}
	return (MAXINT);			/* didn't find anything */
}

/*
 * Search a given loadable module for a symbol near "val".
 */
static struct asym vdasym2;

static int
searchvdval(val, vdp)
	u_int val;
	struct vddrv *vdp;
{
	register int i;
	register int nsyms;
	register struct vdsym *vds;

	struct vdsym *s;

	int j,k;

	if ((nsyms = vdp->vdd_nsyms) == 0)
		return (MAXINT);	/* no symbols */

	vds = (struct vdsym *)vdp->vdd_symvaddr;  /* start of symbol table */

	if (val < (u_int)vdp->vdd_textvaddr || 
	    val > (u_int)vdp->vdd_bssvaddr + vdp->vdd_bsssize)
		return (MAXINT);	/* no symbol for val in this module */

	/*
	 * Let's find the nearest symbol.  They're sorted numerically
	 * so let's do a binary search.
	 */
	i = 0; j = nsyms - 1;
	while (i <= j) {
		k = (i + j) / 2;
		s = &vds[k];
		if (s->vdsym_value == val)
			goto found;
		if ((unsigned)s->vdsym_value > (unsigned)val)
			j = k - 1;
		else
			i = k + 1;
	}
	if (j < 0)
		return (MAXINT);
	s = &vds[j];
found:
	vdasym2.s_name = (char *)((char *)vds + s->vdsym_name);
	vdasym2.s_type = N_GSYM;
	vdasym2.s_value = s->vdsym_value;	
	cursym = &vdasym2;
	return ((unsigned)val - (unsigned)s->vdsym_value );
}
#endif
