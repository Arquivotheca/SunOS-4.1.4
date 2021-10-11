#ifndef lint
static  char sccsid[] = "@(#)misc.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "page.h"
#include "misc.h"
#include "recordrefs.h"
#include <stdio.h>

/* global */

TRIPLE *free_triple_lifo;
BLOCK  *free_block_lifo;

TYPE inttype =    { INT,      ALINT/SZCHAR,    SZINT/SZCHAR    };
TYPE floattype =  { FLOAT,    ALFLOAT/SZCHAR,  SZFLOAT/SZCHAR  };
TYPE doubletype = { DOUBLE,   ALDOUBLE/SZCHAR, SZDOUBLE/SZCHAR };
TYPE undeftype =  { UNDEF };

/* from libc */
extern char *malloc(/*nbytes*/);

BOOLEAN
leaves_overlap(leafA, leafB)
    LEAF *leafA;
    LEAF *leafB;
{
    int   leafAoffset = leafA->val.addr.offset;
    int   leafBoffset = leafB->val.addr.offset;
    int   leafAsize = leafA->type.size;
    int   leafBsize = leafB->type.size;
 
    if (((leafBoffset + leafBsize) > leafAoffset) &&
        (leafBoffset < (leafAoffset + leafAsize))) {
        return IR_TRUE;
    }
    return IR_FALSE;
}

/*
 * search backwards for a STMT triple, and if one is found, use
 * it to give "filename, line number" coordinates for a warning
 * or error message.
 */
static void
where(tp)
        TRIPLE *tp;
{
        TRIPLE *start;
        char *filename;
        int lineno;
 
        start = tp;
        for (tp = start->tprev; tp != start; tp = tp->tprev) {
                if (tp->op == IR_STMT) {
                        filename = ((LEAF*)tp->left)->val.const.c.cp;
                        lineno = ((LEAF*)tp->right)->val.const.c.i;
                        (void)fprintf(stderr, "%s, line %d: ",
                            filename, lineno);
                        return;
                }
        }
}
 
/*
 * issue a warning message with a variable number of arguments (up to 6).
 * The triple (tp) is the site where the reason for the warning was
 * detected.  The "suppress_warnings" flag is set for the entire
 * compilation by the command line option -w.
 */
/*VARARGS2*/
void
warning(tp, msg, a, b, c, d, e, f)
        TRIPLE *tp;
        char *msg;
{
        char buf[BUFSIZ];
/*
        if (suppress_warnings)
                return;
*/
        (void)fflush(stdout);
        (void)fflush(stderr);
        if (tp != TNULL) {
            where(tp);
        }
        (void)sprintf(buf, msg, a, b, c, d, e, f);
        (void)fprintf(stderr,"warning: %s\n", buf);
}

/*
 * delete a triple, or mark it for later deletion, according
 * to the value of 'delay'.  Assume the expr reachdef and canreach
 * chains have been computed.  bp is passed along to update the block
 * descriptor if the last triple in the block is deleted.
 */
void
unlist_triple(delay, tp, bp)
	BOOLEAN delay;
	register TRIPLE *tp;
	BLOCK *bp;
{
	if( tp == TNULL)
		return;
	if( tp->canreach )
	{
		fix_canreach( tp, tp->canreach );
		tp->canreach  = LNULL;
	}
	if( tp->reachdef1 )
		fix_del_reachdef( delay, tp, tp->reachdef1, TNULL );
	if( tp->reachdef2 )
		fix_del_reachdef( delay, tp, tp->reachdef2, TNULL );
	if (delay)
	{	/* do it later */
		tp->inactive = IR_TRUE;
	}
	else
	{	/* do it now */
		(void)free_triple(tp, bp);
	}
}	

/*
 * Just deallocate a triple. Period.
 * Returns the predecessor of the deallocated triple,
 * or TNULL if there isn't one.
 */
TRIPLE *
free_triple(tp, bp)
	TRIPLE *tp;
	BLOCK *bp;
{
	TRIPLE *tprev,*tnext;

        if (bp != NULL) {
                if( tp == tp->tnext )
                {
                        /* the only triple in this block */
                        if(tp != tp->tprev)
                        {
                                quita("free_triple(0x%x): corrupted block", tp);
                                /*NOTREACHED*/
                        }
                        bp->last_triple = TNULL;
                }
                else if( tp == bp->last_triple )
                {
                        /* deleting the last triple */
                        bp->last_triple = tp->tprev;
                }
        } else if (ISOP(tp->op, BRANCH_OP)) {
                /*
                 * this should be the last triple in the
                 * block.  If we don't have a block pointer
                 * we cannot continue (because the block's
                 * last triple has disappeared, with no way
                 * of updating block's last_triple field.
                 */
                quita("free_triple(0x%x): expecting to get a block pointer that is not nil", tp);
        }
        if( tp->tprev )
                tp->tprev->tnext = tp->tnext;
        if( tp->tnext )
                tp->tnext->tprev = tp->tprev;
        tprev = tp->tprev;
        tnext = tp->tnext;
        (void)bzero((char*)tp, sizeof(TRIPLE));
        tp->left = (IR_NODE *)free_triple_lifo;
        free_triple_lifo = tp;
        /*
         * restore tp->tprev, tp->tnext, because the implementation
         * of TFOR(...) *requires* the old value of tnext to be
         * preserved, even though the triple is supposedly out of
         * the mainline triples structure.
         *
         * 6.0E+4931 on the vomit meter.
         */
        tp->tprev = tprev;
        tp->tnext = tnext;
        return tprev;
}

