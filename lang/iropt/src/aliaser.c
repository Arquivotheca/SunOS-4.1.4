#ifndef lint
static	char sccsid[] = "@(#)aliaser.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


/*
 * aliaser.c -- routines in this file compute dynamic aliases 
 *		of variables.  The result along with the
 * 		static aliases( computed by front_end ) are
 *		recorded in the can_access field of triples
 *		ISTORE, and IFETCH for later use by iropt.  
 *		Aliaser module is called by iropt.c for 
 *		languages other than FORTRAN.
 */
 

# include <stdio.h>
# include "iropt.h"
# include "page.h"
# include "misc.h"
# include "bit_util.h"
# include "aliaser.h"

/* locals */
static	SET_PTR	 als_in,  als_out, als_newin, block_effect ;
static	int   setsize;
static	struct pt_effect  *trip_record;

static LIST **ptr_object;
static LEAF **ptr_src;

# define  NO_INDEX	-1
# define  TRECNULL(p)	(p->source == NO_INDEX && p->object == NO_INDEX && p->srctyp == AERR && p->typ == AERR) 

/***********************************************************/

/*
 * column 0 in all bit vectors correspond to the heap leaf 
 * print out the bit vector   -- debug info 
 */

static void
print_bit(set)
SET_PTR 	set;
{
	int i, j;
	LIST	*lp;

	for (i = 0; i < nblocks*npointers; i++) {
		printf("B[%d] -- PTR  %s  :  ", i/npointers,
		(char*) ((LEAF*) ptr_src[i-(i/npointers*npointers)])->pass1_id);
		for (j =0; j <nelvars; j++)
			if (bit_test(set, i,j)) {
				LFOR (lp, (LIST*) ptr_object[j])
					printf(", %s ",
						(char*) LCAST(lp, LEAF)->pass1_id);
			}	
		printf("\n");
	}

}

/***********************************************************/

/* 
 * This routine is called by print_tp to print the alias
 * rule recorded for each tripele -- Debug info           
 */

static void
print_rule(rule)
ALIAS_RULES rule;
{
	switch(rule) {
	case AERR:
		printf(" AERR ");
		break;
	case WRST:
		printf(" WRST ");
		break;
	case ADDR:
		printf(" ADDR ");
		break;
	case CPY:
		printf(" CPY ");
		break;
	case STAR:
		printf(" STAR ");
		break;
	}
}


/***********************************************************/

/* 
 * print pointer info recorded for each triple -- Debug info
 * This routine is called by fillup_canlist.
 */

static void
print_tp(b, tp, p)
int b;
TRIPLE *tp;
struct pt_effect *p;
{
	LIST *lp;

	if (p == (struct pt_effect *) NULL)
		return;
	printf("B[%d] T[%d] :  ", b, tp->tripleno);
	LFOR(lp,tp->can_access)
		printf(", %s L[%d]", (char *) LCAST(lp,LEAF)->pass1_id, 
				LCAST(lp,LEAF)->leafno);
	printf("\n		");
	print_rule(p->typ);
	if (p->object == NO_INDEX )
		printf(", PTR=%d ,", p->object);
	else
		printf(", PTR=%s ,", (char*) ((LEAF*)ptr_src[p->object])->pass1_id);
	print_rule(p->srctyp);
	if (p->source == NO_INDEX)
		printf(", VAR= %d ", p->source);
	else 
		if (p->srctyp == CPY || p->srctyp == STAR)
			printf(", VAR= %s ,", (char*) ((LEAF*)ptr_src[p->source])->pass1_id);
		else	
			LFOR(lp, (LIST *) ptr_object[p->source])
					printf(", VAR= %s ", (char*) LCAST(lp, LEAF)->pass1_id);
	printf("\n \n");
}


/***********************************************************/

/*
 * First clear the row then set the bit 
 */


static void
clear_set(set, row, bitno)	
SET_PTR	set;
int     row;
int	bitno;
{
	BIT_INDEX	temp, index;
	int 		b, i;

	temp = set->bits;
	b = (int) row * (int) setsize;
	index = &temp[b];
	for (i = 0; i < setsize; i++)
		*index++ = 0;
	bit_set(set, row, bitno);
}

/************************************************************/

/*
 * If a member of arg segment need to be set then all other 
 * arguments are also set.				   
 */


static void
arg_set(set, row)
SET_PTR set;
int 	row;
{
	LIST 	*lp;

	LFOR (lp, seg_tab[ARG_SEGNO].leaves)
		bit_set(set, row, LCAST(lp, LEAF)->elvarno);

}

/************************************************************/


/*
 * Find *p where p is a pointer; This is computed as the   
 * union of rows corresponding to each entry in row for p.
 * This routine deals with *p on the RHS of statements.   
 */


