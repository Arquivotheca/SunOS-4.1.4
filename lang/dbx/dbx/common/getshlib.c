#ifndef lint
static	char sccsid[] = "@(#)getshlib.c 1.1 94/10/31 SMI";
#endif

/*
 * Object code interface, mainly for extraction of symbolic information.
 */

#include "defs.h"
#include "machine.h"
#include "dbxmain.h"
#include "symbols.h"
#include "object.h"
#include "names.h"
#include "languages.h"
#include "mappings.h"
#include "lists.h"
#include "process.h"
#include "library.h"
#include <a.out.h>
#include <stab.h>
#include <ctype.h>
#include <setjmp.h>

public struct ld_debug ld_debug_struct;
public caddr_t ld_debug_addr;
public Boolean reading_shlib;
extern char *stringtab;

private jmp_buf shliberr_jmpbuff;
private unsigned int SHLIB_OFFSET;
private Boolean readshlib();
private unsigned int new_stringsize;
private unsigned int new_nsyms;
#ifdef COFF
private unsigned int new_lines;
public SCNHDR shdr;
private char *new_coffstringtab;
private char *new_cofflinetab;
#endif COFF

private struct shlib_descr {
	char *name;
	unsigned int addr;
	struct shlib_descr *next;
} *shlib_list;
private Boolean shlibs_read_done = false;

private dump_shlib_list()
{
struct shlib_descr *p;
	
	for(p = shlib_list; p ; p=p->next)
		printf(" %s  %x\n", p->name, p->addr );
}

public void read_ld_debug()
{
	if(setjmp(shliberr_jmpbuff)) {
		return;
	}
    if (dynam_addr == 0){
		printf("__DYNAMIC is not defined for shlib\n");
		return;
    }
    chk_dread(&dynam, dynam_addr, sizeof(struct link_dynamic));
    ld_debug_addr = (caddr_t)dynam.ldd;
    chk_dread(&ld_debug_struct, ld_debug_addr, sizeof(struct ld_debug));
}

/*
 * go through the shared library list, read in all the information
 */
public void getshlib()
{
    Symbol s;
    struct link_map *lp, entry;
    char file[1024];
    Node cond;
    struct rtc_symb *cp, p;
    struct nlist np;
#ifdef COFF
    struct syment cnp;
#endif COFF
	char sp[100], *ss;

    /* get the mapping between the address and shlib */
	reading_shlib = true;
    read_ld_debug();
	if(setjmp(shliberr_jmpbuff)) {
		reading_shlib = false;
		return;
	}
    if ( dynam.ld_un.ld_1){
	    struct link_dynamic_1 link_d1;
	    chk_dread(&link_d1, dynam.ld_un.ld_1, sizeof(struct link_dynamic_1));
	    lp = link_d1.ld_loaded;
	    while(lp){
		chk_dread(&entry, lp, sizeof(struct link_map));
		chk_dread(file, entry.lm_name, sizeof (file));
		SHLIB_OFFSET = (int)entry.lm_addr;
		if( not shlibs_read_done) {
			struct shlib_descr *shp;
			shp = newarr ( struct shlib_descr, 1);
			if (shp == 0)
				fatal("No memory available. Out of swap space?");
			shp->name =  strdup(file);
			shp->addr = SHLIB_OFFSET;
			shp->next = 0;
			if(!shlib_list)
				shlib_list = shp;
			else {
				struct shlib_descr *shp2;
				for(shp2 = shlib_list; shp2->next; shp2=shp2->next);
				shp2->next = shp;
			}
			if (readshlib(file) == false){
		    	if (*file != '\0')
				fatal("can't read shared library %s", file);
		    	else
				fatal("can't read shared library (bad name)");
			}
		} else {
			struct shlib_descr *shp;
			for(shp = shlib_list; shp; shp=shp->next) 
				if(streq(file, shp->name) and shp->addr == SHLIB_OFFSET) break;
			
			if(!shp)
				error("Shared library description has changed\nEnter `debug %s' command to reread the executable", (objname ? objname : "\"object-file\"") );
		}
		lp = entry.lm_next;
	    }
	    ordfunctab();
		setnlines();
		setnfiles();
    }
    cp = ld_debug_struct.ldd_cp;
    while(cp){
	chk_dread(&p, cp, sizeof(p));
#ifndef COFF
	chk_dread(&np, p.rtc_sp, sizeof(np));
	chk_dread(sp, np.n_un.n_name, sizeof(sp));
	if (*sp == '_')
		ss = &sp[1];
#else
	chk_dread(&cnp, p.rtc_un.rtc_sp_coff, sizeof(cnp));
	chk_dread(sp, cnp._n._n_n._n_offset, sizeof(sp));
	np.n_un.n_name = cnp._n._n_n._n_offset;
	np.n_type = stabtype(&cnp);
	np.n_other = 0;
	np.n_desc = 0;
	np.n_value = cnp.n_value;
	ss = &sp[0];
#endif COFF
	find(s, identname(ss,true)) where
	    s->class == VAR
	endfind(s);
	if (!s){
	    checkin_sym(ss, &np);
	    find(s, identname(ss,true)) where
		s->class == VAR
	    endfind(s);
	    s->symvalue.offset = np.n_value;
	} else if (s && np.n_type == N_COMM){
	    s->symvalue.offset = np.n_value;
	}
	cp = p.rtc_next;
    }
	reading_shlib = false;
	shlibs_read_done = true;
}

