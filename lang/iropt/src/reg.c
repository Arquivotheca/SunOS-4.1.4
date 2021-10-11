#ifndef lint
static	char sccsid[] = "@(#)reg.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * This file contains the utility routines for doing register allocation.
 */

#include "iropt.h"
#include "page.h"
#include "misc.h"
#include "loop.h"
#include "bit_util.h"
#include <stdio.h>
#include <ctype.h>
#include "reg.h"
#include "reg_alloc.h"
#include "recordrefs.h"
#include "cost.h"
#define TWOTO31_1	2147483647;	/*  2 ** 31 - 1 */

/* global */

int regmask = 0;
int fregmask = 0;
int nwebs = 0;
int n_dsort = 0;
int n_asort = 0;
int n_fsort = 0;
int alloc_web= 0;
WEB *web_free = NULL;
WEB *web_head = NULL;
WEB **defs_to_webs = NULL;
LIST *sort_d_list = LNULL;
LIST *sort_a_list = LNULL;
LIST *sort_f_list = LNULL;

#if TARGET == MC68000
int fpa_base_reg;
COST fpa_base_weight;
#endif

#if TARGET == SPARC
/* total number of cycles to save and restore each floating point
   register at each CALL in this procedure */
COST save_f_reg;	
#endif

/* local */

static void	set_pred(/* set,  bp */);
static BOOLEAN	must_merge(/* web1, web2 */);
static void	merge_webs(/* lp1, lp2 */);
static void	print_web(/* show_conflict, lp, which_tree */);
static void	insert_tree(/* head, sort_el */);
static void	listed(/* tree, sort_list */);
static int	preference(/*web*/);
static int	preference_set(/*web*/);
static int	exclude_set(/*web*/);
static BOOLEAN	short_life(/*web*/);
static BOOLEAN	can_share(/*base_web, cur_web, pass*/);

#if TARGET == MC68000
static void	compute_fpa_base_weight();

static char *dreg_name[] = {
	"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7"
};

static char *areg_name[] = {
	"A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7"
};

static char *freg_name[] = {
	"F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7"
};

static char *fpareg_name[] = {
	"FPA0", "FPA1", "FPA2", "FPA3", "FPA4", "FPA5", "FPA6", "FPA7",
	"FPA8", "FPA9", "FPA10", "FPA11", "FPA12", "FPA13", "FPA14", "FPA15"
};
#endif

#if TARGET == SPARC
static char *dreg_name[] = {
	"G0", "G1", "G2", "G3", "G4", "G5", "G6", "G7",
	"O0", "O1", "O2", "O3", "O4", "O5", "O6", "O7",
	"L0", "L1", "L2", "L3", "L4", "L5", "L6", "L7",
	"I0", "I1", "I2", "I3", "I4", "I5", "I6", "I7"
};

static char *freg_name[] = {
	"F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7",
	"F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15",
	"F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23",
	"F24", "F25", "F26", "F27", "F28", "F29", "F30", "F31"
};
#endif

#if TARGET == I386
static char *reg_name[] = {
/*Modified to conform to ../../i386/machdep.h ..KMC*/
	"EAX", "EDX", "ECX", "ST(0)", 
	"WF2", "WF4", "WF6", "WF8",
	"EBX", "ESI", "EDI",
	"ESP", "EBP",
	"WF10", "WF12", "WF14", "WF16",
	"WF18", "WF20", "WF22", "WF24",
	"WF26", "WF28", "WF30"
};
#endif

#if TARGET == SPARC
/*
 * On SPARC, the caller is responsible for saving any floating
 * point registers live at the call site.  Here we estimate the
 * total cost/register due to saving and restoring FP registers.
 * We assume that all such registers will be saved and restored
 * around call sites, regardless of whether they are actually live.
 */
void
compute_fpsave_cost()
{
	register BLOCK *bp;
	register TRIPLE *tp, *first_triple;

	save_f_reg = 0;
	for( bp = entry_block; bp; bp = bp->next ) {
		first_triple = bp->last_triple->tnext;
		TFOR( tp, first_triple ) {
			if( tp->op == IR_SCALL || tp->op == IR_FCALL ) {
				/*
				 * COST_SAVE_FP is the cost of saving
				 * and restoring a pair of FP registers.
				 */
				save_f_reg = add_costs((COST)save_f_reg,
					mul_costs((COST)bp->loop_weight,
						(COST)COST_SAVE_FP));
			}
		}
	}
}

#endif /* TARGET == SPARC */

#if TARGET == MC68000

/*
 * On Sun-3 machines with Weitek FPA's, the cost of a floating
 * point operation can be reduced by keeping the address of the
 * FPA in an address register.  Here we estimate the potential gain
 * to be had by dedicating an A-register to the FPA over the entire
 * procedure.
 */