/*
 * Remove the triples that have been marked 'inactive'
 * by delete_triple() with the 'delay' parameter = IR_TRUE.
 * This two-phase approach is used in cse, because of problems
 * that arise from recursively deleting triples on the fly
 * from within a TFOR(...) loop.
 */
void
remove_dead_triples()
{
	BLOCK *bp;
	TRIPLE *tp, *argp;

	for( bp = entry_block; bp; bp = bp->next )
	{	/* for each basic block */
		TFOR( tp, bp->last_triple->tnext )
		{	/* for each triple */
			if( ( tp->op == IR_SCALL || tp->op == IR_FCALL ) && tp->right )
			{	/* deal with parameter list */
				TFOR(argp, (TRIPLE*)tp->right)
				{
					if (argp->inactive)
					{
						argp = free_triple(argp, bp);
					}
				}
			}
			if( tp->inactive )
			{
				tp = free_triple(tp, bp);
			}
		}
	}
}

TRIPLE *
append_triple(after,op,arg1,arg2,type)
	TRIPLE *after;
	IR_OP op;
	IR_NODE *arg1, *arg2;
	TYPE type;
{
	register TRIPLE *tp;

	if(! free_triple_lifo) {
		tp = (TRIPLE*) new_triple(IR_FALSE);
	} else {
		tp=free_triple_lifo;
		free_triple_lifo = (TRIPLE*) tp->left;
	}
	(void)bzero((char*)tp,sizeof(TRIPLE));

	tp->tprev = tp->tnext = tp;
	tp->tag = ISTRIPLE;
	tp->op = op;
	tp->left =  arg1;
	tp->right =  arg2;
	tp->type = type;
	tp->tripleno = ntriples++;
	if(after == TNULL) {
		tp->tprev = tp->tnext = tp;
	} else {
		TAPPEND(after,tp);
	}
	return(tp);
}

int
new_label()
{
	static label_counter = 77000;
	return(++label_counter);
}

void
move_block(after,block)
	BLOCK *after,*block;
{
	/*
	 ** ensure that a block appears in a given place in "next", ie code
	 ** generation, order. Done to keep unreachable code where
	 **	people expect it to be
	 */
	register BLOCK *bp;
	
	if( block == (BLOCK*) NULL || after == (BLOCK*) NULL ) {
		return;
	}
	
	bp = block->prev;
	if( after == bp || bp == (BLOCK *) NULL) { /* already in order or moving first block */
		return;
	}

	if( after->prev == block ) {/* switching after and block */
		after->prev = bp;
		bp->next = after;
		block->next = after->next;
		block->prev = after;
		if( block->next ) {
			block->next->prev = block;
		}
	} else {
		bp->next = block->next;
		if( block->next ) {
			block->next->prev = bp;
		}
		block->next = after->next;
		block->prev = after;
		after->next->prev = block;
	}
	after->next = block;
	
	if(bp->next == (BLOCK*) NULL) {
		last_block = bp;
	}
}

BLOCK *
new_block()
{
	BLOCK *bp;

	if (free_block_lifo) {
		bp = free_block_lifo;
		free_block_lifo = free_block_lifo->next;
		(void)bzero((char*)bp, sizeof(BLOCK));
	} else {
		bp = (BLOCK*) ckalloc(1,sizeof(BLOCK));
	}
	bp->blockno = nblocks++;
	if (nblocks >= MAX_NBLOCKS) {
		quit("Too many blocks");
		/*NOTREACHED*/
	}
	/* is this the first block request for this ir_file ? */
	if(last_block != (BLOCK*) NULL) {		
		last_block->next = bp;
		bp->prev = last_block;
	}
	last_block = bp;
	bp->tag=ISBLOCK;
	bp->loop_weight = 1;
	return(bp);
}


LIST *
copy_list(tail,lqp) 
	register LIST *tail;
	register LISTQ *lqp;
{
	/* Because the external pointer always points to the */
	/* "tail", so do the copy list and return the tail pointer */
	register LIST *lp1, *lp2, *last;

	if( tail == LNULL )
		return( LNULL );
	last = LNULL;
	LFOR(lp1,tail->next) {
		lp2=NEWLIST(lqp);
		lp2->datap = lp1->datap;
		LAPPEND(last,lp2);
	}
	return(last);
}


/*
 * a variant of copy list that ensures the new list is
 * sorted in increasing order and duplicates are deleted
 */

LIST *
order_list(tail,lqp) 
	register LIST *tail;
	LISTQ *lqp;
{
	register LIST *lp1, *lp2;
	LIST *last;

	if( tail == LNULL )
		return( LNULL );
	last = LNULL;
	LFOR(lp1,tail->next) {
		if( last == LNULL ) {
			last = NEWLIST(lqp);
			last->datap = lp1->datap;
		} else if( last->datap < lp1->datap ) {
			lp2=NEWLIST(lqp);
			lp2->datap = lp1->datap;
			LAPPEND(last, lp2);
		} else {
			(void)insert_list(&last, lp1->datap, lqp);
		}
	}
	return(last);
}

