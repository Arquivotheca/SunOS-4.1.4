#ifndef lint
static	char sccsid[] = "@(#)auto_alloc.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "page.h"
#include "misc.h"
#include "auto_alloc.h"
#include <stdio.h>

/*
 * Storage allocation for auto variables.  This is done after register
 * allocation, to minimize the size of activation records.  This version
 * does not attempt to overlap variables declared in nested scopes, since
 * the required information is not available from the front ends.
 *
 * Algorithm:
 *	for each segment S do
 *	    S.visited = FALSE;
 *	end for;
 *	for each triple T do
 *	    for each operand L used by T do
 *	    	if L is a VAR or ADDR_CONST leaf L in an AUTO_SEG then
 *		    L->seg->visited = TRUE;
 *		end if;
 *	    end if;
 *	end for;
 *	for each segment S do
 *	    if S.visited then
 *		allocate storage to S;
 *	    end if;
 *	end for;
 *	for each leaf L do
 *	    if L is a VAR or ADDR_CONST leaf L in an AUTO_SEG
 *	    and if not L->seg->visited then
 *		delete L;
 *	    end if;
 *	end for;
 *	for each segment S do
 *	    if S is an AUTO_SEG segment and if not S.visited then
 *		delete S;
 *	    end if;
 *	end for;
 */

#define BACKAUTO	/* stack grows backwards on all Sun machines to date */

/* global */
int	nleaves_deleted=0;
int	nsegs_deleted=0;

/* local */
static BOOLEAN is_auto(/*np*/);
static void mark_auto_seg(/*np*/);
static long new_offset(/*nbytes, align*/);
static void auto_alloc(/*segp*/);
static void init_auto_alloc();
static void delete_list_elements(/*listhdr, can_delete*/);
static BOOLEAN is_unused_auto_leaf(/*lp*/);
static void delete_unused_autos();

static long autooff;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * reallocate storage for all AUTO_SEG leaves used
 * or defined as operands of triples
 */
