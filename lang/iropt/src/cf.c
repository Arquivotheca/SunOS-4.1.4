#ifndef lint
static	char sccsid[] = "@(#)cf.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#include "iropt.h"
#include "page.h"
#include "misc.h"
#include "read_ir.h"
#include "cf.h"
#include <stdio.h>
#include <ctype.h>

/* locals */
static LIST	*labeldef_list;	/* to chain label defs in lexical order */
static void	partition_entry_triples(/*entry*/);
static void	move_value_triples();
static void	check_children(/*parent, insert_after*/);
static TRIPLE	*move_comments(/*comment,last_triple*/);
static void	make_succ_list(/*block,next_triple*/);
static void	make_pred_list();
static BOOLEAN	singleton(/*lp*/);
static BOOLEAN	trivial_block(/*bp*/);
static void	remove_trivial_blocks();
static void	remove_unreach();

/* the usual stuff for hashing */
#define LABEL_HASH_SIZE 256
static	LIST	*label_hash_tab[LABEL_HASH_SIZE];
#define label_hash(labelno) (((unsigned)(labelno)) % LABEL_HASH_SIZE)

/*
 * Add a label reference or definition to the label hash table.
 * Add label defs to the labeldef_list in order of their occurrence.
 * Basic blocks will be created in this order.
 */
void
add_labelref(np)
    IR_NODE *np;
{
    TRIPLE *tp;		/* triple pointer, if np is triple */
    LEAF *leafp;	/* leaf pointer, if np is LABELNO const */
    int labelno;	/* label number from triple or leaf */
    LABELREC *label;	/* for new label record */
    LIST *lp;		/* for probing hash chains */
    int h;		/* hash table index */

    /*
     * extract label number from the input operand
     */
    switch (np->operand.tag) {
    case ISTRIPLE:
	tp = (TRIPLE *) np;
	labelno = ((LEAF *) tp->left)->val.const.c.i;
	break;
    case ISLEAF:
	leafp = (LEAF *) np;
	labelno = leafp->val.const.c.i;
	break;
    default:
	quita("add_labelref: illegal operand tag (0x%x)\n",
	      np->operand.tag);
	/* NOTREACHED */
    }
    /*
     * look up the label in the label hash table ...
     */
    h = label_hash(labelno);
    label = NULL;
    LFOR(lp, label_hash_tab[h]) {
	if (LCAST(lp, LABELREC)->labelno == labelno) {
	    label = LCAST(lp, LABELREC);
	    break;
	}
    }
    /*
     * ...inserting a new one if none is found.
     */
    if (label == NULL) {
	/* make new label */
	label = new_labelrec(IR_FALSE);
	label->labelno = labelno;
	lp = NEWLIST(tmp_lq);
	lp->datap = (LDATA *) label;
	LAPPEND(label_hash_tab[h], lp);
    }
    /*
     * LABELNO constants and LABELREFs go on the refs list
     * of their label record.  LABELDEFs (1 per label) go in
     * the defs field of the label record, and the record itself
     * goes on the labeldef_list, which is used later to construct
     * basic blocks in lexical order.
     */
    if (np->operand.tag == ISLEAF) {
	lp = NEWLIST(tmp_lq);
	lp->datap = (LDATA *) leafp;
	LAPPEND(label->refs, lp);
	return;
    }
    switch (tp->op) {
    case IR_LABELDEF:
	if (label->def != NULL) {
	    quita("duplicate definition of label L%d\n",
		  labelno);
	    /* NOTREACHED */
	}
	label->def = tp;
	lp = NEWLIST(tmp_lq);
	lp->datap = (LDATA *) label;
	LAPPEND(labeldef_list, lp);
	break;
    case IR_LABELREF:
	lp = NEWLIST(tmp_lq);
	lp->datap = (LDATA *) tp;
	LAPPEND(label->refs, lp);
	break;
    default:
	quita("add_labelref: expected LABELDEF or LABELREF, got %d\n",
	      (int) tp->op);
	/* NOTREACHED */
    }
}