/* 
 * merge two lists sorted by datap value; the args are assumed to
 * point to the largest (last) elements, and the largest element
 * of the resulting merged list is the value returned.  Both
 * argument lists are destroyed.
 */
LIST *
merge_lists(list1,list2)
	register LIST *list1,*list2;
{
	register LIST *lp1 = LNULL;
	register LIST *lp2 = LNULL;
	register LIST *select = LNULL;
	register LIST *merged = LNULL; 

	/* the usual klutziness about advancing to smallest*/
	if(list1) {
		lp1 = list1->next;
	} else {
		return( list2 );
	}
	if(list2) {
		lp2 = list2->next;
	} else {
		return( list1 );
	}
	while(lp1 || lp2) {
		if(lp1 == LNULL) {
			select = lp2;
			lp2 = ( lp2->next == list2->next ? LNULL : lp2->next);
		} else if(lp2 == LNULL) {
			select = lp1;
			lp1 = ( lp1->next == list1->next ? LNULL : lp1->next);
		} else if(lp1->datap < lp2->datap) {
			select = lp1;
			lp1 = ( lp1->next == list1->next ? LNULL : lp1->next);
		} else if(lp1->datap > lp2->datap) {
			select = lp2;
			lp2 = ( lp2->next == list2->next ? LNULL : lp2->next);
		} else {
			select = lp2;
			lp1 = ( lp1->next == list1->next ? LNULL : lp1->next);
			lp2 = ( lp2->next == list2->next ? LNULL : lp2->next);
		}
		/* take the lp out of the original list */
		select->next = select;
		LAPPEND(merged,select);
	}
	return(merged);
}

/*
 * insert an item in a list maintained in increasing datap order
 * as usual the argument list references the largest (last) element
 */
BOOLEAN
insert_list(lastp,item,lqp)
	register LIST **lastp;
	register LDATA *item;
	LISTQ *lqp;
{
	register LIST *new_l, *lp, *last, *lp_prev;

	if(*lastp == (LIST*) NULL) {
		new_l = NEWLIST(lqp);
		new_l->datap = item;
		LAPPEND(*lastp,new_l);
		return(IR_TRUE);
	} else {
		lp_prev = last = *lastp;
		LFOR(lp,last->next) {
			if(lp->datap == item) return(IR_FALSE);
			else if( item < lp->datap) {
				new_l = NEWLIST(lqp);
				new_l->datap = item;
				LAPPEND(lp_prev,new_l);
				return(IR_TRUE);
			}
			lp_prev = lp;
		}
		new_l = NEWLIST(lqp);
		new_l->datap = item;
		LAPPEND(last,new_l);
		*lastp = last;
		return(IR_TRUE);
	}
}

/*VARARGS1*/
void
quita(msg, a, b, c, d, e, f)
	char *msg;
{
	char buf[BUFSIZ];
	(void)sprintf(buf, msg, a, b, c, d, e, f);
	quit(buf);
}

void
quit(msg)
	char *msg;
{
	(void)fflush(stdout);
	(void)fflush(stderr);
	if(strlen(msg) == 0 ) exit  (0);
	else {
		(void)fprintf(stderr,"compiler(iropt) error:\t%s \n",msg);
		exit(1);
	}
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

/* 
	don't really need this for vars leafno ios sufficient FIXME
*/

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
		constp = (struct constant*) location;
	}
	index = hash_leaf(class,location);

	LFOR(hash_listp,leaf_hash_tab[index]) {
		if(hash_listp && LCAST(hash_listp,LEAF)->class == class )  {
			leafp = (LEAF*)hash_listp->datap;
			if(same_irtype(leafp->type,type)==IR_FALSE) continue;
			if(class == VAR_LEAF || class == ADDR_CONST_LEAF) {
				ap2 = &leafp->val.addr;
				if(ap2->seg == ap->seg &&
			   	   ap2->offset == ap->offset &&
                                   ap2->labelno == ap->labelno) 
			   		return(leafp);
				else continue;
			} else {
				if ( ISINT(type.tword) || ISBOOL(type.tword)) {
					if(leafp->val.const.c.i == constp->c.i) return(leafp);
					continue;
				} else if ( ISCHAR(type.tword) ) {
					if(strcmp(leafp->val.const.c.cp,constp->c.cp) == 0) return(leafp);
					continue;
				}
				continue;
			}
		}
	}

	leafp = (LEAF*) ckalloc(1, sizeof(LEAF));
	leafp->tag = ISLEAF;
	leafp->leafno = nleaves++;
	leafp->pointerno = leafp->elvarno = -1;
	if(leaf_top) {
		leaf_top->next_leaf = leafp;
	}
	leaf_top = leafp;
	leafp->next_leaf = (LEAF*) NULL;
	leafp->type = type;
	leafp->class=class;
	if(leafp->class == VAR_LEAF || leafp->class == ADDR_CONST_LEAF ) {
		leafp->val.addr = *ap;
		if(leafp->class == VAR_LEAF) {
			lp = NEWLIST(proc_lq);
			(LEAF*) lp->datap = leafp;
			LAPPEND(sp->leaves,lp);
		}
	} else {
		leafp->val.const = *constp;
	}
	new_l = NEWLIST(proc_lq);
	(LEAF*) new_l->datap = leafp;
	LAPPEND(leaf_hash_tab[index], new_l);
	return(leafp);
}

