#ifndef lint
static	char sccsid[] = "@(#)tprev_seq.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This module defines an "iterator" abstraction for routines
 * that must traverse triples of a basic block in reverse
 * execution order.
 *
 * The fact that this module is necessary in the first place
 * suggests the presence of dry rot somewhere in the foundation.
 */

#include <stdio.h>
#include "ir_common.h"
#include "tprev_seq.h"

/* local */

typedef enum { Inmain, Inparams } States;

static	States state;
static	TRIPLE *tstop, *tsave_stop;
static	TRIPLE *tcursor, *tsave_cursor;  /* triple currently being processed*/

/*
 * init_tprev_seq(bp) -- initialize a backward sequence
 */
TRIPLE *
init_tprev_seq(bp)
    BLOCK *bp;
{
    state = Inmain;
    tcursor = bp->last_triple;		/* start at the end */
    tstop = tcursor->tnext;		/* and work back to the beginning */
    return tprev_seq();
}

/*
 * prev_triple() -- return the previous triple, i.e.,
 * the next one in reverse execution order.  Returns
 * TNULL when the first triple of the block has already
 * been processed.
 */
TRIPLE *
tprev_seq()
{
    TRIPLE *tp;

    tp = tcursor;
    if (tp == TNULL) {
	/*
	 * already processed all triples
	 */
    } else if ((tp->op == IR_SCALL || tp->op == IR_FCALL)
      && tp->right != NULL) {
	/*
	 * We have a parameter list to deal with.
	 * We are guaranteed that parameter lists do
	 * not nest, so only one level of state need
	 * be saved.
	 */
	state = Inparams;
	tsave_cursor = tcursor->tprev;
	tsave_stop = tstop;
	tcursor = (TRIPLE*)tcursor->right;
	tstop = tcursor->tnext;
    } else if (tp == tstop) {
	/*
	 * we are about to return the last triple
	 * in the sequence.  Next time around, either we
	 * resume the main-line code, or we start emulating
	 * /dev/null.
	 */
	if (state == Inparams) {
	    state = Inmain;
	    tcursor = tsave_cursor;
	    tstop = tsave_stop;
	} else {
	    tcursor = TNULL;
	}
    } else {
	tcursor = tcursor->tprev;
    }
    return tp;
}
