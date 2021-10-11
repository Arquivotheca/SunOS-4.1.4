#ifndef lint
static	char sccsid[] = "@(#)copy_df.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "recordrefs.h"
#include "page.h"
#include "misc.h"
#include "bit_util.h"
#include "copy_ppg.h"
#include "copy_df.h"
#include <stdio.h>

/* global */
int	ncopies;
LIST	*copy_hash_tab[COPY_HASH_SIZE];

/* local */
static void copy_init();
static void find_copy(/*bp,tp*/);
static COPY * lookup_hash_copy(/* index, tp */);
static void kill_nogen(/* cp */);
static int copy_hash_func(/* tp */);

static void
copy_init()
{
	register LIST **lpp;

	for( lpp = copy_hash_tab; lpp < &copy_hash_tab[COPY_HASH_SIZE]; lpp++ )
	{
		*lpp = LNULL;
	}
	ncopies = 0;
}

void
entable_copies()
{
	register BLOCK *bp;
	register TRIPLE *tp;
	BOOLEAN interesting_copy();
	void find_copy();

	copy_init();
	for(bp=entry_block;bp;bp=bp->next) 
	{
		TFOR(tp,bp->last_triple->tnext) 
		{
			if( tp->op == IR_ASSIGN && interesting_copy( tp ) )
				find_copy( bp, tp );
		}
	}
}

BOOLEAN
interesting_copy( tp ) TRIPLE *tp;
{ /* tp->op must be an IR_ASSIGN */
  /* Right now, define interesting copy as */
  /* 1. Both left and right of the assignment are leaves */
  /* 2. In general, reference to the right must be faster*/
  /*    than reference to the left */

	register union leaf_value *rv, *lv;

	if( tp->left->operand.tag != ISLEAF )
		return( IR_FALSE );

	if( tp->right->operand.tag != ISLEAF )
		return( IR_FALSE );

	if( !same_irtype(tp->left->leaf.type, tp->right->leaf.type) )
		return( IR_FALSE );

	rv = &((LEAF *)tp->right)->val;
	lv = &((LEAF *)tp->left)->val;
	if( partial_opt ) { /* in this case, do NOTHING if involve externals */
		if( EXT_VAR((LEAF*)tp->left) ) {
			return IR_FALSE;
		}
		if( ! ISCONST( tp->right ) && EXT_VAR((LEAF*)tp->right)) {
			return IR_FALSE;
		}
	}
	
	if( ISCONST(tp->right))
	/* assign by a constant */
	{
		if( ( (LEAF *)tp->left)->no_reg || IS_SMALL_INT(tp->right))
			return( IR_TRUE );
		/* else assume the left hand side will be a register var */
		return( IR_FALSE );
	}

	if( rv->addr.seg->descr.external != EXTSTG_SEG && rv->addr.seg->descr.class != HEAP_SEG )
		return IR_TRUE;
		
	if( rv->addr.seg->descr.class == lv->addr.seg->descr.class )
	/* left and right have same class */
		return( IR_TRUE );

	return( IR_FALSE );
}

static void
find_copy(bp,tp) BLOCK *bp; TRIPLE *tp;
{ /* tp must have an IR_ASSIGN op and both operand have to be LEAF */

	COPY *cp, *lookup_hash_copy();
	COPY_REF *copy_ref;
	int hash, copy_hash_func();
	LIST *lp;

	if( tp == TNULL )
		quit("add_copy: bad triple" );

	hash = copy_hash_func( tp );

	cp = lookup_hash_copy( hash, tp );

	tp->expr = (EXPR *)cp;
	lp = NEWLIST(copy_lq);
	copy_ref = (COPY_REF *)ckalloc(1, sizeof(COPY_REF));
	copy_ref->site.bp = bp;
	copy_ref->site.tp = tp;
	lp->datap = (LDATA *)copy_ref;
	LAPPEND(cp->define_at, lp);
}

static COPY *
lookup_hash_copy( index, tp ) register int index; TRIPLE *tp;
{
	register LIST *lp;
	register COPY *cp;
	void kill_nogen();

	LFOR( lp, copy_hash_tab[index] )
	{
		cp = (COPY *)lp->datap;
		if( cp->left == (LEAF *)tp->left && 
				cp->right == (LEAF *)tp->right )
			return( cp );
	}
	/* not found, it must be a new COPY */
	cp = (COPY *)ckalloc( 1, sizeof(COPY) );
	cp->copyno = ncopies++;
	cp->visited = IR_FALSE;
	cp->left = (LEAF *)tp->left;
	cp->right = (LEAF *)tp->right;
	cp->define_at = LNULL;
	kill_nogen( cp );
	lp = NEWLIST(copy_lq);
	lp->datap = (LDATA *)cp;
	LAPPEND(copy_hash_tab[index], lp);
	return( cp );
}