static void
compute_fpa_base_weight()
{
	register BLOCK *bp;
	register TRIPLE *tp, *first_triple;

	fpa_base_weight = (COST)0;
	if( !usefpa ) {
		return;
	}
	for( bp = entry_block; bp; bp = bp->next ) {
		first_triple = bp->last_triple->tnext;
		TFOR( tp, first_triple ) {
			if( ISFLOAT(tp->type.tword ) ) {
				/*
				 * every reference counts for two cycles:
				 *	movl	am@(disp16),xxx.l
				 * =>	10 cycles, cache case
				 *	movl	am@(disp16),an@(disp16)
				 * =>	8 cycles, cache case
				 */
				fpa_base_weight = add_costs(fpa_base_weight,
				    mul_costs((COST)bp->loop_weight,
					(COST)FPA_BASE_WEIGHT));
			} else if( ISDOUBLE(tp->type.tword ) ) {
				/*
				 * same cost as above, multiplied by 2
				 */
				fpa_base_weight = add_costs(fpa_base_weight,
				    mul_costs((COST)bp->loop_weight,
					(COST)(FPA_BASE_WEIGHT*2)));
			}
		}
	}
}
#endif
	
void
reg_init()
{
	unsigned nbytes;

	nwebs = n_dsort = n_asort = n_fsort = alloc_web = 0;
	sort_d_list = sort_a_list = sort_f_list = LNULL;
	web_head = NULL;
	if(reg_lq == NULL) reg_lq = new_listq();

#if TARGET == MC68000
	fpa_base_reg = -1;
	compute_fpa_base_weight();
#endif

#if TARGET == SPARC
	regmask = AVAIL_INT_REG;
	fregmask = AVAIL_F_REG;
	compute_fpsave_cost();
#endif

#if TARGET == I386
	regmask = AVAIL_REG;
#endif

	nbytes = roundup((unsigned)nvardefs,BPW) * sizeof(WEB*);
	defs_to_webs = (WEB**)heap_tmpalloc(nbytes);
}

void
reg_cleanup()
{
	heap_tmpfree((char*)defs_to_webs);
}

BOOLEAN
can_in_reg( leafp ) LEAF *leafp;
{
	VAR_REF *vp;		/* for iterating through a leaf's var_refs */
	BOOLEAN interesting;	/* true if we find op worth using a reg */
	IR_OP op;		/* temp for op accessed through var_ref */

	if( (ISFTN( leafp->type.tword ) ) || (ISARY( leafp->type.tword ) ) )
	/* no function or array */
		return( IR_FALSE );


	if( partial_opt ) {
		if( EXT_VAR(leafp) ) {
			return IR_FALSE;
		}
	} else {
		/*
		 * if leaf has no interesting defs or uses, don't bother
		 */
		interesting = IR_FALSE;
		for (vp = leafp->references; vp != NULL; vp = vp->next_vref) {
			op = vp->site.tp->op;
			if (op != IR_IMPLICITDEF && op != IR_IMPLICITUSE
			    && op != IR_ENTRYDEF && op != IR_EXITUSE) {
				interesting = IR_TRUE;
				break;
			}
		}
		if (!interesting) {
			leafp->no_reg = IR_TRUE;
			return IR_FALSE;
		}
	}
#if TARGET == MC68000
	if( ( use68881 || usefpa ) && ( ISREAL( leafp->type.tword ) ) ) {
		return( IR_TRUE );
	}
#endif

#if TARGET == SPARC || TARGET == I386
/* has floating point register always */
	if( ISREAL( leafp->type.tword) ) {
		return( IR_TRUE );
	}
#endif
	/*
	 * avoid: structures, unions, bitfields, things bigger than a word.
	 */
	switch(leafp->type.tword) {
	case STRTY:
	case UNIONTY:
	case BITFIELD:
		return( IR_FALSE );
	default:
		return (BOOLEAN)(type_size( leafp->type ) <= (SZLONG/SZCHAR));
	}
}

/*
 * This routine will build a new web for an object that is
 * seen to be suitable for register allocation.  It updates
 * the defs_to_webs map to point to the newly created web.
 */