static BIT_INDEX 
rhs_star(loc, block, set, result_set)
int	loc;
SET_PTR  set;
int      block;
SET_PTR  result_set;
{
	BIT_INDEX	result, temp, index ;
	int 		b, i, j, pt;
	LIST		*lp;

	temp = set->bits;
	result = result_set->bits;

	if ( bit_test(set, loc, 0 )) {
		set_bit(result, 0);
		return;
	}
	for (i = 1; i < nelvars; i++) { 
		if ( bit_test(set, loc, i) ) {
			LFOR (lp, (LIST*) ptr_object[i])
				if ((pt = LCAST(lp, LEAF)->pointerno) > NO_INDEX) {
					b = (int) (block*npointers+pt) * (int)setsize;
					index = &temp[b];
					if (test_bit(index, 0))
						set_bit(result, 0);
					else {
						for (j = 1; j < nelvars; j++)
							if (test_bit(index, j))
								set_bit(result, j);
					}
				}
				else
					set_bit(result, i);
		}
	}
}


/************************************************************/

/*
 *  copy one bit vector row to another
 */


static void
copy_row(source_row, object_row, set)
int	source_row, object_row;
SET_PTR	set;
{
	BIT_INDEX	temp, sindex, oindex;
	int		 b, i;

	temp = set->bits;
	b = (int) source_row * (int) setsize;
	sindex = &temp[b];	/* where bits for source row starts */
	b = (int) object_row * (int) setsize;
	oindex = &temp[b];	/* where bits for object row starts */
	for (i=0; i < setsize; i++)
		*oindex ++ = *sindex ++;
}


/*****************************************************************/

/* inserts an element into can access list -- 
 * called by worst_access and  fillup_canlist and istore_init
 */

static LIST *
insert_canaccess(t, leafp)
TRIPLE	*t;
LEAF	*leafp;
{
	LIST	*acc_list;

	acc_list= NEWLIST(proc_lq);
	(LEAF *) acc_list->datap = leafp;
	LAPPEND(t->can_access, acc_list);
	return(t->can_access);
}


/*****************************************************************/

/*
 * Find a variable leaf in the subtree.  Return the leaf and the 
 * operator.  If there is no var_leaf then return NULL for the  
 * leaf and IR_ERR for the operator.  For more than one level of
 * indirection, returns NULL and IR_ERR also.			
 */ 


static LEAF *
find_varleaf(t, tp, op, side)
TRIPLE  *t , *tp;
IR_OP   op;
int     side;     /* 1 for right, 0 left */
{

	struct triple t1;
	LEAF    *leafp;
	struct pt_effect *p;

	switch(tp->op) {
	case IR_PLUS:
	case IR_MINUS:
	case IR_MULT:
	case IR_DIV:
	case IR_REMAINDER:
	case IR_AND:
	case IR_OR:
	case IR_XOR:
	case IR_LSHIFT:
	case IR_RSHIFT:
		t->op = tp->op;
		if (ISPTR(tp->left->operand.type.tword) ) {
			if (tp->left->operand.tag == ISLEAF)
				leafp = &(tp->left->leaf);
			else {
				leafp = find_varleaf(&t1, (TRIPLE*)tp->left, 
					(op==IR_IFETCH || op==IR_ISTORE)? op : tp->op,side);
				if (leafp && leafp->pointerno>NO_INDEX && op==IR_ISTORE && side)
					t->op= t1.op;
			}
		}
		else if (ISPTR(tp->right->operand.type.tword)) {
			if (tp->right->operand.tag == ISLEAF)
				leafp = &(tp->right->leaf);
			else { 
				leafp = find_varleaf(&t1, (TRIPLE*)tp->right, 
					(op==IR_IFETCH || op==IR_ISTORE)? op : tp->op, side);
				t->op=t1.op;
			}
		}
		else { 
			leafp = (LEAF *) NULL;
			t->op = IR_ERR;
		}
		break;
	
	case IR_SCALL:
	case IR_FCALL:
	case IR_CONV:
	case IR_PCONV:
	case IR_NOT:
	case IR_COMPL:
	case IR_NEG:
		t->op = tp->op;
		if (tp->left->operand.tag == ISLEAF)
			leafp = &(tp->left->leaf);
		else {
			leafp = find_varleaf(&t1, (TRIPLE*)tp->left, 
					(op==IR_IFETCH || op==IR_ISTORE)? op : tp->op, side);
			if (t->op != IR_SCALL && t->op != IR_FCALL) 
				t->op = t1.op;
			else
				leafp = &leaf_tab[0]; /* returns heap_leaf */
		}
		break;
	case IR_IFETCH:
		p = &trip_record[tp->tripleno];
		if (op == IR_IFETCH || (op == IR_ISTORE && !side)) {
			/* Go on recording effects of other IFETCHES */
			if (tp->left->operand.tag == ISTRIPLE) {
				leafp = find_varleaf(&t1, (TRIPLE*)tp->left,
					tp->op, side);
			}
			p->typ = WRST;
			p->object = NO_INDEX;
			if (tp->can_access == LNULL)  
				t->op = IR_ERR;
			leafp = (LEAF*) NULL; 
			break;
		}
		t->op = tp->op;
		if (tp->left->operand.tag == ISLEAF)
			leafp = &(tp->left->leaf);
		else 
			leafp = find_varleaf(&t1, (TRIPLE*)tp->left,
				tp->op, side);
		break;

	default:
		leafp = (LEAF *) NULL;
		t->op = IR_ERR;
		break;
		
	}
	if (!leafp || (leafp && (leafp->pointerno==NO_INDEX && 
		(leafp->elvarno==NO_INDEX && leafp->class!=ADDR_CONST_LEAF)))) {
		leafp = (LEAF *) NULL;
		t->op = IR_ERR;
	}
	return((LEAF *) leafp);
}