/*
 * Convert label references from label numbers to block
 * pointers, using the records in the label hash table.
 * Most new blocks are created here.
 * 
 * Algorithm:
 *	for each block B,
 *	    label = lookup(labelno(B));
 *	    if null(label) then
 *		error("block with no label");
 *	    end if;
 *	    block(label) = B;
 *	end for;
 *	for each referenced label definition L,
 *	    if null(block(L)) then
 *		block(L) = new block;
 *	    end if;
 *	    labelno(block(L)) = labelno(L) = new label;
 *	    (* convert label refs/defs to block pointers *)
 *	    B = block(L);
 *	    left(def(L)) = B;
 *	    for each label reference LR in refs(L),
 *		if (LR is triple) then
 *		    left(LR) = B;
 *		else (* LR is constant of type LABELNO *)
 *		    value(LR) = labelno(L);
 *		end if;
 *	    end for;
 *	end for;
 *	for each label L,
 *	    if null(def(L)) then
 *		error("undefined label");
 *	    end if;
 *	end for;
 */
void
connect_labelrefs()
{
    int labelno;	/* label numbers */
    unsigned h;		/* for label hashing */
    LABELREC *label;	/* label records */
    TRIPLE *def_tp;	/* for setting fields in LABELDEF triples */
    BLOCK *bp;		/* for traversing blocks */
    LIST *lp;		/* for traversing hash chains, label defs */
    LIST *lrp;		/* for traversing label references */
    LIST *labeldefs;	/* for traversing label definitions */

    /*
     * for each block B,
     */
    for (bp = entry_block; bp != NULL; bp = bp->next) {
	/*
	 * label = lookup(labelno(B));
	 */
	labelno = bp->labelno;
	h = label_hash(labelno);
	label = NULL;
	LFOR(lp, label_hash_tab[h]) {
	    if (LCAST(lp, LABELREC)->labelno == labelno) {
		label = LCAST(lp, LABELREC);
		break;
	    }
	}
	if (label == NULL) {
	    quita("block B[%d] has no def for label L%d\n",
		  bp->blockno, bp->labelno);
	    /* NOTREACHED */
	}
	/*
	 * block(label) = B;
	 */
	label->block = bp;
    }
    /*
     * for each referenced label definition L,
     */
    labeldefs = labeldef_list->next;
    LFOR(lp, labeldefs) {
	label = LCAST(lp, LABELREC);
	if (label->refs == NULL)
	    continue;
	/*
	 * if null(block(L)) then
	 *     block(L) = new block;
	 * end;
	 */
	if (label->block == NULL) {
	    label->block = new_block();
	}
	/*
	 * labelno(block(L)) = labelno(L) = new label;
	 */
	bp = label->block;
	labelno = new_label();
	label->labelno = labelno;
	bp->labelno = labelno;
	/*
	 * convert def(L) and refs(L) to use block pointers
	 * instead of label numbers
	 */
	def_tp = label->def;
	def_tp->left = (IR_NODE *)bp;
	def_tp->visited = IR_TRUE;
	LFOR(lrp, label->refs) {
	    if (LCAST(lrp, IR_NODE)->operand.tag == ISTRIPLE) {
		/* IR_LABEL{REF,DEF} triple */
		LCAST(lrp, TRIPLE)->left = (IR_NODE *)bp;
	    } else {
		/* LABELNO constant leaf */
		LCAST(lrp, LEAF)->val.const.c.i = labelno;
	    }
	}
    }
    /*
     * for each label L,
     */
    for (h = 0; h < LABEL_HASH_SIZE; h++) {
	LFOR(lp, label_hash_tab[h]) {
	    /*
	     * if null(def(L)) then
	     */
	    label = LCAST(lp, LABELREC);
	    if (label->def == NULL) {
		if (label->labelno == -1) {
		    /*
		     * block is exit block; convert all legitimate
		     * references to null block pointers.
		     */
		    LFOR(lrp, label->refs) {
			if (LCAST(lrp, IR_NODE)->operand.tag == ISTRIPLE) {
			    /* IR_LABEL{REF,DEF} triple */
			    LCAST(lrp, TRIPLE)->left = (IR_NODE *) NULL;
			} else {
			    quita("connect_labelrefs: bad label reference\n");
			    /*NOTREACHED*/
			}
		    }
		} else {
		    /*
		     * regular label, with no definition
		     */
		    quita("connect_labelrefs: undefined label L%d\n",
			  label->labelno);
		    /* NOTREACHED */
		}
	    }
	}
    }
    /*
     * cleanup
     */
    empty_listq(tmp_lq);
    labeldef_list = LNULL;
    (void) new_labelrec(IR_TRUE);
}