static void
kill_nogen( cp ) COPY *cp;
{ /* set up the kill field of a LEAF as if the LEAF has be re-defined */

	register LEAF *leafp;
	register LIST *lp;

	/* add cp to both operands kill_copy list */	
	leafp = cp->left;
	lp = NEWLIST(copy_lq);
	lp->datap = (LDATA *)cp;
	LAPPEND(leafp->kill_copy, lp );

	leafp = cp->right;
	lp = NEWLIST(copy_lq);
	lp->datap = (LDATA *)cp;
	LAPPEND(leafp->kill_copy, lp );

}

void
build_copy_kill_gen()
{ /* build the kill and gen for each basic block */

	LIST *lp2;
	VAR_REF *var_ref;
	LEAF *leafp;
	BLOCK *bp;
	TRIPLE *tp;
	COPY *cp;

	for( var_ref = var_ref_head; var_ref ; var_ref = var_ref->next ) 
	{ /* for each variable reference */
		if( var_ref->reftype != VAR_DEF &&
				var_ref->reftype != VAR_AVAIL_DEF )
			continue; /* only inerested in VAR_DEF */

		bp = var_ref->site.bp;
		leafp = var_ref->leafp;
		LFOR( lp2, leafp->kill_copy )
		{ /* kill all the copies */
			cp = (COPY *)lp2->datap;
			bit_set( copy_kill, bp->blockno, cp->copyno );
			bit_reset( copy_gen, bp->blockno, cp->copyno );
		}
		tp = var_ref->site.tp;
		if( tp->op == IR_ASSIGN && tp->expr )
		{ /* gen an interested copy */
			cp = (COPY *)tp->expr;
			bit_set( copy_gen, bp->blockno, cp->copyno );
			bit_reset( copy_kill, bp->blockno, cp->copyno );
		}
	}
	for( leafp = leaf_tab; leafp; leafp = leafp->next_leaf ) {
		leafp->kill_copy = NULL;
	}
}

void
build_copy_in_out()
{ /* compute IN and OUT for forward/union flow problem */
	register BIT_INDEX in_index,out_index,kill_index,gen_index;
	register LIST *lp;
	register BLOCK *bp;	/* steps through blocks */
	BOOLEAN	change;		/* set if any changes propagated */
	register int i;		/* loop control */
	SET_PTR copy_newin;	/* temp set for detecting changes */
	int b;			/* start of a block's bits within a set */
	BIT_INDEX newin, in, out, gen, kill;

	/* get space for newin */
	copy_newin = new_heap_set(1,ncopies);
	newin = copy_newin->bits;
	in = copy_in->bits;
	out = copy_out->bits;
	gen = copy_gen->bits;
	kill = copy_kill->bits;

	/* initialize OUT[b] to GEN[b] */
	
	for( i=0; i<copy_set_wordsize; i++ )
	{
		in[i] = 0;
		out[i] = gen[i];
	}

	for( i=copy_set_wordsize; i<(nblocks*copy_set_wordsize); i++ )
	{
		in[i] = ~0;
		out[i] = ~kill[i];
	}

	change = IR_TRUE;
	while (change == IR_TRUE) 
	{
		change = IR_FALSE;
		for (bp=entry_block->next;bp;bp=bp->next) 
		{
			b=bp->blockno*copy_set_wordsize;
			in_index = &in[b];
			kill_index = &kill[b];
			gen_index = &gen[b];
			for (i=0; i<copy_set_wordsize; i++) 
			{
				newin[i] = ~0;
			}

			/* for each of block b's predecessors */
			LFOR(lp,bp->pred) 
			{ /* newin gets OUT of the predecessor */
				out_index = &out[LCAST(lp,BLOCK)->blockno*copy_set_wordsize];
				for (i=0; i<copy_set_wordsize; i++) 
				{
					newin[i] &= out_index[i];
				}
			}

			/* set change according to whether IN[b] has changed */
			if(change == IR_FALSE) {
				for (i=0; i<copy_set_wordsize; i++) {
					if (in_index[i] != newin[i]) change = IR_TRUE;
				}
			}

			out_index = &out[b];
			/* set IN[b] to NEWIN[b] */
			/* and OUT[b]=IN[b] - KILL[b] U GEN[b] */
			for (i=0; i<copy_set_wordsize; i++) {
				in_index[i] = newin[i];
				out_index[i] =(in_index[i] & ~kill_index[i]) | gen_index[i];
			}
		}
	}
	free_heap_set(copy_newin);
}

static int
copy_hash_func( tp ) TRIPLE *tp;
{
	return( tp->tripleno % COPY_HASH_SIZE );
}
