#ifndef lint
static	char sccsid[] = "@(#)live.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include "iropt.h"
#include "recordrefs.h"
#include "var_df.h"
#include "page.h"
#include "misc.h"
#include "setup.h"
#include "tprev_seq.h"
#include "vprev_seq.h"
#include "bit_util.h"
#include "live.h"

/* local */

static	SET_PTR	db_live_after_blocks;	/* (debug) ptr to live_after_blocks[] */
static	void	compute_live_use_def(/*live_use, live_def*/);

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/* 
 * Live definitions computation.
 * 
 * Algorithm is a variation on the live variables computation in Aho,
 * Sethi, and Ullmann, "Compilers, Principles, Tools, Techniques", p. 631.
 * The information computed here is used to construct an interference
 * map during register allocation.
 * 
 * Inputs:
 * -------
 * 
 *	IR for a procedure, with variable defs and uses represented
 *	by VAR_REF records.
 *
 *	reachdef lists, for determining sets of definitions used within
 *	a block.
 * 
 *	reachdefs[], an array of sets of representing the definitions
 *	reaching the exit of each basic block.  This is used to
 *	identify the interesting live definitions for each block;
 *	see Hecht, "Flow Analysis of Compute Programs", p. 19.
 *
 *	storage for the live_after_blocks array, described below.
 * 
 * Output:
 * -------
 * 
 *	live_after_blocks[], an array of sets representing the definitions
 *	live on exit from each basic block.  For a variable x, a definition
 *	D(x) is live after a block B if there exists a path from D(x),
 *	through B, to a use of x, with no other definitions of x in the
 *	interim.
 *
 * Local storage:
 * --------------
 *
 *	Set arrays of the same dimension and cardinality as 
 *	live_after_blocks[] are used to represent the sets out[],
 *	def[], and use[] below.  These are discarded when the
 *	algorithm terminates.
 * 
 * Algorithm:
 * ----------
 * 
 *	(* out[B] <=> live_after_blocks[B] *)
 *	for each block B do
 *	    use[B] =  { definitions not in B, but used in B prior
 *			to any definition of the same variable in B }
 *	    def[B] =  { definitions in B }
 *	end for;
 *	changed = true;
 *	while changed do
 *	    changed = false;
 *	    for each block B do
 *		newout = {};
 *		for each successor S of B do
 *		    newout += use[S] + ( out[S] - def[S] );
 *		end for;
 *		newout *= reachdefs[B];
 *		if (newout != out[B]) then
 *		    out[B] = newout;
 *		    changed = true;
 *		end if;
 *	    end for;
 *	end while;
 */