void
form_bb() 
{
	register BLOCK *bp;

	if(ext_entries->next != ext_entries) { /* then scan the "dummy" entry_block*/
		if(entry_block->last_triple) {
			partition_entry_triples(entry_block);
		}
	}
	for(bp=entry_tab; bp < entry_top ; bp ++ ) {
		if(bp->last_triple) {
			partition_entry_triples(bp);
		} 
	}
	cleanup_cf();
	/*renumber blocks to ensure bp->blockno in order and unique */
	nblocks =0;
	for(bp=entry_block; bp; bp=bp->next) {
		bp->blockno = nblocks++;
	}
}

/*
 * delete trivial blocks and unreachable code to avoid
 * gumming up the works later on
 */
void
cleanup_cf()
{
	make_pred_list();
	if(optimizing()==IR_TRUE) {
		remove_trivial_blocks();
		remove_unreach();
		move_value_triples();
	}
}

/*	
	pass through all triples and append them to their leaders
	use the references for the last triple in a block to build the succ list
*/

/* 
** returns the next triple in the list and sets branch flags
*/
# 	define NEXT_TRIPLE(tp) \
	( 	(lastwasbranch = isbranch),\
		((tp) = ((tp) == list_last ? TNULL : (tp)->tnext)),\
		(isbranch = (tp == TNULL ? 0 : ISOP(tp->op,BRANCH_OP)))\
	)

#	define SAME_BLOCK(tp) (tp && tp->visited != IR_TRUE && lastwasbranch == 0)

#	define	PATCH_TRIPLES(bp,first_triple,next_block) \
		first_triple->tprev=next_block->tprev;\
		next_block->tprev->tnext = first_triple;\
		bp->last_triple=next_block->tprev;\
		next_block->tprev = list_last; \
		list_last->tnext = next_block; \
		make_succ_list(bp,next_block);\
		prev_block = bp;
	
static void
partition_entry_triples(entry)
	BLOCK *entry;
{
	register TRIPLE *first;
	register TRIPLE *tp;
	TRIPLE  *list_last, *move_comments();
	register BLOCK *bp;
	int isbranch = 0, lastwasbranch = 0;
	BLOCK *prev_block;

	if(entry->is_ext_entry == IR_TRUE) {
		/* change its labelno since not done in connect_labelrefs */
		entry->labelno = new_label();
		/* and mark its first triple a leader */
		entry->last_triple->tnext->visited = IR_TRUE; 
	}
	list_last = entry->last_triple;
	tp = first = entry->last_triple->tnext;
	if(tp->op != IR_LABELDEF)  {
		quit("partition_entry_triples: first triple is not a label");
	}
	/*
	 * pass over the triples that belong to the entry block
	 */
	do {
		NEXT_TRIPLE(tp);
	} while(SAME_BLOCK(tp));
	if(tp) {
		/*
		 * some triples remain : assign them to the block
		 * identified by the "leader" label and fix the entry
		 * block's triple list
		 */
		PATCH_TRIPLES(entry,first,tp);
		do {
			if(tp->op == IR_PASS) {
				tp = move_comments(tp,&list_last);
			}
			first = tp;
			if(tp->visited != IR_TRUE) {
				/*
				 * put unreachable code in separate blocks
				 * identified by a -1 labelno - insert them
				 * so as to maintain code order
				 */
				bp = new_block();
				bp->labelno = -1;
				move_block(prev_block,bp);
			} else {
				bp = (BLOCK*) tp->left;
			}
			do {
				NEXT_TRIPLE(tp);
			} while(SAME_BLOCK(tp));
			if(tp) {
				PATCH_TRIPLES(bp,first,tp);
			} else {
				first->tprev=list_last;
				list_last->tnext = first;
				bp->last_triple = list_last;
				make_succ_list(bp,(TRIPLE*) NULL);
			}
			if(bp->last_triple->tnext->op != IR_LABELDEF) {
				TYPE type;
				type.tword = UNDEF;
				type.size = 0;
				type.align = 0;
				(void)append_triple(bp->last_triple,
					IR_LABELDEF,
					(IR_NODE*)bp,(IR_NODE*)ileaf(0),type);
			} else {
				bp->last_triple->tnext->left = (IR_NODE*) bp;
			}
		} while (tp);
	} else {
		make_succ_list(entry, (TRIPLE*) NULL);
	}
	if(entry->last_triple->tnext->op != IR_LABELDEF) {
		TYPE type;
		type.tword = UNDEF;
		type.size = 0;
		type.align = 0;
		(void)append_triple(entry->last_triple,
			IR_LABELDEF, (IR_NODE*)entry, (IR_NODE*)ileaf(0), type);
	} else {
		entry->last_triple->tnext->left = (IR_NODE*) entry;
	}
}