void
new_web(leafp, rp)
	LEAF *leafp;
	VAR_REF *rp;
{
	WEB *web;
	register LIST *lp;
	register TRIPLE *tp1, *tp2;

	tp1 = rp->site.tp;
	if( tp1->op == IR_ENTRYDEF || tp1->op == IR_IMPLICITDEF )
	{
		LFOR( lp, tp1->canreach )
		{
			tp2 = LCAST( lp, VAR_REF )->site.tp;
			if( tp2->op != IR_EXITUSE && tp2->op != IR_IMPLICITUSE )
				goto do_it;
			/*  else */
			if( tp2->reachdef1 != tp2->reachdef1->next )
			/* there are some other definition can reach here */
				goto do_it;
		}
		/* do NOT even creat web for it */
		return;
	}
do_it:
	if (web_free == NULL) {
		web = alloc_new_web(IR_FALSE);
	} else {
		web = web_free;
		web_free = web->next_web;
	}
	/*
	 * Build a doubly linked list (for fast random deletion)
	 * in LIFO order (so that all webs belonging to a given
	 * leaf are automatically at the front of the list)
	 */
	web->next_web = web_head;
	web->prev_web = NULL;
	if (web_head != NULL)
		web_head->prev_web = web;
	web_head = web;
	if (rp->defno == NULLINDEX) {
		quit("bad var_def in new_web");
		/*NOTREACHED*/
	}
	defs_to_webs[rp->defno] = web;
	web->leaf = leafp;
	web->define_at = NEWLIST(reg_lq);
	web->define_at->datap = (LDATA*)rp;
	web->use = order_list(tp1->canreach, reg_lq);
	web->d_cont = web->a_cont = web->f_cont = 0;
	web->same_reg = (WEB *)NULL;
	web->sort_d = web->sort_a = web->sort_f = (SORT *)NULL;
	web->import = ( tp1->op == IR_ENTRYDEF ) ? IR_TRUE : IR_FALSE;
}

void
delete_web(web)
	WEB *web;
{
	if (web == NULL) {
		quit("null web in delete_web\n");
		/*NOTREACHED*/
	}
	if (web->prev_web != NULL) {
		web->prev_web->next_web = web->next_web;
	}
	if (web->next_web != NULL) {
		web->next_web->prev_web = web->prev_web;
	}
	if (web == web_head) {
		web_head = web->next_web;
	}
	bzero((char*)web, sizeof(*web));
	web->next_web = web_free;
	web_free = web;
}

void
free_webs()
{
	web_free = NULL;
	defs_to_webs = NULL;
	(void)alloc_new_web(IR_TRUE);
}

static void
set_pred( set,  bp ) SET set; BLOCK *bp;
{
	register LIST *lp, *lp1;
	register int bitno;

	if( bp == (BLOCK *)NULL ) /* pred of entry block */
		return;

	lp1 = bp->pred;
	
	LFOR( lp, lp1 )
	{
		bitno = LCAST(lp, BLOCK)->blockno;
		if( test_bit( set, bitno ) ) /* already visited */
			continue;

		set_bit( set, bitno );
		set_pred( set, (BLOCK*)lp->datap );
	}
}

/*
 * Merge webs corresponding to a single variable, forming the maximal
 * connected components of the variable's du chains.  Use the following
 * algorithm:
 *
 * Input:
 *	a list W of webs associated with a variable x; each
 *	    web is a tuple of the form ({D}, canreach(D)), where D
 *	    is a definition of x.  Weblist is actually a contiguous
 *	    sublist at the beginning of a larger list.
 *	a boolean array visited[0..nvarrefs-1], initially FALSE,
 *	    for each var_ref that has not been assigned to a web.
 *	a mapping web(i): definitions => webs
 *
 * Output:
 *	a partitioning of the defs and uses of x into maximal
 *	connected components, i.e., a modified set of tuples of
 *	the form (defs(x), uses(x)), where:
 *	    defs(x) is the union of reachdefs(U) for all U in uses(x);
 *	    uses(x) is the union of canreach(D) for all D in defs(x);
 * Method:
 *	Use the web list as a workpile to control iterations. Similar
 *	algorithms may be found in "A Survey of Data Flow Analysis
 *	Techniques", by K. Kennedy, in "Program Flow Analysis",
 *	Muchnick & Jones, pp. 42-46.
 *
 *	while (leaf(W) == x) do
 *	    changed = FALSE;
 *	    for each definition D in W do
 *		if (!visited(D)) then
 *		    visited(D) = TRUE;
 *		    for each use U in canreach(D) do
 *			if (!visited(U)) then
 *			    visited(U) = TRUE;
 *			    for each def D2 in reachdefs(U) do
 *				W2 = web(D2);
 *				if (W != W2 && W2 != NULL) then
 *				    merge_web(W, W2);
 *				    changed = TRUE;
 *				end if;
 *			    end for;
 *			end if;
 *		    end for;
 *		end if;
 *	    end for;
 *	    if (!changed) then
 *		(* done merging W *)
 *		W = next_web(W);
 *	    end if;
 *	end while;
 */
