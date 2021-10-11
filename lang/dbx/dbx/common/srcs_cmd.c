#ifndef lint
static  char sccsid[] = "@(#)srcs_cmd.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* support for the "modules" command - nee "sources" - hence the naming */

#include <stdio.h>
#include <search.h>
#include "defs.h"
#include "mappings.h"
#include "object.h"
#include <a.out.h>
#include <stab.h>

#ifndef public
#endif

#define MAXNAMLEN 60
typedef struct sources_sel_item {
	char	obj_file[MAXNAMLEN];
	int		flags;
} Sources_sel_item;

/* flag values */
#define ITEM_WAS_SEEN 1
#define ITEM_IMPLIED 2 /*object not selected but required because of includes*/

Sources_sel_item *sources_sel_tab;
int sel_tab_nelem, sel_tab_size;
#define SEL_TAB_INCR 20	/* grow it by this many elements */


public sources_cmd(cp)
char *cp;
{
	if(*cp == '\0') {
		display_source_files();
	} else if(!strncmp(cp,"select", sizeof("select")-1)) {
		char *cp2 = cp+sizeof("select")-1;

		if( *cp2 == '\0') {
			display_select_list();
		} else {
			cp2++;
			if( !strcmp(cp2,"all") ) {
				sel_tab_nelem=0;
			} else {
				sel_tab_nelem=0;
				addto_select_list(cp2);
			}
		}
	} else if(!strncmp(cp,"append", sizeof("append")-1)) {
			addto_select_list(cp+sizeof("append")-1);
	} else {
		error("illegal \"modules\" command");
	}
}


private addto_select_list(cp)
	char *cp;
{
	Sources_sel_item item;
	int compar();

	if(!sources_sel_tab) {
		sources_sel_tab = (Sources_sel_item*) calloc(SEL_TAB_INCR, sizeof(Sources_sel_item));
		if (sources_sel_tab == 0)
			fatal("No memory available. Out of swap space?");
		sel_tab_size = SEL_TAB_INCR;
		sel_tab_nelem = 0;
	}
											/* FIXME - need bulletproofing*/
	item.flags = 0;
	while( sscanf(cp,"%s", item.obj_file ) != EOF) {
		lsearch((char*)&item, (char*)sources_sel_tab, 
			&sel_tab_nelem, sizeof(Sources_sel_item), compar);
		if(sel_tab_nelem == sel_tab_size) {
			enlarge_sel_tab();
		}
		if(*cp == ' ')	cp++;
		cp += strlen(item.obj_file);
	}
}

private compar(p1,p2)
Sources_sel_item *p1,*p2;
{
	return (strcmp(p1->obj_file, p2->obj_file));
}

private enlarge_sel_tab()
{
	int newsize = (sel_tab_size + SEL_TAB_INCR ) * sizeof(Sources_sel_item);

	sources_sel_tab = (Sources_sel_item*) realloc( sources_sel_tab, newsize);
	if (sources_sel_tab == 0)
		fatal("No memory available. Out of swap space?");
	bzero( (char*) &sources_sel_tab[sel_tab_size], 
			SEL_TAB_INCR*sizeof(Sources_sel_item));
	sel_tab_size += SEL_TAB_INCR;
}

public display_select_list()
{
	Sources_sel_item *sp;

	if(sel_tab_nelem){
		printf("modules select ");
		for(sp = sources_sel_tab; sp < &sources_sel_tab[sel_tab_nelem]; sp++) {
			if(!(sp->flags&ITEM_IMPLIED)) {
				printf("%s ",sp->obj_file);
			}
		}
		printf("\n");
	}
}

public is_sources_sel_list_active()
{
	return( sel_tab_nelem != 0 );
}

public isin_sources_sel_list(name, was_seen)
	char *name;
	Boolean was_seen;
{
	Sources_sel_item item, *sp;

	strcpy(item.obj_file, name);
	if(sp = (Sources_sel_item*) lfind((char*)&item, (char*)sources_sel_tab,
			&sel_tab_nelem, sizeof(Sources_sel_item), compar)) {
			sp->flags |= ITEM_WAS_SEEN;
			return 1;
	} else {
		return 0;
	}
}

	/* after symbol reading, gripe about any elements on the selection 
	** list which were not seen 
	*/