/* generate int constant with default integral type = inttype */

LEAF *
ileaf(i)
	int i;
{
	struct constant const;

	const.isbinary = IR_FALSE;
	const.c.i=i;
	return( leaf_lookup(CONST_LEAF, inttype, (LEAF_VALUE *)&const));
}

/* generate int constant with specified type */

LEAF *
ileaf_t(i, t)
	int i;
	TYPE t;
{
	struct constant const;

	const.isbinary = IR_FALSE;
	const.c.i=i;
	return( leaf_lookup(CONST_LEAF, t, (LEAF_VALUE *)&const));
}

/*
 * add bytes to the current string buffer.  Get a new one if needed.
 */
static char *
add_bytes(strp,len)
    register char *strp;
    register int len;
{
    int size;
    STRING_BUFF *tmp;
    register char *start, *cursor;

    if( !string_buff || ((string_buff->top + len -1) >= string_buff->max)) {
        size = ( len > STRING_BUFSIZE ? len : STRING_BUFSIZE);
        tmp = (STRING_BUFF*) malloc((unsigned)sizeof(STRING_BUFF)+size);
        if (tmp == NULL) {
                quita("memory allocation exceeded");
                /*NOTREACHED*/
        }
        tmp->next = (STRING_BUFF*) NULL;
 
        if(!string_buff) {
            first_string_buff = tmp;
        } else {
            string_buff->next = tmp;
        }
        string_buff = tmp;
 
        string_buff->data= string_buff->top = (char*)tmp + sizeof(STRING_BUFF);
        string_buff->max = string_buff->top + size;
    }
    cursor=string_buff->top;
    start = cursor;
    string_buff->top += len;
    bcopy(strp, cursor, len);
    return start ;
}
 
LEAF *
fleaf_t(f, t)
    double f;
    TYPE t;
{
    struct constant const;
    float fval;
 
    bzero((char*)&const, sizeof(const));
    const.isbinary = IR_TRUE;
    if (t.tword == FLOAT) {
        /* convert to single precision (bletch) */
        fval = (float)f;
        const.c.bytep[0] = add_bytes((char*)(&fval), sizeof(float));
    } else {
        const.c.bytep[0] = add_bytes((char*)(&f), sizeof(double));
    }
    return leaf_lookup(CONST_LEAF, t, (LEAF_VALUE *)&const);
}

/* create a string constant leaf */
LEAF *
cleaf(s)
	char *s;
{
	struct constant const;
	TYPE t;

	t.tword = STRING;
	t.size = strlen(s);
	t.align = sizeof(char);
	const.isbinary = IR_FALSE;
	const.c.cp = new_string(s);
	return( leaf_lookup(CONST_LEAF,t, (LEAF_VALUE *)&const) );
}

LEAF*
reg_leaf(leafp,segno,regno)
	LEAF *leafp;
	int segno,regno;
{
	ADDRESS addr;
	register SEGMENT *segp;
	register LEAF *new_leaf;
	register LIST *lp;
	register int index;

	(void)bzero((char*)&addr, sizeof(ADDRESS));
	new_leaf = (LEAF*) ckalloc(1, sizeof(LEAF));
	segp = new_leaf->val.addr.seg = &seg_tab[segno];
	lp = NEWLIST(proc_lq);
	(LEAF*) lp->datap = leafp;
	LAPPEND(segp->leaves,lp);
	new_leaf->val.addr.offset = regno;
	new_leaf->val.addr.labelno = 0;
	/* always create a new leaf for it, */
	/* because same register var could have different */
	/* depends list */
	
	new_leaf->tag = ISLEAF;
	new_leaf->leafno = nleaves++;
	new_leaf->pointerno = new_leaf->elvarno = -1;
	if(leaf_top) {
		leaf_top->next_leaf = new_leaf;
	}
	leaf_top = new_leaf;
	new_leaf->next_leaf = (LEAF*) NULL;
	new_leaf->type = leafp->type;
	new_leaf->class=VAR_LEAF;
	index = hash_leaf( VAR_LEAF, (LEAF_VALUE*)&new_leaf->val.addr );
	lp = NEWLIST(proc_lq);
	(LEAF*) lp->datap = new_leaf;
	LAPPEND(leaf_hash_tab[index], lp);
	return(new_leaf);
}

LEAF *
new_temp( type )
	TYPE type;
{
	ADDRESS addr;
	register SEGMENT *segp;
	char name[40];
	static struct segdescr_st auto_descr = {
		AUTO_SEG, STG_SEG, USER_SEG, LCLSTG_SEG
	};

	(void)bzero((char*)&addr,sizeof(ADDRESS));
	segp = (SEGMENT*)ckalloc(1, sizeof(SEGMENT));
	segp->len = type_size(type);
#if TARGET==SPARC
	switch(type.tword) {
	case DOUBLE:
	case COMPLEX:
	case DCOMPLEX:
		segp->align = SZDOUBLE/SZCHAR;
		break;
	default:
		segp->align = type_align(type);
		break;
	}
#else
	segp->align = type_align(type);
#endif
	segp->len = roundup((unsigned)segp->len, (unsigned)segp->align);
	(void)sprintf(name, "TMP_SEG.%d.%d", nseg, segp->len);
	segp->name = new_string(name);
	segp->descr = auto_descr;
	segp->base = seg_tab[AUTO_SEGNO].base;
	segp->segno = nseg++;
	segp->next_seg = NULL;
	last_seg->next_seg = segp;
	last_seg = addr.seg = segp;
	return leaf_lookup(VAR_LEAF,type, (LEAF_VALUE *)&addr);
}

