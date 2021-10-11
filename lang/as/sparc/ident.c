/* @(#)ident.c 1.1 94/10/31 */

#include <stdio.h>
#include <sun4/a.out.h>

#include "structs.h"
#include "errors.h"
#include  "globals.h"


/*
 * routines to deal with the simple-minded, yet warped
 * .ident strategy we're hacking in here.
 *
 * queue_ident is called by emit_PS_STRG, called by emit_PS, called by emit...
 * it just makes a list of the damn things.
 *
 * ident_to_objfile is called from write_obj_file() in obj.c
 * it computes the size of the ident section, writes the secondary
 * header, and writes the strings. all this happens, of course,
 * after all other processing of the output file is done.
 * good fucking luck
 */

struct ident_string { 
	struct ident_string * next;
	char *                string;
	int		      slen;
};

static struct ident_string *  ident_list = NULL;
static struct ident_string ** ident_list_last_p = &ident_list;

queue_ident( string )
	char *    string;
{
	struct ident_string *p;
	p = (struct ident_string *)as_malloc( sizeof(*p) );
	p->string = string;
	p->next = NULL;
	*ident_list_last_p = p;
	ident_list_last_p = &(p->next);
}

ident_to_objfile(ofp, objfilename)
	FILE *	ofp;
	char *	objfilename;
{
	register struct ident_string *  p;
	struct extra_sections hdr; /* 2dary header from a.out.h */
	int segsize;

	/* compute size of strings. include the trailing \0!! */
	segsize = 0;
	for ( p = ident_list; p != NULL ; p = p->next ){
		segsize += (p->slen = strlen(p->string)+1);
	}
	if ( segsize == 0 )
		return 0 ; /* there ain't none */

	/* write out the header */
	/* we only grot one section */
	hdr.extra_magic = EXTRA_MAGIC;
	hdr.extra_nsects = 1;
	if ( fwrite( (char *)&hdr, sizeof(hdr), 1, ofp ) != 1 ){
		perror(pgm_name);	/* print the sys err message */
		error(ERR_WRITE_ERROR, NULL, 0, objfilename);
		return 0;
	}
	if ( fwrite( (char *)&segsize, sizeof(segsize), 1, ofp ) != 1 ){
		perror(pgm_name);	/* print the sys err message */
		error(ERR_WRITE_ERROR, NULL, 0, objfilename);
		return 0;
	}

	/* (slowly)write out the ident strings */
	for ( p = ident_list; p != NULL ; p = p->next ){
	    if ( fwrite( p->string, 1, p->slen, ofp ) != p->slen ){
		    perror(pgm_name);	/* print the sys err message */
		    error(ERR_WRITE_ERROR, NULL, 0, objfilename);
		    return 0;
	    }
	}
	/* even though we know no-one checks, return the number of bytes written */
	return (sizeof(hdr)+sizeof(segsize)+segsize);
}