public print_sel_list_exceptions() 
{
	register Sources_sel_item item, *sp;
	Boolean flag = false;

	if(sel_tab_nelem){
		for(sp = sources_sel_tab; sp < &sources_sel_tab[sel_tab_nelem]; sp++) {
			if(!(sp->flags&ITEM_IMPLIED) && !(sp->flags&ITEM_WAS_SEEN) ) {
				flag = true;
			}
		}

		if(flag) {
			printf("\nwarning: object file(s) \" ");
			for(sp = sources_sel_tab;sp<&sources_sel_tab[sel_tab_nelem];sp++){
				if(!(sp->flags&ITEM_IMPLIED) && !(sp->flags&ITEM_WAS_SEEN) ) {
					printf("%s ",sp->obj_file);
				}
			}
			printf("\" were not found in %s\n",objname);
		}
	}
}

	/* order the filetab so all files associated with a module are
	** grouped together then print them 
	*/
private display_source_files()
{
	Filetab *p;
	struct mod_files { /* set of files associated with a module */
		char *modname; /* the module name */
		struct src_file { /* the source files */
			char *filename;
			struct src_file *next;
		} *first_src;
		struct mod_files *next;
	} *head = NULL, *q;
	struct src_file  *v;
	char fileNameBuf[1024];


	for(p = filetab; p < &filetab[nlhdr.nfiles]; p++) {

		for(q=head; q; q=q->next)
			if(!strcmp(q->modname, p->modname)) break;
		if(!q) {
			q = (struct mod_files*) calloc(1, sizeof(*q));
			if(!q) {
				error("out of memory");
			}
			q->modname = p->modname;
			q->first_src = NULL;
			q->next = head;
			head = q;
		}
		v = (struct src_file*)calloc(1, sizeof(*v));
		if(!v) {
				error("out of memory");
		}
		v->filename = p->filename;
		v->next = q->first_src;
		q->first_src = v;
	}

	if(head) {
		Boolean first;

		printf("\n   object files(%s)\t\tsource files\n", objname);
		for(q= head;  q; q=q->next)  {
			char	buf[MAXNAMLEN];
			sprintf(buf, "%s%s", q->modname, ".o");
			printf("    %-20s", buf);
			/* check whether this module was implied or not */
			if(is_sources_sel_list_active()) {
				Sources_sel_item item, *sp;
	
				strcpy(item.obj_file, q->modname);
				strcpy(&item.obj_file[strlen(q->modname)], ".o");
				if(sp = (Sources_sel_item*) lfind((char*)&item, 
						(char*)sources_sel_tab, &sel_tab_nelem,
						sizeof(Sources_sel_item), compar)
				) {
					if(sp->flags&ITEM_IMPLIED) {
						putchar('*');
					}
				}
			}
			first = true;
			for(v= q->first_src;  v; v=v->next)  {
				char *cp = (char*) findsource(v->filename, fileNameBuf);
				if(!first)
					printf("%24s"," ");
				if(cp) {
					/* printf("%s\n", shortname(cp)); */
					printf("%s\n", cp);
				} else {
					printf("%s (?)\n", v->filename);
				}
				first = false;
			}
		}
	} else {
		printf("no sources available");
	}
	printf("\n");

	for(q= head;  q; q=q->next)  {
		for(v= q->first_src;  v; v=v->next)  {
			free(v);
		}
		free(q);
	}
}

struct modincludes {
	char *objfile;
	struct include {
		char *incl_name;
		int incl_cksum;
		struct include *next;
	} *defines;
	struct exclude {
		char *excl_name;
		int excl_cksum;
		struct modincludes *depends_on;
		struct exclude *next;
	} *excludes;
	struct modincludes  *next;
};

public find_implied_files(namelist, stringtab, nsyms)
	int nsyms;
	char *stringtab;
	struct nlist *namelist;
{
	register struct nlist *np;
	register char *name;
	register struct modincludes  *p;
	register struct include *ip;
	register struct exclude *xp;
	struct modincludes  *head = NULL;
	