/*****************************************************************/

/* setup_addr  is called by assign_init and istore_init.  It deals
 * with construct such as a pointer = address of some variable.
 */

static void
setup_addr(leafp, p, set, line)
LEAF 	*leafp;
struct pt_effect *p;
SET_PTR	set;
int 	line;
{
	p->srctyp = ADDR;
	p->source = leafp->addressed_leaf->elvarno;
	if (p->source == NO_INDEX) 
		p->source = 0;
	clear_set(set, line, p->source);
# if TARGET == MC68000
	if (leafp->addressed_leaf->val.addr.seg->descr.class == ARG_SEG) 
		arg_set(set, line);
# endif

}


/*****************************************************************/

/*
 *  Initializes pointers where operator is assignment.  
 *  With ASSIGN, LHS is always a leaf.  RHS can be a   
 *  triple.					
 */


static void
assign_init( t, bk, rows)
TRIPLE  *t;
BLOCK	*bk;
int	rows;
{		
	struct triple tp1;
	LEAF	*leafp;
	int 	pnum, elnum, b, i;
	SET_PTR  result_set;
	BIT_INDEX  result, oindex, temp;
	struct pt_effect *p;

	if (!ISPTR(t->type.tword))
		return;
	leafp = &(t->left->leaf);
	if (leafp->pointerno == NO_INDEX) 
		return;
	pnum = leafp->pointerno;
	p = &trip_record[t->tripleno];
	p->object = pnum;

	/* look at right hand side, find_leaf */
	/* return elvarno                     */


	if (t->right->operand.tag == ISLEAF) {
		leafp = &(t->right->leaf);
		if (leafp && leafp->class == ADDR_CONST_LEAF) {
			setup_addr(leafp, p, block_effect, rows+pnum);
			return;
		}
		else {
			if (leafp->pointerno == NO_INDEX) {
				p->srctyp = ADDR;
				p->source = 0;
				clear_set(block_effect, rows+pnum, p->source);
			}
			else {
				p->srctyp = CPY;
				p->source = leafp->pointerno;
				copy_row(rows+leafp->pointerno, rows+pnum, block_effect);
			}
		}
	}
	else {
		leafp = find_varleaf(&tp1, (TRIPLE*)t->right, t->op, 1);
		if (leafp && leafp->class == ADDR_CONST_LEAF) {
			setup_addr(leafp, p, block_effect, rows+pnum);
			return;
		}
		switch(tp1.op) {
		case IR_IFETCH:

			/* tp1->left can be a triple */
			/* copy points(tp1->left.leaf) */
			/*    x = *p			*/

			if (leafp) {
				elnum = leafp->pointerno;
				p->srctyp = STAR;
				result_set = new_heap_set(1, nelvars);
				rhs_star(rows+elnum, bk->blockno, block_effect,
					result_set);
				result = result_set->bits;
				b = (int) (rows+pnum) * (int ) setsize;
				temp = block_effect->bits;
				oindex = &temp[b];
				for (i = 0; i < setsize; i++)
					*oindex ++ = *result++ ;	
				free_heap_set(result_set);
				break;
			}
		case IR_FCALL:
		case IR_ERR:

			/* when IR_ERR is returned, leafp is NULL (worst_case) */

			elnum = 0;
			p->srctyp = ADDR;
			clear_set( block_effect, rows+pnum, elnum);
			break;
		default:	
			if (leafp && leafp->pointerno > NO_INDEX) {
				elnum = leafp->pointerno;
				p->srctyp = CPY;
				copy_row(rows+elnum, rows+pnum, block_effect);
			}
			else {
				elnum = 0;
				p->srctyp = ADDR;
				clear_set( block_effect, rows+pnum, elnum);
			}
			break;
		}  /* switch */
		p->source = elnum;
	}  /* else */	
}

/****************************************************************/

/*
 * Set everything in object list that is a pointer to heap leaf
 */


static void
worst_case(set, rows)
SET_PTR set;
int  rows;
{
	LEAF    *leafp;
	int 	pnum, i;
	LIST	*lp;

	for( i=0; i < nelvars; i++) 
		LFOR (lp, (LIST*) ptr_object[i]) {
			leafp = LCAST(lp, LEAF);
			if (leafp && (pnum = leafp->pointerno) > NO_INDEX)
				clear_set( set, rows+pnum, 0);
		
		}
}