void
merge_same_var( leafp, visited )
    LEAF *leafp;		/* variable leaf node */
    SET visited;		/* identifies defs & uses already processed */
{
    WEB *w;			/* current web -- absorbs others */
    WEB *w2;			/* web to be merged */
    LIST *lp_d;			/* for each D in defs(W) */
    VAR_REF *vr_d;		/* var_ref in defs(W) */
    int d;			/* def # in defs(W) */
    LIST *lp_u, *canreach;	/* for each U in canreach(D) */
    VAR_REF *vr_u;		/* var_ref in canreach(D) */
    int u;			/* def # in canreach(D) */
    LIST *lp_d2, *reachdefs;	/* for each D2 in reachdefs(U) */
    int d2;			/* def # in reachdefs(U) */
    BOOLEAN changed;		/* for controlling iterations */

    /*
     * sanity check on web list
     */
    if (web_head == NULL) {
	quit("merge_same_var: null web list\n");
	/*NOTREACHED*/
    }
    /*
     * while (leaf(W) == x) do
     */
    w = web_head;
    while (w != NULL && w->leaf == leafp) {
	changed = IR_FALSE;
	/*
	 * for each definition D in W do
	 */
	LFOR(lp_d, w->define_at) {	/* note: w->define_at changes below! */
	    /*
	     * if (!visited(D)) then
	     */
	    vr_d = LCAST(lp_d, VAR_REF);
	    d = vr_d->refno;
	    if (!test_bit(visited, d)) {
		/*
		 * visited(D) = TRUE;
		 */
		set_bit(visited, d);
		/*
		 * for each use U in canreach(D) do
		 */
		canreach = vr_d->site.tp->canreach;
		LFOR(lp_u, canreach) {
		    /*
		     * if (!visited(U)) then
		     */
		    vr_u = LCAST(lp_u, VAR_REF);
		    u = vr_u->refno;
		    if (!test_bit(visited,u)) {
			/*
			 * visited(U) = TRUE;
			 */
			set_bit(visited, u);
			/*
			 * for each D2 in reachdefs(U) do
			 */
			if (vr_u->reftype == VAR_EXP_USE1
			  || vr_u->reftype == VAR_USE1) {
			    reachdefs = vr_u->site.tp->reachdef1;
			} else {
			    reachdefs = vr_u->site.tp->reachdef2;
			}
			LFOR(lp_d2, reachdefs) {
			    /*
			     * if (web(D2) != W) then
			     *     merge_web(w, w2);
			     * end if;
			     */
			    d2 = LCAST(lp_d2, VAR_REF)->defno;
			    if (d2 == NULLINDEX) {
				quit("bad defno in merge_same_var\n");
				/*NOTREACHED*/
			    }
			    w2 = defs_to_webs[d2];
			    if (w != w2 && w2 != NULL) {
				merge_webs(w, w2);
				changed = IR_TRUE;
			    }
			} /* for each D2 */
		    } /* if (!visited(U)) */
		} /* for each U */
	    } /* if (!visited(D)) */
	} /* for each D */
	if (!changed) {
	    w = w->next_web;
	}
    } /* while (leaf(W) == x) */
}

/*
 * merge_webs(web1, web2)
 *	Merge the define_at and use lists of web2 into those of
 *	web1.  Update the defs=>webs mapping to reflect the merge.
 *	Recycle web2 for future use.
 */
static void
merge_webs( web1, web2 )
	WEB *web1, *web2;
{
	LIST *lp, *w2defs;
	int d;

	w2defs = web2->define_at;
	LFOR(lp, w2defs) {
		d = LCAST(lp, VAR_REF)->defno;
		if (d == NULLINDEX || defs_to_webs[d] != web2) {
			quit("bad defs=>webs mapping in merge_webs\n");
			/*NOTREACHED*/
		}
		defs_to_webs[d] = web1;
	}
	LAPPEND( web1->define_at, web2->define_at );
	web1->use = merge_lists( web1->use, web2->use );
	if( web2->import )
		web1->import = IR_TRUE;
	delete_web( web2 );
}

void
dump_webs(show_conflict)
	BOOLEAN show_conflict;
{
	register LIST *lp;

	if( n_dsort ) {
		printf( "\nDUMP WEBS for register d: use %d D REGISTERS\n",
					MAX_DREG - n_dreg );
		for( lp =  sort_d_list->next; lp->datap != (LDATA *)NULL; lp = lp->next ) {
			print_web( show_conflict, lp, D_TREE );
		}
	}
	
#if TARGET == MC68000
	if( n_asort ) {
		printf( "\nDUMP WEBS for register a: use %d A REGISTERS\n",
					MAX_AREG - n_areg );
		for( lp = sort_a_list->next; lp->datap != (LDATA *)NULL; lp = lp->next ) {
			print_web( show_conflict, lp, A_TREE );
		}

		if( usefpa && fpa_base_reg != -1 ) {
			printf( "\n\t**** FPA_BASE_REG %s *** with %d LB",
					areg_name[fpa_base_reg], fpa_base_weight );
		}
	}
#endif
#if TARGET == I386
	if( n_asort ) {
		printf( "\nDUMP WEBS for register a: use %d A REGISTERS\n",
					MAX_AREG - n_areg );
		for( lp = sort_a_list->next; lp->datap != (LDATA *)NULL; lp = lp->next ) {
			print_web( show_conflict, lp, A_TREE );
		}
	}
#endif

/* there is no A register on Sparc */

	if( n_fsort ) {
		printf( "\nDUMP WEBS for register f: use %d F REGISTERS\n",
					max_freg - n_freg );
		for( lp = sort_f_list->next; lp->datap != (LDATA *)NULL; lp = lp->next ) {
			print_web( show_conflict, lp, F_TREE );
		}
	}
}