void
fix_del_reachdef( delay, triple, reachdef, base )
	BOOLEAN delay;
	LIST *reachdef; TRIPLE *triple, *base;
{ /* same as fix_reachdef except it has all the expr_df information, */
  /* so do the recursive deletion. */

	LIST *lp, *list_ptr;
	TRIPLE *tp;

	LFOR(lp, reachdef)
	{
		tp = LCAST(lp, EXPR_REF)->site.tp;
		LFOR(list_ptr, tp->canreach)
		{
			if( LCAST(list_ptr, EXPR_REF)->site.tp == triple )
			{
				if( list_ptr->next == list_ptr /* only can reaches here */ &&
								tp != base )
				{ 
					tp->canreach  = LNULL;
					if( can_delete( tp ) )
					{
						delete_triple( delay, tp,
							LCAST(lp, EXPR_REF)->site.bp );
						tp = TNULL;
					}
				}
				else
				{
					delete_list( &tp->canreach, list_ptr );
					if( tp != base && tp->op == IR_ASSIGN &&
							tp->right->operand.tag == ISTRIPLE &&
							tp->canreach &&
							tp->canreach == tp->canreach->next &&
							LCAST( tp->canreach, EXPR_REF)->site.tp == (TRIPLE *)tp->right  &&
							((TRIPLE *)tp->right)->canreach &&
							((TRIPLE *)tp->right)->canreach ==
							((TRIPLE *)tp->right)->canreach->next )
					{
						if( can_delete( tp ) )
						{
							delete_triple( delay, tp,
								LCAST(lp, EXPR_REF)->site.bp);
							tp = TNULL;
						}
					}
				}
				break; /* exit from LFOR(...,tp->canreach) */
			}
		}
	}
}

void
fix_reachdef( delay, triple, reachdef, base )
	BOOLEAN delay;
	TRIPLE *triple;
	LIST *reachdef;
	TRIPLE *base;
{
	LIST *lp, *list_ptr;
	TRIPLE *tp;

	LFOR(lp, reachdef)
	{
		tp = LCAST(lp, EXPR_REF)->site.tp;
		LFOR(list_ptr, tp->canreach)
		{
			if( LCAST(list_ptr, EXPR_REF)->site.tp == triple )
			{
				if( list_ptr->next == list_ptr /* only can reach here */ &&
					tp != base )
				{ 
					tp->canreach  = LNULL;
					if( can_delete( tp ) )
					{
						delete_triple( delay, tp,
							LCAST(lp, EXPR_REF)->site.bp );
						tp = TNULL;
					}
				}
				else
					delete_list( &tp->canreach, list_ptr );
				break;
			}
		}
	}
}

void
fix_canreach( triple, can_reach ) LIST *can_reach; TRIPLE *triple;
{
	LIST *lp, *list_ptr;
	TRIPLE *tp;

	LFOR(lp, can_reach)
	{ /* update the reachdef */
		tp = LCAST( lp, EXPR_REF )->site.tp;
		if( tp->left && tp->left->operand.tag == ISTRIPLE &&
			((TRIPLE *)tp->left)->expr == triple->expr )
		{
			LFOR( list_ptr, tp->reachdef1 )
			{
				if( LCAST(list_ptr, EXPR_REF )->site.tp ==
						triple )
				{ /* no longer can reachs here */
					delete_list( &tp->reachdef1,list_ptr );
					break;
				}
			}
			continue;
		}

		if( ISOP( tp->op, BIN_OP) && tp->right && tp->right->operand.tag == ISTRIPLE &&
			((TRIPLE *)tp->right)->expr == triple->expr )
		{
			LFOR( list_ptr, tp->reachdef2 )
			{
				if( LCAST(list_ptr, EXPR_REF )->site.tp ==
						triple )
				{ /* no longer can reachs here */
					delete_list( &tp->reachdef2,list_ptr );
					break;
				}
			}
		}
	}

}

type_align(type)
	TYPE type;
{
	if (ISFTN(type.tword)) {
		quita("type_align: alignment of function (0x%x)",
			type.tword);
	}
	if (type.tword == BITFIELD) {
		return ALINT/SZCHAR;
	}
	return type.align;
}

type_size(type)
	TYPE type;
{
	if (ISFTN(type.tword)) {
		quita("type_size: size of function (0x%x)",
			type.tword);
	}
	if (type.tword == BITFIELD) {
		return SZINT/SZCHAR;
	}
	return type.size;
}

BOOLEAN
same_irtype(t1,t2)
	TYPE t1,t2;
{
	if(t1.tword == t2.tword ) {
		return((BOOLEAN)(t1.size == t2.size && t1.align == t2.align));
	}
	if (ISPTR(t1.tword) && ISPTR(t2.tword)) {
		return(IR_TRUE);
	}
	return(IR_FALSE);
}

