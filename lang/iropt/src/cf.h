/*	@(#)cf.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __cf__
#define __cf__

/* typedef struct labelrec LABELREC; */

struct labelrec {
	unsigned labelno;
	BLOCK *block;	/* block where this label is defined */
	TRIPLE *def;	/* LABELDEF triple defining this label */
	LIST *refs;	/* list of LABELREF triples referencing it */
}; /*LABELREC*/ 

extern void	add_labelref(/*tp*/);
extern void	connect_labelrefs();
extern void	form_bb(); 
extern void	cleanup_cf();
extern void	init_cf();

#endif