static void
print_web( show_conflict, lp, which_tree )
	BOOLEAN show_conflict;
	LIST *lp;
	TREE which_tree;
{
	WEB *web;
	LIST *lp2;
	int i;
	SORT *sp;

	sp = LCAST(lp, SORT);

	if( sp->weight == 0 )
	/* not a independent web anymore */
		return;

	printf( "\n" );
	printf( "\tMERGED_WEBS: ******weight %d ", sp->weight );
	if( sp->regno ) {
#if TARGET == MC68000
		if( which_tree == D_TREE ) {
			printf( ": IN REG %s***********\n", dreg_name[sp->regno] );
		} else {
			if( which_tree == A_TREE ) {
				printf( ": IN REG %s***********\n", areg_name[sp->regno] );
			} else {
				printf( ": IN REG %s***********\n",
					usefpa ?
					fpareg_name[sp->regno] : freg_name[sp->regno] );
			}
		}
#endif
#if TARGET == SPARC
		if( which_tree == D_TREE ) {
			printf( ": IN REG %s***********\n", dreg_name[sp->regno] );
		} else {
			printf( ": IN REG %s***********\n", freg_name[sp->regno] );
		}
#endif
#if TARGET == I386
		printf( ": IN REG %s***********\n", reg_name[sp->regno],
			(int)which_tree/*shut lint up*/ );
#endif
	} else {
		printf( ": NOT IN REGISTER******\n" );
	}
	
	for(web = LCAST(lp, SORT)->web; web; web = web->same_reg)
	{
		printf( "\t\tWEB[%d]: for leaf[%d] %s\n", web->web_no, 
						web->leaf->leafno,
				web->leaf->pass1_id ? web->leaf->pass1_id : "\"\"" );
		printf( "\t\t\tDEFINE: " );
		if( web->import )
			printf( "IMPORT" );
		LFOR( lp2, web->define_at )
		{
			printf( " D[%d]:(B[%d],T[%d])",
			    LCAST(lp2,VAR_REF)->defno,
			    LCAST(lp2,VAR_REF)->site.bp->blockno,
			    LCAST(lp2,VAR_REF)->site.tp->tripleno);
		}
		printf( "\n\t\t\tUSE:" );
		LFOR( lp2, web->use )
		{
			printf( " (B[%d],T[%d])",
					LCAST(lp2,VAR_REF)->site.bp->blockno,
					LCAST(lp2,VAR_REF)->site.tp->tripleno);
		}
		printf( "\n\t\t\tWEIGHT: " );

#if TARGET == MC68000 || TARGET == I386
		printf( "for A register %dLB, for D register %d LB, for F register %d LB",
					web->a_cont, web->d_cont, web->f_cont );
		if( web->sort_a )
			printf( "\n\t\t\tIN A LIST" );
		else
			printf( "\n\t\t\t**NOT** IN A LIST" );

#endif

#if TARGET == SPARC
		printf( "for D register %d LB, for F register %d LB",
					web->d_cont, web->f_cont );
#endif
		if( web->sort_d )
			printf( "\n\t\t\tIN D LIST" );
		else
			printf( "\n\t\t\t**NOT** IN D LIST" );

		if( web->sort_f )
			printf( "\n\t\t\tIN F LIST" );
		else
			printf( "\n\t\t\t**NOT** IN F LIST" );

		if (show_conflict)
		{
			printf( "\n\t\t\tINTERFERES WITH: " );
			for( i = 0; i < nwebs-1; i++ )
			{
				if( test_bit( web->interference, i ) )
				{
					printf( "WEB[%d], ", i );
					if (i && (i % 8) == 0)
						printf("\n");
				}
			}
		}
		printf( "\n\n" );
	}
	printf( "\n\t\**********************************\n" );
}

/*
 * The "weight" count of each uses of a variable is
 * the difference of the execution cycle (approximately) between
 * putting this variable in the register and putting it in memory
 * (assume always using base-displacement mode).
 */