void
compute_live_after_blocks(reachdefs, live_after_blocks)
    SET_PTR reachdefs;
    SET_PTR live_after_blocks;
{
    int nblocks;	/* #rows in live_after_blocks */
    int ndefs;		/* #columns in live_after_blocks */
    BLOCK *bp;		/* for iterating through blocks */
    LIST *lp, *succ;	/* for iterating through successors of blocks */
    int j, setsize;	/* for iterating through words of a set */
    BOOLEAN changed;	/* flag tested for convergence */
    SET_PTR live_use, live_def, live_newout;
    SET use, def, out, newout;
    SET reach;

    /*
     * allocate storage for use[], def[], and newout[]
     */
    nblocks = NROWS(live_after_blocks);
    ndefs = SET_CARD(live_after_blocks);
    setsize = SET_SIZE(live_after_blocks);
    live_use = new_heap_set(nblocks, ndefs);
    live_def = new_heap_set(nblocks, ndefs);
    live_newout = new_heap_set(1, ndefs);
    newout = ROW_ADDR(live_newout, 0);
    /*
     * for each block B do
     *     compute the sets use[B], def[B];
     * end for;
     */
    compute_live_use_def(live_use, live_def);
    /*
     * iterate over blocks until no out[B] changes.
     * use reverse depth-first ordering.
     */
    do {
	changed = IR_FALSE;
	for(bp = first_block_dfo; bp != NULL; bp = bp->dfoprev) {
	    /*
	     * newout = {};
	     */
	    for (j = 0; j < setsize; j++) {
		newout[j] = 0;
	    }
	    /*
	     * for each successor S of B do
	     *     newout += use[S] + (out[S] - def[S]) ;
	     * end for;
	     */
	    succ = bp->succ;
	    LFOR(lp, succ) {
		int s = LCAST(lp,BLOCK)->blockno*setsize;
		use   = &live_use->bits[s];
		out   = &live_after_blocks->bits[s];
		reach = &reachdefs->bits[s];
		def   = &live_def->bits[s];
		for(j = 0; j < setsize; j++) {
		    newout[j] |= use[j] | (out[j] & ~def[j]);
		}
	    }
	    /*
	     * if (out[B] != (newout&reach[b])) then
	     *     out[B] = newout & reach[b];
	     *     changed = true;
	     * end if;
	     */
	    reach = ROW_ADDR(reachdefs, bp->blockno);
	    out = ROW_ADDR(live_after_blocks, bp->blockno);
	    for (j = 0; j < setsize; j++) {
		if (out[j] != (newout[j]&reach[j])) {
		    break;
		}
	    }
	    if (j < setsize) {
		changed = IR_TRUE;
		for( ; j < setsize; j++) {
		    out[j] = newout[j] & reach[j];
		}
	    }
	} /* for each block B */
    } while (changed);
    if (SHOWLIVE) {
	db_live_after_blocks = live_after_blocks;
	dump_cooked(SHOWLIVE_DEBUG);
	db_live_after_blocks = NULL;
    }
    free_heap_set(live_use);
    free_heap_set(live_def);
    free_heap_set(live_newout);
} /* compute_live_after_blocks */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * display the set of live definitions after a block (B).
 * This only makes sense if the live_after_blocks[] matrix
 * has been computed.
 */