/**********************************************************/

/*
 * Initializes pointers where the operator is an ISTORE. 
 * With ISTORE, both LHS and RHS can be triples.	 
 */


static void
istore_init(t, bk, rows)
TRIPLE  *t;
BLOCK	*bk;
int	rows;
{
	struct  triple tp;
	LEAF 	*leafp;
	int 	pnum, elnum, b, i, j, pt;
	SET_PTR  result_set;
	BIT_INDEX  result, oindex, temp, rindex;	
	LIST	*lp;
	struct  pt_effect *p;

	if (t->left->operand.tag == ISLEAF) 	
		leafp = &(t->left->leaf);
	else 
		leafp = find_varleaf(&tp, (TRIPLE*)t->left, t->op, 0);
	if (leafp) {
		if (leafp->class == ADDR_CONST_LEAF) {
			t->can_access=insert_canaccess(t, leafp->addressed_leaf);
			pnum = leafp->addressed_leaf->pointerno;
			if (pnum == NO_INDEX) {  /* nothing to propagate */
				if (t->right->operand.tag == ISTRIPLE) {
					leafp = find_varleaf(&tp,
						(TRIPLE*)t->right, t->op, 1);
				}
				return;
			}
		}
		else
			pnum = leafp->pointerno;
	}
	p = &trip_record[t->tripleno];
	if (!leafp || (pnum == NO_INDEX)) {	
		p->typ = WRST;
		p->object = NO_INDEX;
		p->source = NO_INDEX;
		worst_case(block_effect, rows);
		rindex = (BIT_INDEX) NULL;
	}
	else {
		p->typ = STAR;
		p->object = pnum;
		temp = block_effect->bits;
		b = (int) (rows+ pnum) * (int) setsize;
		rindex = &temp[b];
	}
	if (t->right->operand.tag == ISLEAF) {
		leafp = &(t->right->leaf);
		if (leafp && leafp->class==ADDR_CONST_LEAF) {
			p->source = leafp->addressed_leaf->elvarno;
			if (p->source == NO_INDEX)
				p->source = 0;
			p->srctyp = ADDR;
			for (i = 0; (i < nelvars && rindex); i++) 
				LFOR (lp, (LIST*) ptr_object[i])
					if ( test_bit(rindex, i) && 
						(pt = LCAST(lp, LEAF)->pointerno) > NO_INDEX)
						setup_addr(leafp, p, block_effect, rows+pt);
			return;
		}
		if (leafp->pointerno == NO_INDEX) {
			p->source = 0;
			p->srctyp = ADDR;
		}
		else {
			p->source = leafp->pointerno;
			p->srctyp = CPY;
		}

		/* for every member of row for pnum that is also a pointer */

		if (rindex) {
			for (i = 0; i < nelvars; i++) 
				LFOR (lp, (LIST*) ptr_object[i]) {
					if (test_bit(rindex,i) && 
							(pt = LCAST(lp, LEAF)->pointerno) > NO_INDEX)
						if (p->srctyp == CPY)
							copy_row(rows+leafp->pointerno, 
									rows+pt, block_effect);
						else
							clear_set(block_effect, rows+pt, 
									p->source);
				}
		}
	}	
	else { 	/* ISTRIPLE */
		leafp = find_varleaf(&tp, (TRIPLE*)t->right, t->op, 1);
		if (leafp && leafp->class==ADDR_CONST_LEAF) {
			p->source = leafp->addressed_leaf->elvarno;
			p->srctyp = ADDR;
			for (i = 0; (i < nelvars && rindex); i++) 
				LFOR (lp, (LIST*) ptr_object[i])
					if ( test_bit(rindex, i) && 
						(pt = LCAST(lp, LEAF)->pointerno) > NO_INDEX) 
						setup_addr(leafp, p, block_effect, rows+pt);
			return;
		}
		switch(tp.op) {
		case IR_IFETCH:
			if (leafp) {
				elnum = leafp->pointerno;
				p->srctyp = STAR;
				result_set = new_heap_set(1, nelvars);
				rhs_star(rows+elnum, bk->blockno, block_effect,
					result_set);
				result = result_set->bits;
				for (i = 0; (i < nelvars && rindex); i++) {
					LFOR (lp, (LIST*) ptr_object[i])
						if ( test_bit(rindex, i) && 
							(pt=LCAST(lp, LEAF)->pointerno) > NO_INDEX) {	
							b = (int) (rows+pt) * (int) setsize;
							temp = block_effect->bits;
							oindex = &temp[b];
							for (j = 0; j < setsize; j++)
								*oindex ++ = *result++ ;	
						}
				}
				free_heap_set(result_set);
				break;
			}
		default:	
			if (!leafp || tp.op == IR_ERR || (leafp && 
				leafp->pointerno==NO_INDEX)) {
				p->srctyp = ADDR;
				elnum = 0;
				for (i = 0; (i < nelvars && rindex); i++) {
					LFOR (lp, (LIST*) ptr_object[i])
						if (test_bit(rindex,i) && (pt = 
							LCAST(lp, LEAF)->pointerno) > NO_INDEX)	
							clear_set(block_effect, rows+pt, 
									elnum);
				}
			}
			else {
				p->srctyp = CPY;
				elnum = leafp->pointerno;
			}
			for (i = 0; (i < nelvars && rindex); i++) {
				LFOR (lp, (LIST*) ptr_object[i])
					if (test_bit(rindex,i) && 
						(pt = LCAST(lp, LEAF)->pointerno) > NO_INDEX)
						if (p->srctyp == CPY)
							copy_row(rows+leafp->pointerno, rows+pt, block_effect);
						else
							clear_set(block_effect, rows+pt, elnum);
			}
			break;
		}	
		p->source = elnum;
	}
}