void
delete_triple( delay, tp, bp )
	BOOLEAN delay; TRIPLE *tp; BLOCK *bp;
{
	register LIST *lp;

	if (tp->op == IR_ERR)
	{
		quit("Deleting an already-deleted triple");
		/*NOTREACHED*/
	}
	if (tp->inactive)
	{
		return;
	}
	LFOR( lp, tp->implicit_use )
	{ /* delete the implicit use with the triple */
		delete_triple( delay, (TRIPLE*)lp->datap, bp );
	}
	unlist_triple( delay, tp, bp );
}

/*
 * delete a block.
 */
void
delete_block(bp)
	BLOCK *bp;
{
	if (bp != NULL) {
		while (bp->last_triple) {
			delete_triple(IR_FALSE, bp->last_triple, bp);
		}
		if (bp->prev)
			bp->prev->next = bp->next;
		if (bp->next)
			bp->next->prev = bp->prev;
		if (bp == last_block) {
			last_block = bp->prev;
		}
		(void)bzero((char*)bp, sizeof(BLOCK));
		bp->next = free_block_lifo;
		free_block_lifo = bp;
		nblocks--;
	}
}

void
remove_triple( tp, bp ) TRIPLE *tp; BLOCK *bp;
{ /* this is almost like the delete triple, except theat */
  /* this routine won't do the recursive deletion, becasue */
  /* there is no expr information. */

	register LIST *lp1, *lp2;
	register TRIPLE *triple;

	LFOR( lp1, tp->implicit_use )
	{ /* delete the implicit use with the triple */
		remove_triple( (TRIPLE*)lp1->datap, bp );
	}

	/*  fix reachdef and canreach */
	if( tp->left->operand.tag == ISTRIPLE )
	{ /* at the time this routine be called,
	   * all triples must be in the TREE format
	   */
		if( can_delete( (TRIPLE*)tp->left ) )
		{
			remove_triple( (TRIPLE*)tp->left, bp );
			tp->left = NULL;
		}
	}
	else
	{ /* it is a leaf, and we do compute the var_df */
		LFOR( lp1, tp->reachdef1 )
		{
			triple = LCAST( lp1, VAR_REF )->site.tp;
			LFOR( lp2, triple->canreach )
			{
				if( LCAST(lp2, VAR_REF)->site.tp == tp )
				{ BLOCK *bp;
					bp = LCAST(lp2, VAR_REF)->site.bp;
					delete_list( &triple->canreach, lp2 );
					if( triple->canreach == LNULL && triple != tp
						&& can_delete( triple ) )
					{
						remove_triple( triple, bp );
						triple = TNULL;
					}
					break; /* exit LFOR(..., triple->canreach) loop */
				}
			}
		}
	}

	if( ISOP( tp->op, BIN_OP ) )
	{
		if( tp->right->operand.tag == ISTRIPLE )
		{
			if( can_delete( (TRIPLE*)tp->right ) )
			{
				remove_triple( (TRIPLE*)tp->right, bp );
				tp->right = NULL;
			}
		}
		else
		{ /* a LEAF */
			LFOR( lp1, tp->reachdef2 )
			{
				triple = LCAST( lp1, VAR_REF )->site.tp;
				LFOR( lp2, triple->canreach )
				{
					if( LCAST(lp2, VAR_REF)->site.tp == tp )
					{ BLOCK *bp;
						bp = LCAST(lp2, VAR_REF)->site.bp;
						delete_list( &triple->canreach, lp2 );
						if( triple->canreach == LNULL && triple != tp
							&& can_delete( triple ) )
						{
							remove_triple( triple, bp );
							triple = TNULL;
						}
						break; /* exit LFOR(..., triple->canreach) loop */
					}
				}
			}
		}
	}
	(void)free_triple(tp, bp);
}

void
delete_list( tail, lp ) register LIST **tail, *lp;
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
	quita("delete_list: can not find the item >%X< in the list", lp );
}

LOCAL
sideffect (tp)
	TRIPLE *tp;
{
	LEAF *leafp;
	int left = 0;
	int right = 0;

	if(may_cause_exception(tp))
		return 1;
	if(tp->left) switch(tp->left->operand.tag) {
		case ISTRIPLE:
			left = sideffect((TRIPLE*)tp->left);
			break;
		case ISLEAF:
			leafp = (LEAF*)tp->left;
			left = (leafp->class == VAR_LEAF) && EXT_VAR(leafp);
			break;
		default:
			quit("bad left operand: sideffect");
	}
	if(left)
		return 1;
	if(tp->right) switch(tp->right->operand.tag) {
		case ISTRIPLE:
			right = sideffect((TRIPLE*)tp->right);
			break;
		case ISLEAF:
			leafp = (LEAF*)tp->right;
			right = (leafp->class == VAR_LEAF) && EXT_VAR(leafp);
			break;
		default:
			quit("bad right operand: sideffect");
	}
	return right;
}