void
do_auto_alloc()
{
    TRIPLE *tp, *first;
    BLOCK *bp;

    init_auto_alloc();
    for (bp = entry_block; bp != NULL; bp = bp->next) {
	if (bp->last_triple) {
	    first = bp->last_triple->tnext;
	    TFOR(tp, first) {
		visit_operands(tp, mark_auto_seg);
	    }
	}
    }
    auto_alloc();
    delete_unused_autos();
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * apply a function to operands of a triple
 */
void
visit_operands(tp, fn)
    TRIPLE *tp;
    void (*fn)();
{
    TRIPLE *tp2;
    IR_OP op;

    op = tp->op;
    if (ISOP(op, USE1_OP) || ISOP(op, MOD_OP) || op == IR_ADDROF) {
	(*fn)(tp->left);
    }
    if (ISOP(op, USE2_OP)) {
	(*fn)(tp->right);
    }
    if (ISOP(op, NTUPLE_OP)) {
	TFOR(tp2, (TRIPLE*) tp->right ) {
	    visit_operands(tp2, fn);
	}
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * is_auto(np) returns 1 if np is a VAR_LEAF or ADDR_CONST_LEAF
 * in an AUTO_SEG segment.
 */
static BOOLEAN
is_auto(np)
    IR_NODE *np;
{
    if (np->operand.tag == ISLEAF
      && (np->leaf.class == VAR_LEAF || np->leaf.class == ADDR_CONST_LEAF)
      && np->leaf.val.addr.seg->descr.class == AUTO_SEG) {
	return IR_TRUE;
    }
    return IR_FALSE;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * If a node is an automatic VAR or ADDR_CONST,
 * mark the underlying AUTO_SEG segment "visited".
 */
static void
mark_auto_seg(np)
    IR_NODE *np;
{
    if (is_auto(np)) {
	np->leaf.val.addr.seg->visited = IR_TRUE;
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * Allocate a new offset for an object of length nbytes
 * in an automatic storage.
 */
static long
new_offset(nbytes, align)
    long nbytes;
    int align;
{
    long newoff;

    if (align == 0) {
	quit("new_offset: zero alignment");
	/*NOTREACHED*/
    }
    if (nbytes >= SZINT/SZCHAR && align <= SZINT/SZCHAR) {
	align = SZINT/SZCHAR;
    }
#   ifdef BACKAUTO
	/* backward-growing stack */
	newoff = -autooff + nbytes;
	newoff = roundup((unsigned)newoff, (unsigned)align);
	autooff = -newoff;
	return autooff;
#   else !BACKAUTO
	/* forward-growing stack */
	autooff = roundup((unsigned)autooff, (unsigned)align);
	newoff = autooff;
	autooff += nbytes;
	return newoff;
#   endif
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * the point of all this.
 */
static void
auto_alloc()
{
    SEGMENT *segp;
    long oldoff, newoff;

    for (segp = seg_tab; segp != NULL; segp = segp->next_seg) {
	if (segp->visited && segp != &seg_tab[AUTO_SEGNO]) {
	    oldoff = segp->offset;
	    newoff = new_offset(segp->len, segp->align);
	    segp->offset += (newoff - oldoff);
	}
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
init_auto_alloc()
{
    SEGMENT *segp;

    nleaves_deleted = 0;
    nsegs_deleted = 0;
    for (segp = seg_tab; segp != NULL; segp = segp->next_seg) {
	segp->visited = IR_FALSE;
    }
    if (seg_tab[AUTO_SEGNO].len > 0) {
	seg_tab[AUTO_SEGNO].visited = IR_TRUE;	/* pc0 KLUDGE*/
    }
#   ifdef BACKAUTO
	/* backward-growing stack */
	autooff = seg_tab[AUTO_SEGNO].offset;
#   else /* !BACKAUTO */
	/* forward-growing stack */
	autooff = seg_tab[AUTO_SEGNO].offset + seg_tab[AUTO_SEGNO].len;
#   endif /* !BACKAUTO */
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * Delete elements from a list.
 * listhdr is a pointer to a pointer to the list.
 * can_delete() is a predicate that returns TRUE for
 * a list element that can be deleted.
 */
static void
delete_list_elements(listhdr, can_delete)
    LIST **listhdr;
    BOOLEAN (*can_delete)(/*lp*/);
{
    LIST *lp, *lnext;

    LFOR(lp, *listhdr) {
	lnext = lp->next;
	while ((*can_delete)(lnext)) {
	    delete_list(listhdr, lnext);
	    if (*listhdr == LNULL) {
		/* thinly disguised GOTO */
		return;
	    }
	    lnext = lp->next;
	}
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static BOOLEAN
is_unused_auto_leaf(lp)
    LIST *lp;
{
    LEAF *leafp;

    leafp = LCAST(lp, LEAF);
    if (is_auto((IR_NODE*)leafp) && !leafp->val.addr.seg->visited)
	return IR_TRUE;
    return IR_FALSE;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
delete_unused_autos()
{
    LEAF *leafp, **leafpp;
    SEGMENT *segp, **segpp;

    /*
     * Delete references to otherwise unused autos from the
     * "heap" list (leaf_tab->overlap).
     */
    delete_list_elements(&leaf_tab->overlap, is_unused_auto_leaf);
    leafpp = &leaf_tab;
    leafp = *leafpp;
    while (leafp != NULL) {
	/*
	 * invariants: leafpp points to a link to the next
	 * leaf in the chain.  leafp == *leafpp.
	 */
	if (is_auto((IR_NODE*)leafp) && !leafp->val.addr.seg->visited) {
	    /*
	     * unused leaf node in an auto segment; delete
	     */
	    leafp = leafp->next_leaf;
	    (void)bzero((char*)*leafpp, sizeof(LEAF));
	    *leafpp = leafp;
	    nleaves_deleted++;
	} else {
	    /*
	     * advance to next leaf
	     */
	    leafpp = &leafp->next_leaf;
	    leafp = *leafpp;
	}
    }
    segpp = &seg_tab;
    segp = *segpp;
    while (segp != NULL) {
	/*
	 * invariants: segpp points to a link to the next
	 * seg in the chain.  segp == *segpp.
	 */
	if (segp->descr.builtin == USER_SEG
	  && segp->descr.class == AUTO_SEG && !segp->visited) {
	    /*
	     * unused auto segment; delete
	     */
	    segp = segp->next_seg;
	    (void)bzero((char*)*segpp, sizeof(SEGMENT));
	    *segpp = segp;
	    nsegs_deleted++;
	} else {
	    /*
	     * advance to next segment
	     */
	    segpp = &segp->next_seg;
	    segp = *segpp;
	}
    }
}