	for(np= namelist; np <&namelist[nsyms]; np++) if(	
			/* select the namelists we want */
			(
				(np->n_type & N_STAB) && 
				(np->n_type == N_BINCL|| np->n_type == N_EXCL )
			) ||
			(
				(np->n_type & N_TYPE) == N_TEXT && !(np->n_type & N_EXT) 
			)
	){
		if (np->n_un.n_strx > 0) {
			name = &stringtab[np->n_un.n_strx-4];
		} else {
			name = "";
		}
		if(!(np->n_type & N_STAB) ) {
			int n = strlen(name);
			if(n > 2 && name[n-2] == '.' && name[n-1] =='o') {
				p = (struct	 modincludes *) calloc(1, sizeof(struct modincludes));
				if(!p) {
					error("out of memory");
				}
				p->objfile = name;
				p->defines = NULL;
				p->excludes = NULL;
				p->next = head;
				head = p;
			}
		} else {
			if(np->n_type == N_BINCL) {
				ip =  (struct  include *) calloc(1, sizeof(struct include));
				if(!ip) {
					error("out of memory");
				}
				ip->incl_name = name;
				ip->incl_cksum = np->n_value;
				ip->next = head->defines;
				head->defines = ip;
			} else if(np->n_type == N_EXCL) {
				xp =  (struct  exclude *) calloc(1, sizeof(struct exclude));
				if(!xp) {
					error("out of memory");
				}
				xp->excl_name = name;
				xp->excl_cksum = np->n_value;
				xp->depends_on = NULL;
				for(p = head; p ; p= p->next) {
					for(ip = p->defines; ip ; ip= ip->next) {
						if( xp->excl_cksum == ip->incl_cksum &&
							strcmp(xp->excl_name, ip->incl_name) == 0
						) {
							xp->depends_on = p;
							break;
						}
					}
					if(xp->depends_on) 
						break;
				}
				xp->next = head->excludes;
				head->excludes = xp;
			}
		}
	}

	for(p = head; p ; p= p->next) {
		Sources_sel_item item;

		strcpy(item.obj_file, p->objfile);
		/* item.obj_file[strlen(item.obj_file)-2] = '\0'; eliminate the '.o' */

		if(lfind((char*)&item, (char*)sources_sel_tab,
			&sel_tab_nelem, sizeof(Sources_sel_item), compar)) {
			for(xp = p->excludes; xp ; xp= xp->next) {
				if(!xp->depends_on) {/* not possible - absolutely */
					continue; 
				}
				strcpy(item.obj_file, xp->depends_on->objfile);
				/* item.obj_file[strlen(item.obj_file)-2] = '\0'; */
				item.flags = ITEM_IMPLIED;

				lsearch((char*)&item, (char*)sources_sel_tab, 
					&sel_tab_nelem, sizeof(Sources_sel_item), compar);
				if(sel_tab_nelem == sel_tab_size) {
					enlarge_sel_tab();
				}
			}
		}
	}
	for(p = head; p ; p= p->next) {
		for(xp = p->excludes; xp ; xp= xp->next) {
			free(xp);
		}
		for(ip = p->defines; ip ; ip= ip->next) {
			free(ip);
		}
		free(p);
	}
}

private print_inex_graph(head)
	struct modincludes *head;
{
	struct modincludes *p;
	struct include *ip;
	struct exclude *xp;
	for(p = head; p ; p= p->next) {
		printf("%s ",p->objfile);
		if(p->defines) {
			printf("\n\tincludes:");
			for(ip = p->defines; ip ; ip= ip->next) {
				printf("%s ", ip->incl_name);
			}
		}
		if(p->excludes) {
			printf("\n\texcludes:");
			for(xp = p->excludes; xp ; xp= xp->next) {
				printf("%s(%s) ", xp->excl_name, 
						(xp->depends_on ? xp->depends_on->objfile : "??") );
			}
		}
		printf("\n");
	}
}