BOOLEAN
can_delete( tp ) register TRIPLE *tp;
{ /* 
   *This routine test see if tp can be deleted.
   * Right now, we assume that we can not delete
   * any triple which assign a value to a STATIC or
   * EXTERN variable savely.  Do not delete the triple
   * that has implicit definations.
   */
	register LEAF *func;

	if( tp->op == IR_FCALL )
	{ /*if it's INTR_FUNC then delete it */
		func = (LEAF *)tp->left;
		if( func->class == CONST_LEAF &&
				func->func_descr == INTR_FUNC )
			return( IR_TRUE );
		else {
			return( IR_FALSE );
		}
	}

	if( tp->implicit_def )
		return( IR_FALSE );
				 
	if( ISOP( tp->op, VALUE_OP )){
		if (partial_opt == IR_TRUE && tp->expr == (EXPR*)NULL){
			return(IR_FALSE);
		}
		return( IR_TRUE );
	}

	if( tp->op != IR_ASSIGN && tp->op != IR_ISTORE )
		return( IR_FALSE );

	if( partial_opt == IR_TRUE ){
		if( tp->op == IR_ISTORE )
			return ( IR_FALSE );

		if( tp->left->leaf.class == VAR_LEAF && EXT_VAR((LEAF*)tp->left)) 
			return (IR_FALSE);

		if( tp->right->operand.tag == ISLEAF
			&& tp->right->leaf.class == VAR_LEAF && EXT_VAR((LEAF*)tp->right)) 
			return (IR_FALSE);

		if( tp->right->operand.tag == ISTRIPLE ) {
			if (sideffect((TRIPLE*)tp->right)) {
				return (IR_FALSE);
			}
			if (!can_delete((TRIPLE*)tp->right)) {
				return (IR_FALSE);
			}
		}
	}
	return( IR_TRUE );
}

/*
 * may_cause_exception(tp) returns TRUE if evaluation of the
 * operator (tp) may cause a run time exception.   A triple with
 * this property should not be moved to another block unless the
 * destination block is known to be executed iff the original block
 * is executed.  In particular, triples with this property should
 * not be moved to loop preheaders unless they are originally in a
 * block that dominates all of the loop's exits.
 */
BOOLEAN
may_cause_exception(tp)
	TRIPLE *tp;
{
	TWORD t;

	switch(tp->op) {
	case IR_FCALL:		/* who knows */
	case IR_ISTORE:		/* memory fault */
	case IR_IFETCH:		/* memory fault */
	case IR_DIV:		/* division by zero */
	case IR_REMAINDER:	/* division by zero */
		/*
		 * These operations may cause exceptions,
		 * independent of range checking or other
		 * code generation options.
		 */
		return IR_TRUE;
	case IR_PLUS:
	case IR_MINUS:
	case IR_MULT:
	case IR_NEG:
		/*
		 * These operators may cause arithmetic
		 * exceptions if overflow checking is enabled
		 * during code generation.
		 */
		if (tp->chk_ovflo) {
			return IR_TRUE;
		}
		/*FALL THROUGH*/
	default:
		/*
		 * Floating point exceptions.
		 * These are controlled dynamically, so
		 * we can't predict at compile time whether
		 * an exception may occur at run time.
		 */
		if (ISOP(tp->op, FLOAT_OP)) {
			if (ISOP(tp->op, VALUE_OP)) {
				t = tp->type.tword;
				if(IR_ISREAL(t) || IR_ISCOMPLEX(t)) {
					return IR_TRUE;
				}
			}
			if (ISOP(tp->op, (MOD_OP|USE1_OP))) {
				t = tp->left->operand.type.tword;
				if (IR_ISREAL(t) || IR_ISCOMPLEX(t)) {
					return IR_TRUE;
				}
			}
			if (ISOP(tp->op, (USE2_OP))) {
				t = tp->right->operand.type.tword;
				if (IR_ISREAL(t) || IR_ISCOMPLEX(t)) {
					return IR_TRUE;
				}
			}
		}
		break;
	}
	return IR_FALSE;
}
BOOLEAN
no_change( from, to, var ) 
	TRIPLE *from, *to; 
	LEAF *var;
{
	/* 
	 * This routine test see if var has been changed or not
	 * between triple from(included) and to(excluded)
	 * here from and to MUST point to two triples that
	 * in the SAME basic block and from is preceding the to
	 * in the lexical order.
	 */
	register VAR_REF *rp;
	register TRIPLE *tp;

	if(ISCONST(var))
	/* it is a constant */
		return( IR_TRUE );
	TFOR(tp,from) {
		if( tp == to) break;
		for(rp = tp->var_refs; rp && rp->site.tp == tp ; rp = rp->next) {
			if( rp->reftype != VAR_AVAIL_DEF &&
					rp->reftype != VAR_DEF )
				continue;
			if( rp->leafp == var ) /* defines var */
				return( IR_FALSE );
		}

	}
	/* NO CHANGE */
	return( IR_TRUE );
}

/*
 * binary constants are usually treated as byte strings - this is
 * for when it is necessary to deal with them as aligned data
 */
long
l_align(cp1)
	register char *cp1;
{
	union {
		char c[sizeof(long)];
		long l;
	} word;
	register char *cp2 = word.c;
	register int i;

	for(i=0; i< sizeof(long); i++){
		*cp2++ = *cp1++;
	}
	return(word.l);
}

/*
 * roundup using unsigned arithmetic.
 * Beware divisions with mixed int and unsigned operands.
 * Changed in ANSI C.
 */
unsigned
roundup(u1,u2)
	unsigned u1,u2;
{
	return (u2*((u1+u2-1)/u2));
}

