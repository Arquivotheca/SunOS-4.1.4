#ifndef lint
static	char sccsid[] = "@(#)misc.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "cg_ir.h"
#include <stdio.h>

TYPE inttype = { PCC_INT, ALINT/SZCHAR, SZINT/SZCHAR };

static char *ir_alloc_list = (char*) NULL;
char *ckalloc();

/*
 * binary constants are usually treated as byte strings - this is
 * for when it is necessary to deal with them as aligned data
 */
long
l_align(cp1)
	register char *cp1;
{
	union {
		char c[4];
		long w;
	} word;
	register char *cp2 = word.c;
	register int i;

	for(i=0;i<4;i++){
		*cp2++ = *cp1++;
	}
	return(word.w);
}

char *
ckalloc(size)
	unsigned size;
{
	register char *header, *start;
	extern char *malloc();

	header = malloc(size+sizeof(char*));
	if (header == NULL) {
		quit("ckalloc: out of memory\n");
	}
	*((char**)header) = ir_alloc_list;
	ir_alloc_list = header;
	start = header+sizeof(char*);
	bzero(start, (int)size);
	return start;
}

free_ir_alloc_list()
{
	register char *cp;

	if(ir_alloc_list) {
		for(cp=ir_alloc_list; cp; cp = *((char**)cp) ) {
			free(cp);
		}
		ir_alloc_list = (char*) NULL;
	}
}

void
quit(msg)
	char *msg;
{
	fprintf(stderr,"%s\n",msg);
	exit(1);
}

LIST *
new_list()
{
	LIST *lp;
	lp = (LIST*) ckalloc(sizeof(LIST));
	lp->next = lp;
	return(lp);
}

new_label()
{
	static label_counter =0;
	return(++label_counter);
}
 
/*VARARGS1*/
void
quita(str, arg1, arg2, arg3, arg4)
	char *str;
{
	char buf[BUFSIZ];
	(void)sprintf(buf, str, arg1, arg2, arg3, arg4);
	quit(buf);
}

void
delete_list( tail, lp )
	register LIST **tail, *lp;
{
	LIST *lp1_prev, *lp1;
	
	if( lp == LNULL )
		return;
	if( lp->next == lp) {
		*tail = LNULL;
		return;
	}

	/* else, search the *tail list for lp */
	lp1_prev = *tail;
	
	LFOR( lp1, (*tail)->next ) {
		if( lp1 == lp ) {
			lp1_prev->next = lp->next;
			if( *tail == lp ) { /* deleting the tail */
				*tail = lp1_prev;
			}
			return;
		} else {
			lp1_prev = lp1;
		}
	}
	/*  can NOT find lp in *tail list */
	quita("delete_list: can not find the item (0x%x) in the list",
		(int)lp );
}

roundup(i1,i2)
{
	return (i2*((i1+i2-1)/i2));
}


LEAF *
ileaf(l)
	long l;
{
	struct constant const;

	const.isbinary = IR_FALSE;
	const.c.i=l;
	return( leaf_lookup(CONST_LEAF, inttype, (LEAF_VALUE*)&const) );
}

LEAF *
leaf_lookup(class,type,location)
	LEAF_CLASS class;
	TYPE type;
	LEAF_VALUE *location;
{
	register LEAF *leafp;
	register LIST *hash_listp;
	register SEGMENT *sp;
	register ADDRESS *ap,*ap2;
	register struct constant *constp; 
	int index;
	LIST *new_l, *lp;

	if (class == VAR_LEAF || class == ADDR_CONST_LEAF)  {
		ap = (ADDRESS*) location;
		sp = ap->seg;
	} else {
		constp = (struct constant *) location;
	}
	index = hash_leaf(class,location);

	LFOR(hash_listp,leaf_hash_tab[index]) {
		if(hash_listp && LCAST(hash_listp,LEAF)->class == class )  {
			leafp = (LEAF*)hash_listp->datap;
			if(same_irtype(leafp->type,type)==IR_FALSE) continue;
			if(class == VAR_LEAF || class == ADDR_CONST_LEAF) {
				ap2 = &leafp->val.addr;
				if(ap2->seg == ap->seg &&
			   		ap2->offset == ap->offset) 
			   		return(leafp);
				else continue;
			} else {
				if ( IR_ISINT(type.tword)
				    || IR_ISBOOL(type.tword)) {
					if(leafp->val.const.c.i == constp->c.i)
						return(leafp);
					continue;
				} else if ( IR_ISCHAR(type.tword) ) {
					if(strcmp(leafp->val.const.c.cp,
					    constp->c.cp) == 0)
						return(leafp);
					continue;
				}
				quita("no literal pool for tword (0x%x)",
					(int)type.tword);
				continue;
			}
		}
	}

	leafp = (LEAF*) ckalloc(sizeof(LEAF));
	leafp->tag = ISLEAF;
	leafp->leafno = nleaves++;
	leafp->type = type;
	leafp->class=class;
	if(leafp->class == VAR_LEAF || leafp->class == ADDR_CONST_LEAF ) {
		leafp->val.addr = *ap;
		lp = new_list();
		(LEAF*) lp->datap = leafp;
		LAPPEND(sp->leaves,lp);
	} else {
		leafp->val.const = *constp;
	}
	new_l = new_list();
	(LEAF*) new_l->datap = leafp;
	LAPPEND(leaf_hash_tab[index], new_l);
	return(leafp);
}