void
weigh_web( web, vp ) WEB *web; VAR_REF *vp;
{
	register TRIPLE *tp;
	register BLOCK *bp;

	tp = vp->site.tp;
	bp = vp->site.bp;
	
#if TARGET == MC68000
	if( ( ISREAL( web->leaf->type.tword ) ) && ( use68881 || usefpa ) ) {
		web->f_cont = add_costs(web->f_cont, 
			mul_costs((COST)bp->loop_weight,
				(COST)op_descr[((int)tp->op)].f_weight));
		web->a_cont = web->d_cont = 0;
	} else {
		web->f_cont = 0;
		if( vp->reftype == VAR_USE2 || vp->reftype == VAR_EXP_USE2 ) { 
			web->d_cont = add_costs(web->d_cont,
			    mul_costs((COST)bp->loop_weight,
				(COST)op_descr[((int)tp->op)].right.d_weight));
			web->a_cont = add_costs(web->a_cont,
			    mul_costs((COST)bp->loop_weight,
				(COST)op_descr[((int)tp->op)].right.a_weight));
		} else { /* use on left handside or define. */
			web->d_cont = add_costs(web->d_cont,
			    mul_costs((COST)bp->loop_weight,
				(COST)op_descr[((int)tp->op)].left.d_weight));
			web->a_cont = add_costs(web->a_cont,
			    mul_costs((COST)bp->loop_weight,
				(COST)op_descr[((int)tp->op)].left.a_weight));
		}
		if( ISCHAR(web->leaf->type.tword)
		    || ISUNSIGNED(web->leaf->type.tword)) {
			/*
			 * never put char types or unsigned types in A-reg
			 */
			web->a_cont = 0;
		}
	}
#endif

#if TARGET == SPARC
	if( ( ISREAL( web->leaf->type.tword ) ) ) {
		COST operand_weight = (COST)op_descr[((int)tp->op)].f_weight;
		if (ISDOUBLE(web->leaf->type.tword))
			operand_weight = mul_costs(operand_weight, (COST)2);
		web->f_cont = add_costs(web->f_cont,
		    mul_costs((COST)bp->loop_weight, operand_weight));
		web->d_cont = 0;
	} else { /* always use left.d_weight - also see opdescr.h */
		 /* in coming argument register should count as +3 */
		if( tp->op == IR_ENTRYDEF ) {
			if( web->leaf->val.addr.seg == &seg_tab[ARG_SEGNO] &&
				web->leaf->val.addr.offset+web->leaf->type.size<=MAX_ARG_REG*4+ARGOFFSET ) {
				web->d_cont = add_costs(web->d_cont, (COST)3);
				web->f_cont = 0;
				return;
			}
		}
		web->d_cont = add_costs(web->d_cont,
		    mul_costs((COST)bp->loop_weight,
			(COST)op_descr[((int)tp->op)].left.d_weight));
		web->f_cont = 0;
	}
#endif

#if TARGET == I386
	if( ISREAL(web->leaf->type.tword) ) {
		web->f_cont = add_costs(web->f_cont, 
			mul_costs((COST)bp->loop_weight,
				(COST)op_descr[((int)tp->op)].f_weight));
		web->a_cont = web->d_cont = 0;
	} else {
		/* always use left.{d,a}_weight - also see opdescr.h */
		web->f_cont = 0;
		web->d_cont = add_costs(web->d_cont,
		    mul_costs((COST)bp->loop_weight,
			(COST)op_descr[((int)tp->op)].left.d_weight));
		web->a_cont = add_costs(web->a_cont,
		    mul_costs((COST)bp->loop_weight,
			(COST)op_descr[((int)tp->op)].left.a_weight));
		if( ISCHAR(web->leaf->type.tword) ) {
			/*
			 * never put char types in ESI,EDI
			 */
			web->a_cont = 0;
		}
	}
#endif
}

void 
sort_webs()
{ /* using the binary tree to sort the webs */
  /* base on the wight for A and D register */

	LIST *lp;
	WEB *web;
	SORT *sort_a, *sort_d, *sort_f, *sort_d_root, *sort_a_root, *sort_f_root;

	sort_d_root = sort_a_root = sort_f_root = (SORT *)NULL;

	for( web = web_head; web != NULL; web = web->next_web ) {
		if( web->d_cont > 0 ) {
			++n_dsort;
			web->sort_d = sort_d = (SORT *)ckalloc(1, sizeof(SORT));
			sort_d->weight = web->d_cont;
			sort_d->web = web;
			sort_d->left = sort_d->right = (SORT *)NULL;
			insert_tree( &sort_d_root, sort_d );
		} else {
			web->sort_d = (SORT *)NULL;
		}

		if( web->a_cont > 0 ) {
			++n_asort;
			web->sort_a = sort_a = (SORT *)ckalloc(1, sizeof(SORT));
			sort_a->weight = web->a_cont;
			sort_a->web = web;
			sort_a->left = sort_a->right = (SORT *)NULL;
			insert_tree( &sort_a_root, sort_a );
		} else {
			web->sort_a = (SORT *)NULL;
		}

		if( web->f_cont > 0 ) {
			++n_fsort;
			web->sort_f = sort_f = (SORT *)ckalloc(1, sizeof(SORT));
			sort_f->weight = web->f_cont;
			sort_f->web = web;
			sort_f->left = sort_f->right = (SORT *)NULL;
			insert_tree( &sort_f_root, sort_f );
		} else {
			web->sort_f = (SORT *)NULL;
		}
	}

	/* put the tree into list in the order of */
	/* the heaviest first */

	if( n_dsort )
	{
		listed( sort_d_root, &sort_d_list );
		/*
		 * At this point, sort_d_list points to the lightest
		 * web in the a list. Append a dummy list at the end
		 * to mark the end and let the sort_list points to it.
		 */
		lp = NEWLIST( reg_lq );
		lp->datap = (LDATA *)NULL;
		LAPPEND( sort_d_list, lp );
	}

	if( n_asort )
	{
		listed( sort_a_root, &sort_a_list );
		lp = NEWLIST( reg_lq );
		lp->datap = (LDATA *)NULL;
		LAPPEND( sort_a_list, lp );
	}

	if( n_fsort )
	{
		listed( sort_f_root, &sort_f_list );
		lp = NEWLIST( reg_lq );
		lp->datap = (LDATA *)NULL;
		LAPPEND( sort_f_list, lp );
	}
}