static void
move_value_triples()
{
	register BLOCK *bp;
	register TRIPLE *tp, *first, *tlp;
	register int tripleno;

	for(bp=entry_block; bp; bp=bp->next) {
		if(bp->last_triple) {
			first=bp->last_triple->tnext;
		} else {
			first = TNULL;
		}
		TFOR(tp,first){
			if( ISOP(tp->op,ROOT_OP)
			    && (ISOP(tp->op,USE1_OP) || ISOP(tp->op,USE2_OP))){
				check_children(tp,tp->tprev);
			}
		}
	}
	tripleno=0;
	for(bp=entry_block; bp; bp=bp->next) {
		if(bp->last_triple) {
			first=bp->last_triple->tnext;
		} else {
			first = TNULL;
		}
		TFOR(tp,first){
			tp->tripleno = tripleno++;
			if( tp->op == IR_SCALL || tp->op == IR_FCALL ) {
				/* PARMS */
				TFOR(tlp, (TRIPLE *)tp->right ) {
					tlp->tripleno = tripleno++;
				}
			}
		}
	}
}

static void
check_children(parent, insert_after)
	register TRIPLE *parent, *insert_after;
{
	register TRIPLE *child;

	/*
	 * We pay no attention to last_triple in any of this on the grounds
	 * that only value_ops are moved and a value_op cannot be the
	 * last_triple of either the from or to blocks.
	 */
	if (ISOP(parent->op,USE1_OP) && parent->left->operand.tag == ISTRIPLE) {
		child = (TRIPLE*) parent->left;
		if(child != insert_after) {
			child->tprev->tnext = child->tnext;
			child->tnext->tprev = child->tprev;
			child->tprev = child->tnext = child;
			TAPPEND(insert_after,child);
		}
		check_children(child,child->tprev);
	}
	if (ISOP(parent->op,USE2_OP) && parent->right->operand.tag == ISTRIPLE){
		child = (TRIPLE*) parent->right;
		if(child != insert_after) {
			child->tprev->tnext = child->tnext;
			child->tnext->tprev = child->tprev;
			child->tprev = child->tnext = child;
			TAPPEND(insert_after,child);
		}
		check_children(child,child->tprev);
	}
	if(ISOP(parent->op,NTUPLE_OP)) {
		register TRIPLE *params;
		params = (TRIPLE*) parent->right;
		TFOR(child, params) {
			check_children(child,parent->tprev);
		}
	}
}

static TRIPLE*
move_comments(comment,last_triple)
	TRIPLE *comment, **last_triple;
{
	TRIPLE *code;

	if (comment == *last_triple) {
		return comment;
	}
	code = comment;
	while(code != *last_triple && code->op == IR_PASS) {
		code = code->tnext;
	}
	if (code->op == IR_PASS) {
		return comment;
	}
	if (code == *last_triple) {
		*last_triple = code->tprev;
	}
	code->tprev->tnext = code->tnext;
	code->tnext->tprev = code->tprev;

	code->tnext = comment;
	code->tprev = comment->tprev;
	comment->tprev->tnext = code;
	comment->tprev = code;
	return code;
}