/*****************************************************************/

/*
 * Records the initial places that a pointer might point to 
 * for each triple that involves a pointer.               
 */


static void
pointer_init()
{
	TRIPLE *tp, *tlp, *first;        /* steps through the triples */
	LEAF   *leafp;     /* steps through leaves  */
	BLOCK  *bp; 	   /* steps through blocks     */
	int pnum, nrows;
	struct triple	t1;
	struct pt_effect *p;

	/* Initialize all global and formal pointer parameters  */
	/* to heap leaf (worst case).				*/

	for( pnum = 0; pnum < npointers; pnum++) {
		leafp = (LEAF *) ptr_src[pnum];
		if( leafp && (leafp->val.addr.seg->descr.external == EXTSTG_SEG
		    || leafp->val.addr.seg->descr.class == ARG_SEG))
			clear_set(block_effect, pnum, 0);
	}
	for(bp=entry_block; bp; bp=bp->next) {
		first =  TNULL; 
		nrows = bp->blockno * npointers;
		if(bp->last_triple) 
			first=bp->last_triple->tnext;
                TFOR(tp,first){
			p = &trip_record[tp->tripleno];
			switch(tp->op) {
			case IR_ASSIGN:
				if (ISPTR(tp->type.tword))
					assign_init(tp, bp, nrows);
				else 
	
					/* Record effect of IFETCH, if any */

					if (tp->right->operand.tag == ISTRIPLE)
						(void)find_varleaf(&t1, (TRIPLE*)tp->right,
							tp->op, 1);
				break;
			case IR_ISTORE:
				istore_init( tp, bp, nrows );
				break;
			case IR_FCALL:
			case IR_SCALL:
				worst_case( block_effect, nrows );
				p->typ = WRST;

				/* record the effect of ifetch appearing */
				/* as parameters.			 */

				if (tp->right) {
					TFOR(tlp, tp->right->triple.tnext)
						if (tlp->left->operand.tag == ISTRIPLE)
							leafp = find_varleaf(&t1,
								(TRIPLE*)tlp->left, tlp->op, 0);
				}
				break;
			case IR_IFETCH:
				if(tp->left && tp->left->operand.tag == ISTRIPLE)
					leafp = find_varleaf(&t1,
						(TRIPLE*)tp->left, tp->op, 0);
				else
					leafp = &(tp->left->leaf);
				if (leafp) {
					if (leafp->class != ADDR_CONST_LEAF) {
						if (leafp->pointerno > NO_INDEX) {
							p->typ = STAR;
							p->object=leafp->pointerno;
						}
						else {
							p->typ = WRST;
							p->object = NO_INDEX;
						}
					}
					else  /* addr_const */
						tp->can_access=insert_canaccess(tp,leafp->addressed_leaf);
				}
				else {
						p->typ = WRST;
						p->object = NO_INDEX;
				}
				break;
			default:
				break;
			}  /* switch */
		} /* TFOR */
		if (debugflag[ALIAS_DEBUG] && SHOWCOOKED) { 
			TFOR(tp,first){
				print_tp(bp->blockno, tp, &trip_record[tp->tripleno]);
			}
		}

	}  /* for bp */
	
}


/*****************************************************************/

/* Addr_rule is called by append_ptinfo to set bits corresponding
 * to the sourceno in the given set.  Also deals with members
 * of arg_seg.
 */

static void
addr_rule(set, line, element)
SET_PTR	set;
int 	line, element;
{

	clear_set(set, line, element); 
# if TARGET == MC68000
    {
	LIST	*lp;
	LFOR (lp, (LIST*) ptr_object[element])
		if (LCAST(lp, LEAF)->val.addr.seg->descr.class == ARG_SEG)
			arg_set(set, line);

    }
# endif
}


/*****************************************************************/

/*
 * Update the given bit vector with the effect of the 
 * given triple.				     
 */