void
delete_implicit()
{
	/* do NOT do this if it is a mutiple entry procedure */
	/* because the leaf->entry_define and exit_use are NOT correct */

	LEAF *leafp;
	LIST *lp, **lpp;
	struct segdescr_st descr;
	
	if( ! entry_block->is_ext_entry ) {
		return;
	}
	
	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) 
	{ /* for each hash entry */
		LFOR( lp, *lpp )
		{ /* for each leaf */
			
			leafp = (LEAF *) lp->datap;
			if( ISCONST(leafp) || leafp->references == (VAR_REF *) NULL )
				continue;
				
			descr = leafp->val.addr.seg->descr;
			if( descr.class == BSS_SEG || descr.class == DATA_SEG ||
				descr.class == ARG_SEG ||
				descr.external == EXTSTG_SEG )
			{ /*delete unnecessary entry defination and exit use*/ 
				delete_entry( leafp );
			}
		}
	}
}

void
delete_entry( leafp ) LEAF *leafp;
{
	TRIPLE *tp;
	struct segdescr_st descr;

	tp = leafp->entry_define;
	if( tp == TNULL ) return;	/* partial_opt == IR_TRUE */

	if( ( tp->canreach == LNULL ) ||
			(tp->canreach == tp->canreach->next &&
			 LCAST( tp->canreach, VAR_REF)->site.tp == 
							leafp->exit_use ) ) { 
		descr = leafp->val.addr.seg->descr;
		if( descr.external == EXTSTG_SEG ) {
			tp = leafp->exit_use;
			/* at least one def can reach exit use */
			/* except when the end stmt is NOT reachable */
			if( tp && tp->reachdef1 && tp->reachdef1 == tp->reachdef1->next && 
				LCAST(tp->reachdef1, VAR_REF)->site.tp == 
							leafp->entry_define ) {
				/* delete both entry define and exit use */
				remove_triple(leafp->entry_define,entry_block);
				remove_triple(tp, exit_block); /* both are gone */
				leafp->entry_define = leafp->exit_use=TNULL;
				tp = TNULL;
			} else {
				if( leafp->entry_define->canreach == LNULL ) {
					/* delete the entry define */
					remove_triple(leafp->entry_define,entry_block);
					leafp->entry_define = TNULL;
				}
			}
		} else { /* bss or data seg or arg seg */
			remove_triple(tp,entry_block);
			tp = TNULL;
			if( leafp->exit_use != TNULL ) {
				remove_triple( leafp->exit_use, exit_block );
			}
			leafp->entry_define = leafp->exit_use = TNULL;
		}
	} else {
		tp = leafp->exit_use;
		if( tp && tp->reachdef1 && tp->reachdef1 == tp->reachdef1->next && 
				LCAST(tp->reachdef1, VAR_REF)->site.tp == 
							leafp->entry_define ) {
			/* nobody modify it, so delete the exit use */
			remove_triple(tp, exit_block); /* both are gone */
			tp = TNULL;
			leafp->exit_use=TNULL;
		}
	}
}


LIST *
copy_and_merge_lists(list1,list2,lqp)
	register LIST *list1,*list2;
	LISTQ *lqp;
{
		
	register LIST *lp1 = LNULL;
	register LIST *lp2 = LNULL;
	register LIST *select = LNULL;
	register LIST *merged = LNULL; 
	LIST *new_l;

	/* the usual klutziness about advancing to smallest*/
	if(list1) {
		lp1 = list1->next;
	} else {
		return( copy_list(list2,lqp) );
	}
	if(list2) {
		lp2 = list2->next;
	} else {
		return( copy_list(list1,lqp) );
	}
	while(lp1 || lp2) {
		if(lp1 == LNULL) {
			select = lp2;
			lp2 = ( lp2->next == list2->next ? LNULL : lp2->next);
		} else if(lp2 == LNULL) {
			select = lp1;
			lp1 = ( lp1->next == list1->next ? LNULL : lp1->next);
		} else if(lp1->datap < lp2->datap) {
			select = lp1;
			lp1 = ( lp1->next == list1->next ? LNULL : lp1->next);
		} else if(lp1->datap > lp2->datap) {
			select = lp2;
			lp2 = ( lp2->next == list2->next ? LNULL : lp2->next);
		} else {
			select = lp2;
			lp1 = ( lp1->next == list1->next ? LNULL : lp1->next);
			lp2 = ( lp2->next == list2->next ? LNULL : lp2->next);
		}
		new_l = NEWLIST(lqp);
		new_l->datap = select->datap;
		LAPPEND(merged,new_l);
	}
	return(merged);
}


/*
 * copy a string into string buffer space, and return
 * a pointer to the copy.
 */
char *
new_string(strp)
	register char *strp;
{
	int size;
	register int len;
	STRING_BUFF *tmp;
	register char *start, *cursor;

	len = strlen(strp)+1;
	if( !string_buff || ((string_buff->top + len -1) >= string_buff->max)) {
		size = ( len > STRING_BUFSIZE ? len : STRING_BUFSIZE);
		tmp = (STRING_BUFF*)ckalloc(1, (unsigned)sizeof(STRING_BUFF)+size);
		tmp->next = (STRING_BUFF*) NULL;

		if(!string_buff) {
			first_string_buff = tmp;
		} else {
			string_buff->next = tmp;
		}
		string_buff = tmp;

		string_buff->data= string_buff->top = (char*)tmp + sizeof(STRING_BUFF);
		string_buff->max = string_buff->top + size;
	}
	cursor=string_buff->top;
	start = cursor;
	string_buff->top += len;
	while( len-- > 0) *cursor++ = *strp++;
	return start ;
}