int
hash_leaf(class,location)
	LEAF_CLASS class;
	LEAF_VALUE *location;
{
	register unsigned key;
	register char *cp;
	register SEGMENT *sp;
	register ADDRESS *ap;
	register struct constant *constp;

	key = 0;

	if (class == VAR_LEAF || class == ADDR_CONST_LEAF)  {
		ap = (ADDRESS*) location;
		sp = ap->seg;
		if(sp->descr.builtin == BUILTIN_SEG) {
			key = ( ( (int) sp->descr.class << 16) | (ap->offset) );
		} else {
			for(cp=sp->name;*cp!='\0';cp++) key += *cp;
			key = ( (key << 16) | (ap->offset) );
		}
	} else { /* class == CONST_LEAF */
		constp = (struct constant*) location;
		key = (unsigned) constp->c.i; 
	}

	key %= LEAF_HASH_SIZE;
	return( (int) key);
}

BOOLEAN
same_irtype(p1,p2)
TYPE p1,p2;
{
	if(p1.tword == p2.tword ) {
		if(PCC_BTYPE(p1.tword) == PCC_STRTY ) {
			if(p1.size != p2.size) return(IR_FALSE);
			if(p1.align != p2.align) return(IR_FALSE);
		}
		return(IR_TRUE);
	}
	return(IR_FALSE);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int
firstreg(regclass)
    SEGCLASS regclass;
{
#   if TARGET == MC68000
	switch(regclass) {
	case DREG_SEG: return D0;
	case AREG_SEG: return A0;
	case FREG_SEG: return FP0;
	}
#   endif
#   if TARGET == SPARC
	switch(regclass) {
	case DREG_SEG: return RG0;
	case FREG_SEG: return FR0;
	}
#   endif
#   if TARGET == I386
        switch(regclass) {
        case DREG_SEG: return REG_EAX;
        case FREG_SEG: return REG_FP0;
        }
#   endif
    quita("not a register segment");
    /*NOTREACHED*/
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

char *
regname(n)
    int n;
{
#   if TARGET == I386
    /* found in pcc/i386/local2.c */	
    extern char *rnames[];
#   else
    static char buf[10];
    char *fmt;
#   endif

#   if TARGET == MC68000
	if (n <= D7) {
	    fmt = "d%d"; n -= firstreg(DREG_SEG);
	} else if (n <= SP) {
	    fmt = "a%d"; n -= firstreg(AREG_SEG);
	} else {
	    fmt = "fp%d"; n -= firstreg(FREG_SEG);
	}
#   endif
#   if TARGET == SPARC
	if (n <= RO7) {
	    fmt = "%%o%d"; n -= firstreg(DREG_SEG);
	} else if (n <= RL7) {
	    fmt = "%%l%d"; n -= RL0;
	} else if (n <= RI7) {
	    fmt = "%%i%d"; n -= RI0;
	} else {
	    fmt = "%%f%d"; n -= firstreg(FREG_SEG);
	}
#   endif
#   if TARGET != I386
    (void)sprintf(buf, fmt, n);
    return buf;
#   else
    return rnames[n];
#   endif
}


proc_init()
{
register LIST **lpp, **lptop;

	lptop = &leaf_hash_tab[LEAF_HASH_SIZE];
	for(lpp=leaf_hash_tab; lpp < lptop;) *lpp++ = LNULL;
}