void
show_live_after_block(bp)
    BLOCK *bp;
{
    SET bv_live;
    int defno,ndefs;

    if (db_live_after_blocks != NULL) {
	/*
	 * live_after_blocks has been computed
	 */
	bv_live = ROW_ADDR(db_live_after_blocks, bp->blockno);
	ndefs = SET_CARD(db_live_after_blocks);
	printf("\n		live_defs:");
	for (defno = 0; defno < ndefs; defno++) {
	    if (test_bit(bv_live, defno)) {
		printf(" D[%d]", defno);
	    }
	}
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * Compute live definitions after a program site (B,T).
 *
 * Inputs:
 *	live_after_blocks[nblocks]: for blocks B = 0..nblocks-1,
 *	    the set of live definitions after B.
 *	site: a point in the program.
 * Output:
 *	live_after_site: the set of definitions live after the given site
 * Algorithm:
 *	B := site.bp;
 *	live_after_site := live_after_blocks[B];
 *	for each triple T of B, in reverse execution order, do
 *	    if T == site.tp then
 *		exit;
 *	    end if;
 *	    for each var_ref V of T, in reverse execution order, do
 *	        if ISUSE(V) then
 *	    	    live_after_site += reachdef(V);
 *	        else
 *	    	    live_after_site -= {V}
 *	        end if;
 *	    end for; (*var_refs*)
 *	end for; (*triples*)
 *			
 * Note[1]:
 *	Execution order does not always correspond to the order implied
 *	by the "tnext" links. For example, in
 *		T10:  x = y
 *		T11:  SCALL foo
 *			T12: PARAM z
 * 	the execution-order successor of T10 is T12, not T11.  This is
 *	significant, for example, if T12 is the last use of z.
 * Note[2]:
 *	We assume that ambiguous definitions (IMPLICIT_DEF) can be treated
 *	as if they were not, because an IMPLICIT_DEF always forces a memory
 *	reference.
 */
void
compute_live_after_site(live_after_blocks, live_after_site, site)
    SET_PTR live_after_blocks;	/* in: live definitions after each block */
    SET_PTR live_after_site;	/* out: set of live defs at a given site */
    SITE site;			/* site of a definition */
{
    TRIPLE *tp;			/* for iterating over triples */
    VAR_REF *vp;		/* for iterating over var_refs */
    SET bv_live_after_blocks;	/* bit vector of row of live_after_blocks[] */
    SET bv_live_after_site;	/* bit vector of live_after_site */
    int j, setsize;		/* for iterating over words in a set */
    LIST *lp,*reachdef;		/* for iterating over reachdef lists */
    int defno;			/* definition number, for checking */

    /*
     * initialization
     */
    bv_live_after_site = ROW_ADDR(live_after_site, 0);
    bv_live_after_blocks = ROW_ADDR(live_after_blocks, site.bp->blockno);
    setsize = SET_SIZE(live_after_blocks);
    for (j = 0; j < setsize; j++) {
	bv_live_after_site[j] = bv_live_after_blocks[j];
    }
    for (setsize = SET_SIZE(live_after_site); j < setsize; j++) {
	bv_live_after_site[j] = 0;
    }
    /*
     * for each triple T of B, in reverse execution order, do
     */
    for (tp = init_tprev_seq(site.bp); tp != TNULL; tp = tprev_seq()) {
	/* 
	 * exit when backward scan reaches site
	 */
	if (tp == site.tp) {
	    break;
	}
	/*
	 * for each var_ref V of T, in reverse execution order, do
	 */
	for (vp = init_vprev_seq(tp); vp != NULL; vp = vprev_seq()) {
	    switch (vp->reftype) {
	    case VAR_USE1:
	    case VAR_EXP_USE1:
		/*
		 * live_after_site += reachdef1(V->site.tp);
		 */
		reachdef = vp->site.tp->reachdef1;
		LFOR(lp, reachdef) {
		    defno = LCAST(lp, VAR_REF)->defno;
		    if (defno != NULLINDEX) {
			set_bit(bv_live_after_site, defno);
		    }
		}
		break;
	    case VAR_USE2:
	    case VAR_EXP_USE2:
		/*
		 * live_after_site += reachdef2(V->site.tp);
		 */
		reachdef = vp->site.tp->reachdef2;
		LFOR(lp, reachdef) {
		    defno = LCAST(lp, VAR_REF)->defno;
		    if (defno != NULLINDEX) {
			set_bit(bv_live_after_site, defno);
		    }
		}
		break;
	    case VAR_DEF:
	    case VAR_AVAIL_DEF:
		/*
		 * live_after_site -= { V }
		 */
		defno = vp->defno;
		if (defno != NULLINDEX) {
		    reset_bit(bv_live_after_site, defno);
		}
		break;
	    } /* switch */
	} /* for var_refs */
    } /* for triples */
} /* compute_live_after_site */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * display the set of live definitions after a site (B,T).
 * This only makes sense if the live_after_blocks[] matrix
 * has been computed.
 */
void
show_live_after_site(site)
    SITE site;
{
    SET_PTR live_after_site;
    SET bv_live_after_site;
    int defno;

    if (db_live_after_blocks != NULL) {
	live_after_site = new_heap_set(1, nvardefs);
	compute_live_after_site(db_live_after_blocks, live_after_site, site);
	bv_live_after_site = ROW_ADDR(live_after_site, 0);
	printf("\n		live defs:");
	for (defno = 0; defno < nvardefs; defno++) {
	    if (test_bit(bv_live_after_site, defno)) {
		printf(" D[%d]", defno);
	    }
	}
	free_heap_set(live_after_site);
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * given block B, compute the sets
 *	use[B] = { definitions not in B, but used in B prior to any
 *		   definition of the same variable in B }
 *	def[B] = { definitions in B }
 * These sets are the information on which the global live_after_blocks
 * computation is based.  Note that use[B] is derived from the result of
 * the "reaching definitions" computation.
 *
 * Algorithm:
 *	for each block B, do
 *	    for each triple T of B, in reverse execution order, do
 *		for each var_ref V of T, in reverse execution order, do
 *		    if ISUSE(V) & EXPOSED(V) then
 *			use[B] += reachdef(V);
 *		    else if AVAIL(V) then
 *			def[B] += {V};
 *			use[B] -= {available defs defining same var as V};
 *		    end if;
 *		end for; (*var_refs*)
 *	    end for; (*triples*)
 *	end for; (*blocks*)
 *
 * Remark:
 *	We assume that ambiguous definitions (IMPLICIT_DEF) can be treated
 *	as if they were not, because an IMPLICIT_DEF always forces a memory
 *	reference.
 */
static void
compute_live_use_def(live_use, live_def)
    SET_PTR live_use;	/* for block B, defs not in B but used in B */
    SET_PTR live_def;	/* defs in block B */
{
    BLOCK  *bp;			/* for iterating over blocks */
    TRIPLE *tp;			/* for iterating over triples */
    VAR_REF *vp;		/* for iterating over var_refs */
    SET bv_live_use;		/* bit vector of row of live_use */
    SET bv_live_def;		/* bit vector of row of live_def */
    unsigned defno;		/* definition number, for checking */
    unsigned maxdefno;		/* max definition number, for checking */
    LIST *lp, *reachdef;	/* for iterating over reachdef lists */

    /*
     * live_use and live_def are subsets of the set of globally
     * available definitions.  Exclude the local defs, whose def
     * numbers begin where the global ones end.
     */
    maxdefno = (unsigned)SET_CARD(live_use);
    if (maxdefno != (unsigned)SET_CARD(live_def)) {
	quit("compute_live_use_def: inconsistent set sizes");
	/*NOTREACHED*/
    }
    /*
     * for each block B, do
     */
    for (bp = entry_block; bp != NULL; bp = bp->next) {
	/*
	 * for each triple T of B, in reverse execution order, do
	 */
	bv_live_use = ROW_ADDR(live_use, bp->blockno);
	bv_live_def = ROW_ADDR(live_def, bp->blockno);
	for (tp = init_tprev_seq(bp); tp != TNULL; tp = tprev_seq()) {
	    /*
	     * for each var_ref V of T, in reverse execution order, do
	     */
	    for (vp = init_vprev_seq(tp); vp != NULL; vp = vprev_seq()) {
		switch (vp->reftype) {
		case VAR_EXP_USE1:
		    /*
		     * use[B] += reachdef1(V->site.tp);
		     */
		    reachdef = vp->site.tp->reachdef1;
		    LFOR(lp, reachdef) {
			defno = LCAST(lp, VAR_REF)->defno;
			if (defno < maxdefno) {
			    set_bit(bv_live_use, defno);
			}
		    }
		    break;
		case VAR_EXP_USE2:
		    /*
		     * use[B] += reachdef2(V->site.tp);
		     */
		    reachdef = vp->site.tp->reachdef2;
		    LFOR(lp, reachdef) {
			defno = LCAST(lp, VAR_REF)->defno;
			if (defno < maxdefno) {
			    set_bit(bv_live_use, defno);
			}
		    }
		    break;
		case VAR_AVAIL_DEF:
		    /*
		     * def[B] += {V};
		     * use[B] -= {V};
		     */
		    defno = vp->defno;
		    if (defno < maxdefno) {
			set_bit(bv_live_def, defno);
			reset_bit(bv_live_use, defno);
		    }
		    break;
		} /* switch */
	    } /* for var_refs */
	} /* for triples */
    } /* for blocks */
} /* compute_live_use_def */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * density(set_ptr) = (# of '1' bits)/(total bits)
 */

double
density(set_ptr)
    SET_PTR set_ptr;
{
    register i,nrows;		/* row index, count */
    register j,ncols;		/* column index, count */
    register ones;		/* # of 1 bits encountered */
    register SET s;		/* row of bit matrix */

    ones = 0;
    nrows = NROWS(set_ptr);
    for (i = 0; i < nrows; i++) {
	s = ROW_ADDR(set_ptr, i);
	ncols = SET_CARD(set_ptr);
	for (j = 0; j < ncols; j++) {
	    if (test_bit(s, j)) {
		ones++;
	    }
	}
    }
    return ones/((double)(nrows*ncols));
}