static void
append_ptinfo(set, bp, index, t)
SET_PTR		set;
BLOCK		*bp;
BIT_INDEX	index;
struct pt_effect  *t;
{
	register  int  i, j, b1;
	SET_PTR result_set;
	BIT_INDEX	rindex, result, oindex, temp;
	int	nrows, pt;
	LIST	*lp;


	nrows = bp->blockno * npointers;
	if (t->typ == WRST) {
		worst_case(set, nrows);
		return;
	}
	if (t->typ == STAR) {
		b1 = (int) (t->object) * (int) setsize;
		rindex = &index[b1];
	}
	else 
		rindex = (BIT_INDEX) NULL;
	switch (t->srctyp) {	
	case ADDR:
		if (rindex) {
			for (i=0; i < nelvars; i++) 
				LFOR (lp, (LIST*) ptr_object[i])
					if ( test_bit(rindex,i) && (pt=LCAST(lp,LEAF)->pointerno) > NO_INDEX)
						addr_rule(set, nrows+pt, t->source);
		}
		else 
			addr_rule(set, nrows+t->object, t->source);
		break;
	case CPY:
		if (rindex) {
			for (i=0; i < nelvars; i++) 
				LFOR (lp, (LIST*) ptr_object[i])
					if ( test_bit(rindex, i) && 
						(pt = LCAST(lp, LEAF)->pointerno) > NO_INDEX)	
						copy_row(nrows+t->source, nrows+pt, set);
		}
		else {
			copy_row(nrows+t->source, nrows+t->object, set);
	}
		break;
	case STAR:
		temp = set->bits;
		result_set = new_heap_set(1, nelvars);
		rhs_star(nrows+t->source, bp->blockno, set, result_set);
		result = result_set->bits;
		if (rindex) {
			for (i=0; i< nelvars; i++) {
				LFOR (lp, (LIST*) ptr_object[i])
					if ( test_bit(rindex, i) && 
						(pt = LCAST(lp, LEAF)->pointerno) > NO_INDEX) {	
						b1 = (int ) (nrows+pt) * (int) setsize;
						oindex = &temp[b1];
						for (j = 0; j < setsize; j++)
							*oindex++ = *result++;	
					}
			}
		}
		else {
			b1 = (int) (nrows+t->object) * (int) setsize;
			oindex = &temp[b1];
			for (i =0; i < setsize; i++) 
				*oindex++ = *result++;
		}
		free_heap_set(result_set);
		break;
	default:
		if (rindex) {
			for (i=0; i < nelvars; i++) 
				LFOR (lp, (LIST*) ptr_object[i])
					if ( test_bit(rindex, i) && 
						(pt = LCAST(lp, LEAF)->pointerno) > NO_INDEX)  
						clear_set(set, nrows+pt, 0); 
					
		}
		else 
			if (t->object > NO_INDEX)
				clear_set(set, nrows+t->object, 0);
		break;
	}

}

/*****************************************************************/

/*
 *  Recompute out for a changed block
 */

static BIT_INDEX
newout(set, bp, index, b )
SET_PTR		set;
BLOCK		*bp;
BIT_INDEX	index;
register int 	b;
{
	register  int  i;
	BIT_INDEX	oindex, temp;
	TRIPLE	  *tp, *first;
	struct pt_effect  *t;
	
	first = TNULL;
	if(bp->last_triple)
		first=bp->last_triple->tnext;
	TFOR(tp,first){
		t = &trip_record[tp->tripleno];	
		if (!TRECNULL(t) && tp->op != IR_IFETCH) 
			append_ptinfo(set, bp, index, t);
	}
	temp = set->bits;
	oindex = &temp[b];
	for (i = 0; i< npointers*setsize; i++)
		*index++ = *oindex++; 
	return(index);
}


/*****************************************************************/

/*
 *   For cases where worst case is applicable, 
 *   some IFETCHs and some ISTOREs.  The worst_access 
 *   routine is called to set the can access list to all 
 *   externals plus  locals whose address have been taken.
 */

static void
worst_access(t)
TRIPLE	*t;
{
	LIST 	*lp;

	/* must be expanded with heap_leaf's overlaps when implicit.c 
		is fixed */
	LFOR (lp, (LIST *) ptr_object[0]) 
		t->can_access = insert_canaccess(t, LCAST(lp, LEAF));
}


/*****************************************************************/

/*
 *   Fill up can access_list for each triple, so that optimizer  
 *   can use the alias information.				
 */