static void
insert_tree( head, sort_el ) 
	SORT **head, *sort_el;
{ /* put the sort_el into the tree pointed by head at right position */

	register SORT *cur_p, *parent_p;

	if( *head == (SORT *)NULL )
	{
		*head = sort_el;
		return;
	}

	for( cur_p= *head; cur_p; ) {
		parent_p = cur_p;
		if( cur_p->weight > sort_el->weight )
			cur_p = cur_p->left;
		else
			cur_p = cur_p->right;
	}
	/* found it */

	if( parent_p->weight > sort_el->weight )
		parent_p->left = sort_el;
	else
		parent_p->right = sort_el;
}

static void
listed( tree, sort_list ) SORT *tree; LIST **sort_list;
{
	register LIST *lp;

	if( tree == (SORT *)NULL ) {
		return;
	}
	
	listed( tree->right, sort_list );

	/* put the tree into the list*/
	lp = NEWLIST(reg_lq);
	lp->datap = (LDATA *) tree;
	tree->lp = lp;
	LAPPEND( *sort_list, lp );
	listed( tree->left, sort_list );
}

#define Dreg 0
#define Areg 1
#define Freg 2

static int
preference(web)
    WEB *web;
{
#   if TARGET == MC68000 || TARGET == I386
	if (web->a_cont > web->d_cont && web->a_cont > web->f_cont) {
	    return Areg;
	}
	if (web->f_cont > web->d_cont && web->f_cont > web->a_cont) {
	    return Freg;
	}
#   endif
#   if TARGET == SPARC
	if (web->f_cont > web->d_cont) {
	    return Freg;
	}
#   endif
    return Dreg;
}

static int
preference_set(web)
    WEB *web;
{
    return (1 << preference(web));
}

/*
 * Are there kinds of registers we'd really rather not
 * use for this?  Return the set of such kinds of registers.
 */
static int
exclude_set(web)
    WEB *web;
{
    int x = 0;
    if (web->sort_d == NULL)
	x |= (1<<Dreg);
    if (web->sort_f == NULL)
	x |= (1<<Freg);
#   if TARGET == MC68000
	if (web->sort_a == NULL)
	    x |= (1<<Areg);
#   endif
    return x;
}

/*
 * return TRUE if a web lives entirely within a single block.
 */
static BOOLEAN
short_life(web)
    WEB *web;
{
    BLOCK *bp;
    LIST *lp, *defs, *uses;

    defs = web->define_at;
    bp = LCAST(defs,VAR_REF)->site.bp;
    LFOR(lp, defs) {
	if (LCAST(lp, VAR_REF)->site.bp != bp)
	    return IR_FALSE;
    }
    uses = web->use;
    LFOR(lp, uses) {
	if (LCAST(lp, VAR_REF)->site.bp != bp)
	    return IR_FALSE;
    }
    return IR_TRUE;
}

static BOOLEAN
can_share(base_web, cur_web, pass)
    WEB *base_web, *cur_web;
    int pass;
{
    if( !test_bit( base_web->interference, cur_web->web_no ) ) {
	/*
	 * no interference between these webs; but do we
	 * want them to share the same register?
	 */
	switch(pass) {
	case 1:
	    /* give preference to web pairs with a common leaf */
	    return (BOOLEAN)(base_web->leaf == cur_web->leaf);
	case 2:
	    /* delay merging webs with different preferences */
	    if (preference(base_web) != preference(cur_web)) {
		return IR_FALSE;
	    }
	    /* delay merging short-lived webs with long-lived webs */
	    if (short_life(base_web) != short_life(cur_web)) {
		return IR_FALSE;
	    }
	    return IR_TRUE;
	case 3:
	    /* we'll take *almost* anything at this point */
	    if (preference_set(base_web) & exclude_set(cur_web)) {
		return IR_FALSE;
	    }
	    return IR_TRUE;
	}
    }
    return IR_FALSE;
}

/*
 * merge webs along a sorted list.  multiple passes
 * are used to implement priority-based merging.
 */