static void
make_succ_list(block,next_triple)
	BLOCK* block;
	TRIPLE *next_triple;
{
	register TRIPLE *label, *last;
	TYPE type;
	BLOCK *next_block;
	TRIPLE *ref_triple;

	last = (TRIPLE*) block->last_triple;
	if(last == (TRIPLE*) NULL) {
		block->succ = LNULL;
	} else if(ISOP(last->op,BRANCH_OP)) {
		switch(last->op) {
		case IR_SWITCH :
		case IR_CBRANCH :
		case IR_INDIRGOTO :
		case IR_REPEAT:
			TFOR(label,(TRIPLE*)last->right) {
				(void)insert_list(&block->succ,
					(LDATA*)label->left, proc_lq);
			}
			break;
		case IR_GOTO :
			label = (TRIPLE*) last->right;
			if((BLOCK*)label->left != (BLOCK*) NULL) {
				block->succ = NEWLIST(proc_lq);
				block->succ->datap = (LDATA*)label->left;
			} else {
				if(exit_block == (BLOCK*) NULL) {
					exit_block = block;
				} else {
					quit("make_succ_list: multiple exit blocks");
				}
			}
			break;

		}
	} else if(next_triple == (TRIPLE*) NULL) {
			block->succ = LNULL;
	} else {
		if(next_triple->op != IR_LABELDEF || next_triple->visited != IR_TRUE ) {
			quit("make_succ_list: fall through into an unlabeled block");
		}
		next_block = (BLOCK*) next_triple->left;

		type.tword = UNDEF;
		type.size = 0;
		type.align = 0;

		ref_triple = append_triple(TNULL,
			IR_LABELREF, (IR_NODE*) next_block,
			(IR_NODE*)ileaf(0), type);
 		block->last_triple = append_triple(block->last_triple,
			IR_GOTO, (IR_NODE*)NULL, (IR_NODE*)ref_triple, type);
		block->succ = NEWLIST(proc_lq);
		(BLOCK*) block->succ->datap = next_block;
	}
}

static void
make_pred_list()
{
	register BLOCK *a, *b;
	register LIST *lp1, *lp2;

	/* 
	**	build the predecessor lists from the succesor lists:
	**	all b that are successors of a have a as a predecessor
	*/

	for(a=entry_block;a;a=a->next) {
		LFOR(lp1,a->succ) {
			b = (BLOCK*) lp1->datap;
			lp2=NEWLIST(proc_lq);
			(BLOCK*) lp2->datap = a;
			LAPPEND(b->pred,lp2);
		}
	}
}

/*
 * returns TRUE for a list of exactly 1 item
 */
static BOOLEAN
singleton(lp)
    LIST *lp;
{
    return (BOOLEAN)(lp != LNULL && lp->next == lp);
}

/*
 * returns TRUE for a block containing nothing but
 * LABELDEF, GOTO, and STMT triples.
 */
static BOOLEAN
trivial_block(bp)
    BLOCK *bp;
{
    TRIPLE *tp, *tlast;
    LIST *lp;

    tlast = bp->last_triple;
    TFOR(tp, tlast) {
	switch(tp->op) {
	case IR_LABELDEF:
	case IR_GOTO:
	case IR_STMT:
	    continue;
	default:
	    return IR_FALSE;
	}
    }
    LFOR(lp, indirgoto_targets) {
	if ((BLOCK*)LCAST(lp,TRIPLE)->left == bp) {
	    /*
	     * can't remove this block,
	     * because we can't find the references
	     */
	    return IR_FALSE;
	}
    }
    return IR_TRUE;
}

/*
 * look for blocks that have only one successor, and
 * no content.  This isn't done so much to improve the final
 * code as to reduce the dimensionality of data flow computations.
 */