static void
fillup_canlist(  )
{
	TRIPLE  *tp, *first;
	BLOCK	*bp;
	LEAF	*leafp;
	LIST	*lp;
	BIT_INDEX	index, rindex, result, temp;
	int	b, b1, b2, i, j, nrows, pt;
	struct  pt_effect  *t, *p;
	BOOLEAN change;


	temp = block_effect->bits;
	result = (BIT_INDEX ) NULL;
	for (bp=entry_block; bp; bp=bp->next) {
		first = TNULL;
		nrows = bp->blockno * npointers;
		b = (int) nrows * (int) setsize;
		index = &temp[b];
		if (bp->last_triple)
			first = bp->last_triple->tnext;
		TFOR(tp, first) {
			p = &trip_record[tp->tripleno];
			if (!TRECNULL(p) && ((tp->op == IR_IFETCH || tp->op == IR_ISTORE) &&
				p->srctyp != WRST && tp->can_access == LNULL)) {
				if (p->object == NO_INDEX &&  p->typ == WRST) 
					worst_access(tp);
				else {
					leafp = (LEAF *) ptr_src[p->object];
					b1 = (int) (nrows+leafp->pointerno) * (int) setsize;
					rindex = &temp[b1];
					result = temp;
					for (i=0; i < setsize; i++)
						*result++ = *rindex++;
					change = IR_TRUE;
					while (change) {
						change = IR_FALSE;
						result = temp;
						for (i=0; i<nelvars; i++) {
							LFOR (lp, (LIST*) ptr_object[i])
								if ((test_bit(result,i)) && 
									((pt=LCAST(lp, LEAF)->pointerno) > NO_INDEX)) {
									b2 = (int)(nrows+pt)*(int)setsize;
									rindex= &temp[b2];
									for (j=0; j<nelvars; j++) 
										if((test_bit(rindex,j)) &&
											!(test_bit(result,j))) {
											change = IR_TRUE;
											break;
										}
									if (change) {
										{
										register BIT_INDEX  res_ptr = result;
										
										for (j=0; j<setsize; j++)
											*res_ptr++ |= *rindex++;	
										}
									}
								}
						}
					}
					for (i = 0; i < nelvars; i++) {
						if (test_bit(result, i)) {  
							LFOR (lp, (LIST *) ptr_object[i]) 
								/* This is a kludge for implicit.c */
								/* The list should also be expanded with overlaps */
/*
								if ((leafp=LCAST(lp,LEAF))->val.addr.seg->descr.class != ARG_SEG) 
*/
									tp->can_access = insert_canaccess(tp, LCAST(lp,LEAF));
						}
					}
				}
			}
			if (tp->can_access)
				tp->can_access = order_list(tp->can_access, proc_lq);
			if (debugflag[ALIAS_DEBUG] && SHOWCOOKED) 
				print_tp(bp->blockno, tp, p);

			if (!TRECNULL(p) && tp->op != IR_IFETCH) {
				t = &trip_record[tp->tripleno];
				append_ptinfo(block_effect, bp, index, t);	
			}
		}
	}

}

/*******************************************************************/

/*
 * Compute pointer in and out for each block.		 
 */


static void
compute_alias_in_out()
{
	BIT_INDEX	in, out, trans, newin;
	/* point to sets for a specific block */

	BIT_INDEX	in_index, out_index,
			newin_index, result;
	BLOCK		*bp;
	BOOLEAN		change, block_change;
	int		passn = 0;
	int 		nrows;
	register	int  b, temp, temp1, i;

	in = als_in->bits;
	out = als_out->bits;
	trans = block_effect->bits;
	newin = als_newin->bits;

	/* initialize in[b] = 0 and out[b] = block_effect[b] */

	temp = nblocks * npointers * setsize;
	temp1 = npointers * setsize;
	{
		register BIT_INDEX  	in_ptr=in, 
					out_ptr=out, 
					trans_ptr=trans;
	
		for (i=0; i < temp; i++) {
			*in_ptr++ = 0;
			*out_ptr++ = *trans_ptr++;
		}
	}	
	change = IR_TRUE;
	while (change == IR_TRUE) {
		if (debugflag[ALIAS_DEBUG] && SHOWCOOKED) {
			printf("passn =%d \n \n",passn);
			printf("\n IN :  \n");
			print_bit(als_in);
			printf("\n OUT : \n");
			print_bit(als_out);
			printf("\n");
		}
		passn++;
		change = IR_FALSE;
		for (bp = entry_block; bp; bp = bp->dfonext) {
			nrows = bp->blockno * npointers;
			b = (int) nrows * (int) setsize;
			in_index = &in[b];
			out_index = &out[b];
			newin_index = &newin[b];

			/* compute newin for each of block b's predecessors */
			/* newin is set to the union of pred's out's        */
			
			{
				register BIT_INDEX	newin_ptr = newin_index;
				
				for (i = 0; i < temp1; i++) {
					*newin_ptr++ = 0;
				}
			}
			if (bp->pred) {
				register LIST   *lp, *pred;
				
				pred = bp->pred;
				LFOR(lp, pred) {
					result = &out[(int) LCAST(lp, BLOCK)->blockno*
							npointers * (int) setsize];
					{
						register BIT_INDEX newin_ptr = newin_index; 

						for(i = 0; i < temp1; i++) {
							*newin_ptr++ |= *result++;
						}
					}
				}
				/* test if in eq newin */
				
				block_change = IR_FALSE;
				{
					register BIT_INDEX  	in_ptr=in_index, 
								newin_ptr=newin_index;
			
						for (i=0; i<temp1; i++) {
							if ( *in_ptr != *newin_ptr )  
								block_change = IR_TRUE;
							else {
								in_ptr++;
								newin_ptr++; 
							}
						}
				}
	
				out_index = &out[b];
				if (block_change == IR_TRUE) {
				
				/* set in[b] to newin[b] and out[b] to complete effect of */
				/* statements within block b applied to in[b]             */
	
					{
						register  BIT_INDEX  	in_ptr=in_index, 
							   		newin_ptr=newin_index, 
										out_ptr=out_index;

						for (i=0; i < temp1; i++) {
							*in_ptr = *newin_ptr;
							in_ptr++;
							newin_ptr++;
						}
						newin_index = newout(als_newin, bp, newin_index, b);
						newin_index = &newin[b];
						for (i=0; i < temp1; i++) {
							*out_ptr++ = *newin_index++;
						}
					}
					change = IR_TRUE;
				}
			}  /* if bp->pred */
		}     /* for bp */
	}  /* while change */
}