/*
 * Read in the namelist from the shared library.
 *
 * Borrowed from readobj().
 */

private Boolean readshlib(file)
String file;
{
    Fileid f;
    struct exec hdr;
    struct nlist nlist;
#ifdef COFF
    FILHDR fhdr;
    register int ix;
#endif COFF

    if (file==(char*)0 || *file=='\0')
	return (false);
    f = open(file, 0);
    if (f < 0) {
	return(false);
    }
#ifndef COFF
    if (read(f, &hdr, sizeof(hdr)) != sizeof(hdr) || N_BADMAG(hdr)) 
	return (false);
    new_nsyms = hdr.a_syms / sizeof(nlist);
	if((SHLIB_OFFSET + hdr.a_text) > objsize )
		objsize = SHLIB_OFFSET + hdr.a_text;
	if(coredump) {
		add_txt_map(file, SHLIB_OFFSET, SHLIB_OFFSET + hdr.a_text,
				N_TXTOFF(hdr));
	}
    if (new_nsyms > 0) {
    	lseek(f, (long) N_STROFF(hdr), 0);
    	read(f, &(new_stringsize), sizeof(new_stringsize));
    	new_stringsize -= 4;
    	stringtab = newarr(char, new_stringsize);
	if (stringtab == 0)
		fatal("No memory available. Out of swap space?");
    	read(f, stringtab, new_stringsize);
    	lseek(f, (long) N_SYMOFF(hdr), 0);
    	readnewsyms(f);
    }
#else
    read(f, &fhdr, sizeof(fhdr));
    read(f, &hdr, fhdr.f_opthdr);
	if((SHLIB_OFFSET + hdr.a_text) > objsize)
		objsize = SHLIB_OFFSET + hdr.a_text;
    new_nsyms = fhdr.f_nsyms;
    for (ix = 0; ix < fhdr.f_nscns; ix++) {
	read(f, &shdr, SCNHSZ);
	if (!strcmp(shdr.s_name, _TEXT)) break;
    }
	if(coredump) {
		add_txt_map(file, SHLIB_OFFSET, SHLIB_OFFSET + hdr.a_text,
				0);
	}
    if (new_nsyms > 0) {
        lseek(f, fhdr.f_symptr+fhdr.f_nsyms*SYMESZ, 0);
    	read(f, &(new_stringsize), sizeof(new_stringsize));
    	new_stringsize -= 4;
    	new_coffstringtab = newarr(char, new_stringsize);
	if (new_coffstringtab == 0)
		fatal("No memory available. Out of swap space?");
    	read(f, new_coffstringtab, new_stringsize);
	if (shdr.s_lnnoptr != 0 and shdr.s_nlnno > 0) {
	    new_cofflinetab = newarr(char, shdr.s_nlnno*LINESZ);
	    if (new_cofflinetab == 0)
		fatal("No memory available. Out of swap space?");
	    lseek(f, shdr.s_lnnoptr, 0);
	    read(f, new_cofflinetab, shdr.s_nlnno*LINESZ);
	    new_lines = shdr.s_nlnno;
	}
	lseek(f, fhdr.f_symptr, 0);
    	readnewsyms(f);
    }
#endif COFF
    close(f);
    return(true);
}

/*
 * Read in more symbols from shared library
 * A simplified readsym() which only deals symbols in the library.
 */

private readnewsyms(f)
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
    struct nlist *np;
	Name n;
	Symbol t;

#ifndef COFF
    namelist = newarr(struct nlist, new_nsyms);
    if (namelist == 0)
	fatal("No memory available. Out of swap space?");
    read(f, namelist, new_nsyms * sizeof(struct nlist));