static void
remove_trivial_blocks()
{
    BLOCK *bp;			/* for traversing each block in program */
    BLOCK *b_pred, *b_succ;	/* predecessors and successor of bp */
    LIST  *lp, *lp2;		/* scratch list pointers */
    TRIPLE *tlast;		/* last triple of each block in pred(bp) */
    TRIPLE *tlab;		/* LABELREFs of tlast */

    for(bp = entry_block; bp != NULL ; bp = bp->next) {
	if (!bp->is_ext_entry
	  && singleton(bp->succ)
	  && LCAST(bp->succ,BLOCK) != bp
	  && trivial_block(bp)) {
	    /*
	     * bp has only one successor, and no content
	     * worth preserving; modify all of its
	     * predecessors to refer to its successor.
	     */
	    b_succ = LCAST(bp->succ, BLOCK);
	    LFOR (lp, bp->pred) {
		b_pred = LCAST(lp, BLOCK);
		tlast = b_pred->last_triple;
		if (!ISOP(tlast->op,BRANCH_OP)) {
		    quita("block ends with op 0x%x, not a branch\n",
			tlast->op);
		    /*NOTREACHED*/
		}
		/*
		 * look for LABELREFs that reference bp
		 * and change them to reference b_succ
		 */
		TFOR(tlab, (TRIPLE*)tlast->right) {
		    if (tlab->op != IR_LABELREF) {
			quita("expected LABELREF, got 0x%x\n", tlab->op);
			/*NOTREACHED*/
		    }
		    if ((BLOCK*)tlab->left == bp) {
			tlab->left = (IR_NODE*)b_succ;
		    }
		}
		/*
		 * modify b_pred's successor list
		 * to reference b_succ instead of bp
		 */
		LFOR(lp2, b_pred->succ) {
		    if (LCAST(lp2, BLOCK) == bp) {
			delete_list(&b_pred->succ, lp2);
			(void)insert_list(&b_pred->succ, (LDATA*)b_succ,
				proc_lq);
			break;
		    }
		}
	    } /* foreach b_pred in pred(bp) */
	    /*
	     * remove bp from b_succ's predecessor list
	     */
	    LFOR(lp2, b_succ->pred) {
		if (LCAST(lp2, BLOCK) == bp) {
		    delete_list(&b_succ->pred, lp2);
		    break;
		}
	    }
	    /*
	     * b_succ inherits bp's predecessors
	     */
	    b_succ->pred = merge_lists(bp->pred, b_succ->pred);
	    bp->pred = LNULL;
	} /* if bp ... */
    } /* foreach bp */
}

/*
 * ensure that the graph being worked on is a connected region entered
 * at entry_block - except - even if the exit block is
 * not connected it is preserved
 */
static void
remove_unreach()
{
	BOOLEAN change;
	BLOCK *bp, *nextbp;

	/* visited will indicate whether the block is to be deleted */
	for(bp=entry_block;bp;bp=bp->next) {
		bp->visited = IR_FALSE;
	}
	do {
		change = IR_FALSE;
		for(bp=entry_block;bp;bp=bp->next) {
			if(bp == entry_block
			    || bp == exit_block || bp->visited == IR_TRUE)
				continue;
			if(bp->pred == LNULL) {
				/*
				 * if the block has no predecessors,
				 * and it's not the entry or exit,
				 * mark it for deletion
				 */
				bp->visited = IR_TRUE;
				change = IR_TRUE;
			} else {
				BOOLEAN preds_marked = IR_TRUE;
				LIST *lp;

				LFOR(lp,bp->pred) {
					if(LCAST(lp,BLOCK)->visited == IR_FALSE) {
						preds_marked = IR_FALSE;
						break;
					}
				}
				if(preds_marked == IR_TRUE) {
					/* all predecesors are marked - so mark this block too */
					bp->visited = IR_TRUE;
					change = IR_TRUE;
				}
			}
		}
	} while(change == IR_TRUE);
	bp = entry_block;
	nextbp = bp->next;
	while (nextbp != NULL) {
		if(nextbp->visited == IR_TRUE) {
			delete_block(nextbp);
			nextbp = bp->next;
		} else {
			bp = nextbp;
			nextbp = nextbp->next;
		}
	}
	/* recompute predecessors since some could have been deleted */
	for(bp=entry_block;bp;bp=bp->next) {
		bp->pred = LNULL;
	}
	make_pred_list();
}

void
init_cf()
{
	int i;

	indirgoto_targets = LNULL;
	labeldef_list = LNULL;
	for(i = 0; i < LABEL_HASH_SIZE; i++) {
		label_hash_tab[i] = NULL;
	}
}