/*******************************************************************/

/*
 * The main driver for the alias computation 
 */


void
find_aliases()
{
	int   nrows;
	register  int  i;
	BIT_INDEX    in, trans;
	LIST	*lp;
	LEAF	*leafp;
	LIST	*acc_list;

	if (!DO_ALIASING(hdr)) {
		return;
	}
	ptr_object = (LIST **) ckalloc((unsigned)nelvars , sizeof(LIST));
	ptr_src = (LEAF **) ckalloc((unsigned)npointers, sizeof(LEAF));
	for(leafp=leaf_tab; leafp != NULL; leafp = leafp->next_leaf) {
		if (leafp->elvarno > NO_INDEX  ) {
			acc_list = NEWLIST(tmp_lq);
			(LEAF*)acc_list->datap = leafp;
			LAPPEND((LIST *)ptr_object[leafp->elvarno], acc_list);
		}
		if (leafp->pointerno > NO_INDEX )
			(LEAF*)ptr_src[leafp->pointerno] = leafp;
	}
	setsize = (roundup( (unsigned)nelvars, (unsigned)BPW)) / BPW;
	nrows = nblocks * npointers;
	if (debugflag[ALIAS_DEBUG] && SHOWCOOKED) {
	 	printf("NBLOCKS = %d , NPOINTERS = %d , NVARS= %d \n",
			nblocks,npointers, nelvars); 
		if (npointers > 0) {
			printf("\n POINTER SET \n");
			for (i=0; i<npointers; i++) {
				leafp = (LEAF*) ptr_src[i];
				if (leafp)
					printf("%d   %s L[%d] \n", i, (char*) leafp->pass1_id,
							leafp->leafno);
			}
		}
		if (nelvars > 0) {
			printf("\n COLUMN SET ");
			for (i=0; i<nelvars; i++) {
				printf("\n %d  ", i);
				LFOR (lp, (LIST*) ptr_object[i])
					printf(", %s L[%d] ",
						(char*) LCAST(lp, LEAF)->pass1_id,
							LCAST(lp, LEAF)->leafno);
			}
		}
		printf("\n");
	}
	trip_record = (struct pt_effect *)
		heap_tmpalloc((unsigned)ntriples*sizeof(struct pt_effect));
	for (i=0; i<ntriples; i++) {
		trip_record[i].source = NO_INDEX;
		trip_record[i].object = NO_INDEX;
		trip_record[i].srctyp = trip_record[i].typ = AERR;
	}
	block_effect = new_heap_set(nrows, nelvars);
	pointer_init();
	if (debugflag[ALIAS_DEBUG] && SHOWCOOKED) {
		printf("\n INITIAL BLOCK_EFFECT  :  \n");
		print_bit(block_effect );
		printf("\n");	
	}
	als_newin = new_heap_set(nrows, nelvars);
	als_in = new_heap_set(nrows, nelvars);
	als_out = new_heap_set(nrows, nelvars);
	compute_alias_in_out();
	if (debugflag[ALIAS_DEBUG] && SHOWCOOKED) {
		printf("\n IN :  \n");
		print_bit(als_in);
		printf("\n OUT : \n");
		print_bit(als_out);
		printf("\n");
	}

	/* fill up can_access field for each triple */

	in = als_in->bits;
	trans = block_effect->bits;
	
	/*  set block-effect to in for all blocks */

	{
		BIT_INDEX   in_ptr = in , trans_ptr=trans ; 
		
		for (i=0; i < (nrows*setsize); i++)
			*trans_ptr++ = *in_ptr++;	
	}
	fillup_canlist();
	empty_listq(tmp_lq);           /* frees up ptr_object */
	free_heap_set(block_effect);
	free_heap_set(als_newin);
	free_heap_set(als_in);
	free_heap_set(als_out);
	heap_tmpfree((char*)trip_record);
}