#else
    namelist = newarr(struct nlist, new_nsyms+new_lines);
    if (namelist == 0)
	fatal("No memory available. Out of swap space?");
    coffnamelist = newarr(SYMENT, new_nsyms);
    if (coffnamelist == 0)
	fatal("No memory available. Out of swap space?");
    read(f, coffnamelist, new_nsyms * SYMESZ);
    jx = stringtabsize(coffnamelist, new_coffstringtab, new_nsyms);
    stringtab = newarr(char, jx);
    if (stringtab == 0)
	fatal("No memory available. Out of swap space?");
    new_nsyms = massagecoff(f, coffnamelist, namelist, stringtab, new_coffstringtab,
				new_cofflinetab, new_nsyms, new_lines, jx);
    new_stringsize = jx;
    dispose(coffnamelist);
    dispose(new_coffstringtab);
    if (shdr.s_lnnoptr != 0 and shdr.s_nlnno > 0)
        dispose(new_cofflinetab);
#endif COFF
    ub = &namelist[new_nsyms];
    for (np = &namelist[0]; np < ub; np++) {
	ix = np->n_un.n_strx;
	if (ix > new_stringsize+4) {
	    continue;
	}
	if (ix != 0) {
	    name = &stringtab[ix - 4];
	} else {
	    name = nil;
	}
	/*
	 * assumptions:
	 *	same as for object.c but we ignore ld inserted symbols
	 *	and don't rely on afterlg
	 *  all N_STAB symbols are entered
	 *  externals which do not collide are entered
	 *	all locals/filenames are entered
	 */
	if ((np->n_type & N_STAB) != 0) {
	    enter_nl(name, &np,  SHLIB_OFFSET);
	} else  if (np->n_type&N_EXT) {
#ifndef COFF
    	if (name[0] == '_'){
			n = identname(&name[1], true);
    	} else {
			n = identname(name, true);
		}
#else
		n = identname(name, true);
#endif COFF

		find (t, n) where
			(char)t->level == (char)program->level and
			(t->class == PROC or t->class == FUNC or t->class == VAR) and
			t->flag == AOUT
		endfind(t);
		/* only look at globals that haven't been defined in a.out */
	    if(t==nil)
			check_global(name, np, SHLIB_OFFSET);
	} else if (name[0] == '_') {
		check_local(&name[1], np, SHLIB_OFFSET);
    } else if ((np->n_type & N_TYPE) == N_TEXT) {
		check_filename(name, np, SHLIB_OFFSET);
	}
	}
    dispose(namelist);
}

private checkin_sym(name, np)
String name;
register struct nlist *np;
{
    register Name n;
    register Symbol t;

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
	    t = insert(n);
	    t->language = findlanguage(".s");
	    t->class = FUNC;
	    t->type = t_int;
	    t->block = program;
	    t->level = program->level;
	    t->symvalue.funcv.src = false;
	    t->symvalue.funcv.inline = false;
	    t->flag = SHLIB;
	    t->symvalue.funcv.beginaddr = np->n_value + SHLIB_OFFSET;
	} else if(t->flag == SHLIB && t->symvalue.funcv.beginaddr == 0){
	    t->symvalue.funcv.beginaddr = np->n_value + SHLIB_OFFSET;
	} else
	    return;
	if (strcmp(name, "_sigtramp") == 0) {
	    sigtramp_Name = t->name;
	}
	newfunc(t, codeloc(t));
	findbeginning(t);
    } else if ((np->n_type & N_TYPE) != N_TEXT){
    	register Symbol t2;

    	find(t2, n) where
        	t2->class == VAR and (char)t2->level == (char)program->level
    	endfind(t2);
    	if (t2 == nil) {
        	t2 = insert(n);
        	t2->language = findlanguage(".s");
        	t2->class = VAR;
        	t2->type = t_int;
        	t2->level = program->level;
		if ((np->n_type & N_TYPE) == N_EXT)
        		t2->block = program;
		else
			t2->block = program;
		t2->flag = SHLIB;
    		t2->symvalue.offset = np->n_value + SHLIB_OFFSET;
    	} else if (t2->flag == SHLIB && t2->symvalue.offset ==0){
		t2->symvalue.offset = np->n_value + SHLIB_OFFSET;
	} else
		return;
    }
}

private chk_dread( arg1, arg2, arg3 )
{
	int save_flag = read_err_flag;

	read_err_flag = 0;
	dread(arg1, arg2, arg3);
	if(read_err_flag) {
		 printf("error while reading shared library\n");
		 read_err_flag = save_flag;
		 longjmp(shliberr_jmpbuff,1);
	}
	read_err_flag = save_flag;
}