void
merge_sorts( lp, pass ) LIST *lp;  int pass;
{
	WEB *base_web;	/* first web in list; always merge with this one */
	WEB *cur_web;	/* current candidate for merging */
	LIST *tmp_lp;
	SORT *sp;
	SET base_intr;	/* interference set of base web */
	SET cur_intr;	/* interference set of web being merged with base */
	unsigned long new_intr;
	BOOLEAN can_continue;
	int i;

	base_web = LCAST(lp,SORT)->web;
	
	/* search for can share starting at next web */
	for( tmp_lp = lp->next; tmp_lp->datap; tmp_lp = tmp_lp->next)
	{
		if( (LCAST(tmp_lp, SORT))->weight == 0 )
			continue;

		cur_web = (LCAST(tmp_lp, SORT))->web;
		if( can_share( base_web, cur_web, pass ) )
		{ /* can share same register */
#if TARGET == MC68000 || TARGET == I386
			if( sp = base_web->sort_a ) {
			    sp->weight = add_costs(sp->weight, cur_web->a_cont);
			}
#endif
			if( sp = base_web->sort_d ) {
			    sp->weight = add_costs(sp->weight, cur_web->d_cont);
			}

			if( sp = base_web->sort_f ) {
			    sp->weight = add_costs(sp->weight, cur_web->f_cont);
			}
#if TARGET == MC68000 || TARGET == I386
			if( cur_web->sort_a ) /* mark vistied */
				cur_web->sort_a->weight = 0;
#endif
			if( cur_web->sort_d )
				cur_web->sort_d->weight = 0;
			if( cur_web->sort_f )
				cur_web->sort_f->weight = 0;

			cur_web->same_reg = base_web->same_reg;
			base_web->same_reg = cur_web;
			/*
			 * compute union of interference sets; if
			 * there still exists a web that does not
			 * interfere with the union, merging can
			 * continue.
			 */
			base_intr = base_web->interference;
			cur_intr = cur_web->interference;
			can_continue = IR_FALSE;
			for (i = reg_share_wordsize; i > 0; i--) {
				new_intr = (*base_intr++ |= *cur_intr++);
				if (~new_intr != 0) {
					can_continue = IR_TRUE;
				}
			}
			if (!can_continue) {
				break;
			}
		}
	}
}

void
reorder_sorts( lp, lpp ) LIST *lp, **lpp;
{ /* after merge, the weight will only be increase. */
  /* so the lp can only go up in the list but never go down */

	register LIST *search_lp, *search_lp_prev;
	register SORT *cur_sp, *search_sp;
	register COST cur_weight, search_weight;
	LIST *sort_list;

	cur_sp = LCAST(lp, SORT);
	if( lp == (*lpp)->next )
		return;

	sort_list = *lpp;
	cur_weight = cur_sp->weight;
	
	search_lp_prev = sort_list;
	
	for( search_lp = sort_list->next; search_lp != lp;
		search_lp_prev = search_lp, search_lp = search_lp->next )
	{
		search_sp = LCAST(search_lp, SORT);
		if( search_sp->weight == 0 ) {
			continue;
		}
		search_weight = search_sp->weight;
		if( search_weight < cur_weight )
		{ /* found the right spot for lp */
		/* take lp out of the sort list first. */
			delete_list( lpp, lp );
			lp->next = lp;
			LAPPEND( search_lp_prev, lp );
			return;
		}
	}
}

BLOCK *
find_df_block( web ) WEB *web;
{ /*
   * find the "best" block to do the initial loading
   * for this import web.  The "correct" blocks to do
   * the initial loading are the blocks that dominate
   * all the definition and usage points.  The "best"
   * block to do it is the "nearest" one amount all
   * the "correct" blocks and the loop_weight is 1.
   * Since, this web is IMPORT, so there is at least
   * the entry block will be the "best" block for it.
   * return NULL in MULTIPLE-ENTRY case.
   */

	LIST *lp;
	BLOCK *bp, *df_block;

	df_block = (BLOCK *)NULL;
	for( bp = entry_block; bp; bp = bp->next )
	{
		if( bp->loop_weight > 1 || bp->labelno <= 0 )
		{ /* dont do the initial loading inside a loop,
		     or in an unreachable block */
			continue;
		}

		LFOR( lp, web->define_at )
		{
			if( LCAST( lp, VAR_REF )->site.tp->op == IR_ENTRYDEF )
			/* import dummy definition */
				continue;

			if( !dominates(bp->blockno,LCAST(lp,VAR_REF)->site.bp->blockno) )
				goto next;
		}

		LFOR( lp, web->use )
		{
			if( !dominates(bp->blockno, LCAST(lp,VAR_REF)->site.bp->blockno) )
				goto next;
		}

		/* bp dominates all the occurences of the web */

		if( df_block == (BLOCK *)NULL )
			df_block = bp;

		else
		{ /* both bp and df_block are "correct" block */
		  /* see which one is better */
			if( dominates(df_block->blockno, bp->blockno ) )
			{ /* bp is better */
				df_block = bp;
			}
			/* else df_block is a better fit */
		}
	next: /* continue */;
	}
	return( df_block );
}

