#ifndef lint
static	char sccsid[] = "@(#)reg_alloc.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * This file contains the main routine of register allocation.
 */

#include "iropt.h"
#include "recordrefs.h"
#include "page.h"
#include "misc.h"
#include "var_df.h"
#include "bit_util.h"
#include "loop.h"
#include "cost.h"
#include "live.h"
#include "reg.h"
#include "reg_alloc.h"
#include <stdio.h>
#include <ctype.h>

/* global */
int reg_share_wordsize;
int n_dreg, n_freg, max_freg;

#if TARGET == MC68000 || TARGET == I386
int n_areg;
#endif

/* local */
static SET_PTR	interference;
static void	build_du_web();
static void	compute_interference();
static void	sequence_local_defs();
static void	weigh_du_web();
static void	modify_triples(/* reg_a, reg_d, reg_f */);
static void	replace_leaves(/* web, new_leaf, old_leaf */);
static void	share_registers();


void
do_register_alloc()
{
	LIST *lpd, *lpf, *reg_d, *reg_f, *cur_d, *cur_f;
	BOOLEAN more_d, more_f;
	int first_freg;
	COST d_weight, f_weight;
#	if TARGET == SPARC
	    int i;
	    int first_dreg;
	    LIST *tmp_lpd;
	    WEB *tmp_web;
	    LEAF *leafp;
#	endif
#	if TARGET == MC68000
	    LIST *lp, *lpa, *reg_a, *cur_a;
	    BOOLEAN more_a;
	    COST a_weight;
#	endif
#	if TARGET == I386
	    LIST *lp, *lpa, *reg_a, *cur_a;
	    BOOLEAN more_a;
	    COST a_weight;
	    int first_dreg;
	    int first_areg;
#	endif

	sequence_local_defs();
	reg_init();
	build_du_web();
	if( nwebs == 0 ) { /* no web worth to put into the register */
		reg_cleanup();
		return;
	}
	compute_interference();
	weigh_du_web();

	/* Do register allocation according to the information */
	/* gather from the above calls. sort_a_list should points */
	/* to the heaviest a web and so does sort_d_list and
	/* sort_f_list. */

	share_registers();
	if(debugflag[B_REGALLOC_DEBUG]) {
		dump_webs(IR_TRUE);
	}

#if TARGET == MC68000
	/* 
	 * sort_a_list, sort_d_list and sort_f_list is in order 
	 * of the weight -- heaviest is the first one 
	 * in the list and than the ->next
	 */

	n_dreg = MAX_DREG;
	n_areg = MAX_AREG;
	if( usefpa ) {
		max_freg = n_freg = MAX_FPAREG;
		first_freg = FIRST_FPAREG;
	} else {
		if( use68881 ) {
			max_freg = n_freg = MAX_FREG;
			first_freg = FIRST_FREG;
		} else {
			max_freg = n_freg = 0;
		}
	}
	lpa = sort_a_list ? sort_a_list->next : LNULL;
	lpd = sort_d_list ? sort_d_list->next : LNULL;
	lpf = sort_f_list ? sort_f_list->next : LNULL;
	
	more_a = ( lpa && lpa->datap != (LDATA *)NULL ) ?  IR_TRUE : IR_FALSE;
	more_d = ( lpd && lpd->datap != (LDATA *)NULL ) ?  IR_TRUE : IR_FALSE;
	more_f = ( lpf && lpf->datap != (LDATA *)NULL ) ?  IR_TRUE : IR_FALSE;

	cur_a = reg_a = (LIST *)ckalloc((unsigned)MAX_AREG, sizeof( LIST ) );
	cur_d = reg_d = (LIST *)ckalloc((unsigned)MAX_DREG, sizeof( LIST ) );
	cur_f = reg_f = (LIST *)ckalloc((unsigned)n_freg, sizeof( LIST ) );
	
	while( ( n_areg && more_a ) || ( n_dreg && more_d ) || (n_freg && more_f ) ) {
		/* more registers and more webs to allocate */
		
		a_weight = d_weight = f_weight = 0;
		
		while( n_areg && more_a && a_weight == 0 ) {
			a_weight = LCAST( lpa, SORT )->weight;
			if( a_weight == 0 ) { /* skip those webs that have been merged */
				lpa = lpa->next;
				if( lpa->datap == (LDATA *)NULL ) {
					more_a = IR_FALSE;
					lpa = LNULL;
				}
			}
		}
		if( n_areg && usefpa && fpa_base_weight > a_weight && 
			fpa_base_reg == -1 ) {
			a_weight = fpa_base_weight;
		} 

		while( n_dreg && more_d && d_weight == 0 ) {
			d_weight = LCAST( lpd, SORT )->weight;
			if( d_weight == 0 ) { /* skip those webs that have been merged */
				lpd = lpd->next;
				if( lpd->datap == (LDATA *)NULL ) {
					more_d = IR_FALSE;
					lpd = LNULL;
				}
			}
		}

		while( n_freg && more_f && f_weight == 0 ) {
			f_weight = LCAST( lpf, SORT )->weight;
			if( f_weight == 0 ) { /* skip those webs that have been merged */
				lpf = lpf->next;
				if( lpf->datap == (LDATA *)NULL ) {
					more_f = IR_FALSE;
					lpf = LNULL;
				}
			}
		}

		if( d_weight <= (COST)INIT_CONT && 
			a_weight <= (COST)INIT_CONT &&
			f_weight <= (COST)FINIT_CONT ) { /* DONE */
			break;
		}

		if( ( a_weight > f_weight ) && ( ( a_weight > d_weight ) ||
			( ( a_weight == d_weight ) &&
			lpa != NULL &&
			ISPTR( LCAST(lpa, SORT)->web->leaf->type.tword ) ) ) ) {
			/* allocate a A register for lpa or for fpa_base */
			if( usefpa && fpa_base_weight >= a_weight && fpa_base_reg == -1 ) {
				fpa_base_reg = FIRST_AREG - MAX_AREG + n_areg--;
				continue;
			}
			cur_a->datap = lpa->datap;
			LCAST(cur_a, SORT)->regno = FIRST_AREG - MAX_AREG + n_areg--;
			if( lpd && LCAST(lpd, SORT)->web == LCAST(lpa, SORT)->web ) {
				lpd = lpd->next;
				if( lpd->datap == (LDATA *) NULL ) {
					more_d = IR_FALSE;
					lpd = LNULL;
				}
			}
			if( lpf && LCAST(lpf, SORT)->web == LCAST(lpa, SORT)->web ) {
				lpf = lpf->next;
				if( lpf->datap == (LDATA *) NULL ) {
					more_f = IR_FALSE;
					lpf = LNULL;
				}
			}

			if( LCAST(lpa, SORT)->web->sort_d ) { /* in D list too */
				lp = LCAST(lpa, SORT)->web->sort_d->lp;
				delete_list( &sort_d_list, lp );
			}

			if( LCAST(lpa, SORT)->web->sort_f ) { /* in F list too */
				lp = LCAST(lpa, SORT)->web->sort_f->lp;
				delete_list( &sort_f_list, lp );
			}

			lpa = lpa->next;
			if( lpa == LNULL || lpa->datap == (LDATA *)NULL ) {
				more_a = IR_FALSE;
				lpa = LNULL;
			} else {
				cur_a++;
			}
			continue;
		}
		if( d_weight >= a_weight && d_weight > f_weight ) {
			/* allocate a D register for lpd */
			cur_d->datap = lpd->datap;
			LCAST(cur_d, SORT)->regno = FIRST_DREG - MAX_DREG + n_dreg--;
			if( lpa && LCAST(lpa, SORT)->web == LCAST(lpd, SORT)->web ) {
				/* lpa will be deleted, so advance it first */
				lpa = lpa->next;
				if( lpa->datap == (LDATA *)NULL ) {
					more_a = IR_FALSE;
					lpa = LNULL;
				}
			}
			if( lpf && LCAST(lpf, SORT)->web == LCAST(lpd, SORT)->web ) {
				lpf = lpf->next;
				if( lpf->datap == (LDATA *) NULL ) {
					more_f = IR_FALSE;
					lpf = LNULL;
				}
			}
			if( LCAST(lpd, SORT)->web->sort_a ) { /* in A list too */
				lp = LCAST(lpd, SORT)->web->sort_a->lp;
				delete_list( &sort_a_list, lp );
			}
			if( LCAST(lpd, SORT)->web->sort_f ) { /* in F list too */
				lp = LCAST(lpd, SORT)->web->sort_f->lp;
				delete_list( &sort_f_list, lp );
			}
			
			lpd = lpd->next;
			if( lpd->datap == (LDATA *) NULL ) {
				more_d = IR_FALSE;
				lpd = LNULL;
			} else {
				cur_d++;
			}
			continue;
		}

		if( f_weight >= a_weight && f_weight >= d_weight ) {
			/* allocate a F register for lpf */
			cur_f->datap = lpf->datap;
			if( use68881 ) {
				LCAST(cur_f, SORT)->regno = first_freg - max_freg + n_freg--;
			} else if( usefpa ) { /* start at fpa4 ends with fpa15 */
				LCAST(cur_f, SORT)->regno = first_freg + max_freg - n_freg--;
				/*
				 * bookkeeping for additional FPA accesses
				 * implied by save/restore code:
				 *	fpmoved	fpn,a6@(offset) (on entry)
				 *	...
				 *	fpmoved	a6@(offset),fpn (on exit)
				 * If based addressing is used, we save
				 * (FPA_BASE_WEIGHT*4) cycles
				 */
				fpa_base_weight = add_costs(fpa_base_weight,
					(COST)(FPA_BASE_WEIGHT * 4));
			}
			
			if( lpa && LCAST(lpa, SORT)->web == LCAST(lpf, SORT)->web ) {
				/* lpa will be deleted, so advance it first */
				lpa = lpa->next;
				if( lpa->datap == (LDATA *)NULL ) {
					more_a = IR_FALSE;
					lpa = LNULL;
				}
			}
			if( lpd && LCAST(lpd, SORT)->web == LCAST(lpf, SORT)->web ) {
				lpd = lpd->next;
				if( lpd->datap == (LDATA *) NULL ) {
					more_d = IR_FALSE;
					lpd = LNULL;
				}
			}

			if( LCAST(lpf, SORT)->web->sort_a ) { /* in A list too */
				lp = LCAST(lpf, SORT)->web->sort_a->lp;
				delete_list( &sort_a_list, lp );
			}
			if( LCAST(lpf, SORT)->web->sort_d ) { /* in F list too */
				/* take it out of the D list */
				lp = LCAST(lpf, SORT)->web->sort_d->lp;
				delete_list( &sort_d_list, lp );
			}
			
			lpf = lpf->next;
			if( lpf->datap == (LDATA *) NULL ) {
				more_f = IR_FALSE;
				lpf = LNULL;
			} else {
				cur_f++;
			}
			continue;
		}
	}

	/*
	 * set up regmask
	 */
	if( usefpa ) {
		if( fpa_base_weight >= (COST)INIT_CONT && n_areg > 0
		    && fpa_base_reg == -1 ) {
			fpa_base_reg = FIRST_AREG - MAX_AREG + n_areg--;
		}
		if( fpa_base_reg == -1 ) {
			regmask = SET_REGMASK( n_freg, 0, 0, n_areg, n_dreg );
		} else {
			regmask = SET_REGMASK( n_freg, fpa_base_reg, 0, n_areg, n_dreg );
		}
	} else {
		regmask = SET_REGMASK( 0, 0, n_freg, n_areg, n_dreg );
	}
#endif

#if TARGET == SPARC
	/* 
	 * sort_d_list and sort_f_list is in order 
	 * of the weight -- heaviest is the first one 
	 * in the list and than the ->next
	 * Assume there always a FPU exist on SPARC
	 * so no integer will in float-reg and no float
	 * variable in integer register.
	 */

	n_dreg = MAX_DREG;
	max_freg = n_freg = MAX_FREG;
	first_freg = FIRST_FREG;
	regmask &= ~SCRATCH_REGMASK;

	lpd = sort_d_list ? sort_d_list->next : LNULL;
	lpf = sort_f_list ? sort_f_list->next : LNULL;
	
	more_d = ( lpd && lpd->datap != (LDATA *)NULL ) ?  IR_TRUE : IR_FALSE;
	more_f = ( lpf && lpf->datap != (LDATA *)NULL ) ?  IR_TRUE : IR_FALSE;

	cur_d = reg_d = (LIST *)ckalloc((unsigned)MAX_DREG, sizeof( LIST ) );
	cur_f = reg_f = (LIST *)ckalloc((unsigned)MAX_FREG, sizeof( LIST ) );

	/* Allocate the register ARGUMENTs in IN first */
	/* Arguments will be on the top of the WEB list in all SORTs */

	for(i=0,tmp_lpd=lpd;
			i < MAX_DREG && tmp_lpd && tmp_lpd->datap;
					i++, tmp_lpd=tmp_lpd->next ) {
		/* Look for arguments */
		if( LCAST(tmp_lpd, SORT)->weight == 0 ) {
			continue;
		}
		for( tmp_web = LCAST(tmp_lpd, SORT)->web;
					tmp_web; tmp_web = tmp_web->same_reg ) {
			if( tmp_web->import==IR_FALSE ) {
				continue;
			}
			/* ELSE, import web */
			leafp = tmp_web->leaf;
			if( !(ISREAL(leafp->type.tword) ) &&
					leafp->val.addr.seg == &seg_tab[ARG_SEGNO] &&
					leafp->val.addr.offset >= 0 ) {
				if( leafp->val.addr.offset+leafp->type.size<=MAX_ARG_REG*4 + ARGOFFSET ) {
					(LCAST(tmp_lpd, SORT))->regno = FIRST_ARG_REG +
								(leafp->val.addr.offset-ARGOFFSET) / 4;
					leafp->must_store_arg = IR_FALSE;
					cur_d->datap = tmp_lpd->datap;
					cur_d++;
					--n_dreg;
					regmask &= ~(1 << ((LCAST(tmp_lpd, SORT))->regno));
				}
			}
		}
	}

	first_dreg = TOTAL_DREG;
	while( ! (regmask & (1 << (first_dreg)))) {
		--first_dreg;
	}

	while( n_dreg && more_d ) { /* more registers and more webs to allocate */
		while( more_d && ((LCAST(lpd, SORT))->regno != 0 /* ARG_REG */ ||
				(d_weight = LCAST( lpd, SORT) ->weight) == 0 ) ) {
			/* skip those webs that have been merged */
			lpd = lpd->next;
			if( lpd->datap == (LDATA *)NULL ) {
				more_d = IR_FALSE;
				lpd = LNULL;
			}
		}
		if( more_d && d_weight > INIT_CONT ) {
			/* allocate a D register for webs that in lpd */
			(LCAST(lpd, SORT))->regno = first_dreg;
			cur_d->datap = lpd->datap;
			cur_d++;
			lpd = lpd->next;
			if( lpd->datap == (LDATA *)NULL ) {
				more_d = IR_FALSE;
				lpd = LNULL;
			}
			if( n_dreg > 0 ) { /* there are more D registers */
				--n_dreg;
				regmask &= ~(1 << first_dreg );
				if (regmask) {
					while(!(regmask & (1<<(--first_dreg))));
				}
			}
		} else { /* no more web worth to allocate to D register */
			more_d = IR_FALSE;
		}
	}

	while( n_freg && more_f ) { /* more registers and more webs to allocate */
		while( more_f && (f_weight = LCAST( lpf, SORT) ->weight) == 0 ) {
			/* skip those webs that have been merged */
			lpf = lpf->next;
			if( lpf->datap == (LDATA *)NULL ) {
				more_f = IR_FALSE;
				lpf = LNULL;
			}
		}
		if( more_f && f_weight > save_f_reg ) {
			WEB *web;
			BOOLEAN need_pair = IR_FALSE;
			for (web = LCAST(lpf,SORT)->web; web != NULL;
			    web = web->same_reg) {
				if (web->leaf->type.size == SZDOUBLE/SZCHAR) {
					need_pair = IR_TRUE;
					break;
				}
			}
			if (need_pair == IR_TRUE) {
				/*
				 * allocate the second of a pair of F
				 * registers.
				 */
				fregmask &= ~(1 << first_freg );
				first_freg--;
				n_freg--;
				if (n_freg == 0) {
					/* last one, sorry */
					break;
				}
				if (first_freg&1) {
					/*
					 * first reg of pair must be even;
					 * waste one
					 */
					fregmask &= ~(1 << first_freg );
					first_freg--;
					n_freg--;
				}
			}
			LCAST(lpf, SORT)->regno = first_freg;
			fregmask &= ~(1 << first_freg );
			first_freg--;
			n_freg--;
			cur_f->datap = lpf->datap;
			cur_f++;
			lpf = lpf->next;
			if( lpf->datap == (LDATA *)NULL ) {
				more_f = IR_FALSE;
				lpf = LNULL;
			}
		} else { /* no more web worth to allocate for F register */
			more_f = IR_FALSE;
		}
	}

	/*
	 * add code generator's regs back to the available set 
	 */
	regmask |= SCRATCH_REGMASK;
#endif

#if TARGET == I386
	/* 
	 * sort_d_list, sort_a_list, and sort_f_list are
	 * sorted in order of decreasing weight.
	 * Assume there is always a 387,
	 * so no integer will be in a float-reg and
	 * no float variable in an integer register.
	 */

	n_dreg = MAX_DREG;		/* # of d regs remaining for iropt */
	first_dreg = FIRST_DREG;	/* the next available d reg */
	n_areg = MAX_AREG;		/* # of a regs remaining for iropt */
	first_areg = FIRST_AREG;	/* the next available a reg */
	max_freg = n_freg = MAX_FREG;
	first_freg = FIRST_FREG;	/* the next available f reg */
	regmask &= ~SCRATCH_REGMASK;	/* make cg regs unavailable */

	lpd = sort_d_list ? sort_d_list->next : LNULL;
	lpa = sort_a_list ? sort_a_list->next : LNULL;
	lpf = sort_f_list ? sort_f_list->next : LNULL;
	
	more_d = ( lpd && lpd->datap != (LDATA *)NULL ) ?  IR_TRUE : IR_FALSE;
	more_a = ( lpa && lpa->datap != (LDATA *)NULL ) ?  IR_TRUE : IR_FALSE;
	more_f = ( lpf && lpf->datap != (LDATA *)NULL ) ?  IR_TRUE : IR_FALSE;

	cur_d = reg_d = (LIST *)ckalloc(MAX_DREG, sizeof( LIST ) );
	cur_a = reg_a = (LIST *)ckalloc(MAX_AREG, sizeof( LIST ) );
	cur_f = reg_f = (LIST *)ckalloc(MAX_FREG, sizeof( LIST ) );
	
	while( ( n_areg && more_a ) || ( n_dreg && more_d ) ) {
		/* more registers and more webs to allocate */
		a_weight = d_weight = 0;
		while( n_areg && more_a && a_weight == 0 ) {
			a_weight = LCAST( lpa, SORT )->weight;
			if( a_weight == 0 ) { /* skip those webs that have been merged */
				lpa = lpa->next;
				if( lpa->datap == (LDATA *)NULL ) {
					more_a = IR_FALSE;
					lpa = LNULL;
				}
			}
		}
		while( n_dreg && more_d && d_weight == 0 ) {
			d_weight = LCAST( lpd, SORT )->weight;
			if( d_weight == 0 ) { /* skip those webs that have been merged */
				lpd = lpd->next;
				if( lpd->datap == (LDATA *)NULL ) {
					more_d = IR_FALSE;
					lpd = LNULL;
				}
			}
		}
		if( d_weight <= (COST)INIT_CONT && 
			a_weight <= (COST)INIT_CONT) { /* DONE */
			break;
		}
		if ( a_weight > d_weight 
		    || ( a_weight == d_weight
			&& ISPTR(LCAST(lpa, SORT)->web->leaf->type.tword) ) ) {
			cur_a->datap = lpa->datap;
			LCAST(cur_a, SORT)->regno = first_areg;
			cur_a++;
			if( lpd && LCAST(lpd, SORT)->web == LCAST(lpa, SORT)->web ) {
				lpd = lpd->next;
				if( lpd->datap == (LDATA *) NULL ) {
					more_d = IR_FALSE;
					lpd = LNULL;
				}
			}
			if( LCAST(lpa, SORT)->web->sort_d ) { /* in D list too */
				lp = LCAST(lpa, SORT)->web->sort_d->lp;
				delete_list( &sort_d_list, lp );
			}
			lpa = lpa->next;
			if( lpa->datap == (LDATA *)NULL ) {
				more_a = IR_FALSE;
				lpa = LNULL;
			}
			--n_areg;
			regmask &= ~(1 << first_areg);
			++first_areg;
			continue;
		}
		if( d_weight >= a_weight ) {
			/* allocate a D register for lpd */
			cur_d->datap = lpd->datap;
			LCAST(cur_d, SORT)->regno = first_dreg;
			cur_d++;
			if( lpa && LCAST(lpa, SORT)->web == LCAST(lpd, SORT)->web ) {
				/* lpa will be deleted, so advance it first */
				lpa = lpa->next;
				if( lpa->datap == (LDATA *)NULL ) {
					more_a = IR_FALSE;
					lpa = LNULL;
				}
			}
			if( LCAST(lpd, SORT)->web->sort_a ) { /* in A list too */
				lp = LCAST(lpd, SORT)->web->sort_a->lp;
				delete_list( &sort_a_list, lp );
			}
			lpd = lpd->next;
			if( lpd->datap == (LDATA *) NULL ) {
				more_d = IR_FALSE;
				lpd = LNULL;
			}
			--n_dreg;
			regmask &= ~(1 << first_dreg );
			++first_dreg;   /* count up to get more d regs*/
			continue;
		}
	}
	while( n_freg && more_f ) { /* more registers and more webs to allocate */
		while( more_f
		    && (f_weight = LCAST(lpf, SORT)->weight) == (COST)0 ) {
			/* skip those webs that have been merged */
			lpf = lpf->next;
			if( lpf->datap == (LDATA *)NULL ) {
				more_f = IR_FALSE;
				lpf = LNULL;
			}
		}
		if( more_f && f_weight > (COST)FINIT_CONT) {
			/* allocate an F register for webs in lpf */
			(LCAST(lpf, SORT))->regno = first_freg;
			cur_f->datap = lpf->datap;
			cur_f++;
			lpf = lpf->next;
			if( lpf->datap == (LDATA *)NULL ) {
				more_f = IR_FALSE;
				lpf = LNULL;
			}
			--n_freg;
			regmask &= ~(1 << first_freg );
			--first_freg;	    /* count down to get more fregs */
		} else { /* no more web worth to allocate for F register */
			more_f = IR_FALSE;
		}
	}
	/*
	 * add code generator's regs back to the available set 
	 */
	regmask |= SCRATCH_REGMASK;
#endif

	/* the dominator information is not correct */
	/* for those new created pre_header blocks */
	find_dominators();

#if TARGET==MC68000 || TARGET == I386
	modify_triples( reg_a, reg_d, reg_f );	
#endif
#if TARGET==SPARC
	modify_triples( LNULL, reg_d, reg_f );	
#endif
	if(debugflag[A_REGALLOC_DEBUG]) {
		dump_webs(IR_FALSE);
	}
	reg_cleanup();
}

static void 
build_du_web()
{ /*
   * Algorithm:
   *	Scan throught all the leaves, first, find out all the variables
   *	that can be a register variable.  For each interested variable,
   *	for each defination build a du-web and then merege those webs
   *	that have to be allocated in the same register or memory location.
   */

	LIST *lp1, **lpp;
	LEAF *leafp;
	SET_PTR var_refs_visited;
	VAR_REF *var_ref;
	WEB *web;
	struct segdescr_st descr;

	var_refs_visited = new_heap_set(1, nvarrefs);
	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) {
		/* for each hash entry */
		LFOR( lp1, *lpp ) { /* for each leaf */
			leafp = (LEAF *) lp1->datap;
			/* initially assume not an interesting variable */
			if( ISCONST(leafp) || leafp->no_reg || leafp->overlap )
				continue;

			descr = leafp->val.addr.seg->descr;
			if( descr.class == HEAP_SEG ) /* dummy leaf */
				continue;

			if( !can_in_reg( leafp ) )
				continue;

			if( leafp->references == (VAR_REF*) NULL )
			/* never used */
				continue;

			web = web_head;
			for(var_ref = leafp->references; var_ref;
                              var_ref = var_ref->next_vref) {
				/* make a web for each definition */
				if( var_ref->reftype == VAR_AVAIL_DEF ||
						var_ref->reftype == VAR_DEF )
					new_web( leafp, var_ref );
			}
			if(web != web_head) {
				merge_same_var( leafp, var_refs_visited->bits );
			}
		}
	}
	nwebs = 0;
	for(web = web_head; web != NULL; web = web->next_web) {
		web->web_no = nwebs++;
	}
	free_heap_set(var_refs_visited);
}

/*
 * compute_interference()
 *	Build the interference graph for all webs.
 *
 * Algorithm:
 *	compute_live_after_blocks(live_after_blocks);
 *	for each web W1, do
 *	    for each definition D1 in W1, do
 *		live_set := { definitions live after D1 };
 *		for each definition D2 in live_set,
 *		    if D2 is a member of a web W2 then
 *			add_interference(W1, W2);
 *		    end if;
 *		end for;
 *	    end for;
 *	end for;
 */
static void
compute_interference()
{
    register WEB *web1, *web2;
    SET_PTR live_after_blocks;	/* global live definitions matrix */
    SET_PTR live_after_site;	/* local live definitions set */
    SET bv_live_after_site;	/* bit vector of local live defs set */
    LIST *lpd1, *definitions1;  /* for iterating through definitions */
    int i,j;			/* for iterating through live defs set */
    int defno;			/* def# corresponding to a BPW boundary */
    int set_size;		/* size of local live defs set, in words */

    /*
     * compute the global live definitions sets
     */
    live_after_blocks = new_heap_set(nblocks, var_avail_defno);
    live_after_site = new_heap_set(1, nvardefs);
    set_size = SET_SIZE(live_after_site);
    bv_live_after_site = ROW_ADDR(live_after_site, 0);
    /*
     * compute live definitions
     */
    compute_live_after_blocks(save_reachdefs, live_after_blocks);
    /*
     * allocate the interference matrix, and assign each web
     * a pointer to a row in the matrix
     */
    free_heap_set(save_reachdefs);
    interference = new_heap_set(nwebs, nwebs);
    reg_share_wordsize = roundup((unsigned)nwebs, BPW) / BPW;
    for(web1 = web_head; web1 != NULL; web1 = web1->next_web) {
	web1->interference = ROW_ADDR(interference, web1->web_no);
    }
    /*
     * for each web W1, do
     */
    for(web1 = web_head; web1 != NULL; web1 = web1->next_web) {
	/*
	 * for each definition D1 in W1, do
	 */
	definitions1 = web1->define_at;
	LFOR(lpd1, definitions1) {
	    /*
	     * compute live definitions after D1
	     */
	    compute_live_after_site(live_after_blocks, live_after_site,
		LCAST(lpd1, VAR_REF)->site);
	    /*
	     * for each definition D2 in live(D1),
	     */
	    for (i = 0; i < set_size; i++) {
		if (bv_live_after_site[i] != 0) {
		    /*
		     * assuming live sets are sparse, this inner loop
		     * should be executed infrequently; the hope is to
		     * get a low constant of proportionality for an
		     * O(nvardefs**2) algorithm
		     */
		    defno = i*BPW;
		    for (j = 0; j < BPW; j++) {
			if (test_bit(bv_live_after_site, defno+j)) {
			    /*
			     * if live def belongs to a web, add an
			     * edge to interference graph
			     */
			    web2 = defs_to_webs[defno+j];
			    if (web2 != NULL && web2 != web1) {
				set_bit( web1->interference, web2->web_no );
				set_bit( web2->interference, web1->web_no );
			    }
			    /*
			     * clear bit, in the hopes of getting on to
			     * the next word in the set
			     */
			    reset_bit(bv_live_after_site, defno+j);
			    if (bv_live_after_site[i] == 0) {
				break;
			    } /* if */
			} /* if */
		    } /* for */
		} /* if */
	    } /* for */
	} /* for */
    } /* for */
    if (SHOWDENSITY) {
	printf("	density(interference) = %g\n",
	    density(interference));
    }
    free_heap_set(live_after_blocks);
    free_heap_set(live_after_site);
} /* compute_interference */

/*
 * assign sequence numbers to local definitions
 * starting where the globally available definitions
 * leave off.  This is required by compute_interference().
 */
static void
sequence_local_defs()
{
    VAR_REF *vp;	/* for iterating through all var_refs */
    int defno;		/* for range-checking def#'a */

    defno = navailvardefs;
    for (vp = var_ref_head; vp != NULL; vp = vp->next) {
	if (vp->reftype == VAR_DEF) {
	    /* local var definition */
	    if (vp->defno != NULLINDEX) {
		quita("sequence_local_defs: defno [%d] already assigned",
		    vp->defno);
	    }
	    vp->defno = defno++;
	}
    }
    if (defno != nvardefs) {
	quita("sequence_local_defs: missed var_defs (defno=%d, nvardefs=%d)",
	    defno, nvardefs);
    }
}

static void
weigh_du_web()
{ /* weight the webs and sort into two list*/
  /*  -- one for A one for D registers */

	WEB *web;
	LIST *lp;

	for( web = web_head; web != NULL; web = web->next_web ) {
		LFOR( lp, web->define_at ) {
			weigh_web( web, LCAST(lp, VAR_REF) );
		}

		LFOR( lp, web->use ) {
			weigh_web( web, LCAST(lp, VAR_REF) );
		}
	}
	sort_webs(); /* sort into a binary tree than put into a list */
}

/*ARGSUSED*/
static void
modify_triples( reg_a, reg_d, reg_f ) LIST *reg_a, *reg_d, *reg_f;
{
	LIST *lp;
	WEB *web;
	LEAF *new_leaf, *old_leaf;
	int i;

	for( i = 0, lp = reg_d; i < MAX_DREG && lp->datap; i++, lp++ ) {
		/* for all webs that have been allocated in D register. */
		for( web = LCAST( lp, SORT )->web; web; web = web->same_reg ) {
			/* for all the web in that D register */
			if( web->d_cont <= 0 ) {
				continue;
			}
			old_leaf = web->leaf;
			new_leaf = reg_leaf( old_leaf, DREG_SEGNO,
					LCAST( lp, SORT )->regno  );
			replace_leaves( web, new_leaf, old_leaf );
		}
	}

#if TARGET == MC68000 || TARGET == I386
	for( i = 0, lp = reg_a; i < MAX_AREG && lp->datap; i++, lp++ ) {
		/* for all webs that be allocated in A register. */
		for( web = LCAST( lp, SORT )->web; web; web = web->same_reg ) {
			/* for all the web in that A register */
			if( web->a_cont <= 0 ) {
				continue;
			}
			old_leaf = web->leaf;
			new_leaf = reg_leaf( old_leaf, AREG_SEGNO,
						LCAST( lp, SORT)->regno );
			replace_leaves( web, new_leaf, old_leaf );
		}
	}
#endif
	for( i = 0, lp = reg_f; i < max_freg && lp->datap; i++, lp++ ) {
		/* for all webs that have been allocated in F register. */
		for( web = LCAST( lp, SORT )->web; web; web = web->same_reg ) {
			/* for all the web in that F register */
			old_leaf = web->leaf;
			new_leaf = reg_leaf( old_leaf, FREG_SEGNO,
					LCAST( lp, SORT )->regno );
			replace_leaves( web, new_leaf, old_leaf );
		}
	}
}

static void
replace_leaves( web, new_leaf, old_leaf )
	WEB *web;
	LEAF *new_leaf, *old_leaf;
{
	LIST *lp;
	TRIPLE *tp, *triple;
	BLOCK *bp, *best_bp;
	LEAF *tmp_leaf;
	VAR_REF *var_def;
	VAR_REF *var_use;
	
	LFOR( lp, web->define_at ) {
		var_def = LCAST( lp, VAR_REF );
		tp = var_def->site.tp;
		bp = var_def->site.bp;

		if( tp->op == IR_ENTRYDEF) { /* do pre-loading */
#if TARGET == SPARC
			if( old_leaf->val.addr.seg == &seg_tab[ARG_SEGNO] &&
					old_leaf->must_store_arg == IR_FALSE ) {
				/* in the in-comming IN register already. */
				continue;
			}
#endif
			best_bp = find_df_block( web ); /* find best block to load */
			if( best_bp != bp && best_bp != (BLOCK *)NULL ) {
			  /* other place than the IR_ENTRYDEF */
			  /* do loading after first triple */
				tp = best_bp->last_triple->tnext;
			}
			triple = append_triple( tp,
				IR_ASSIGN, (IR_NODE*)new_leaf,
				(IR_NODE*)old_leaf, old_leaf->type );
			if( bp->last_triple == tp )
				bp->last_triple = triple;
			continue;
		}

		/* else */
		if( tp->op == IR_IMPLICITDEF ) {
			if( ( (TRIPLE *)tp->right)->op == IR_FCALL ) {
				for( tp=(TRIPLE *)tp->right; ;tp=tp->tnext ) {
					/* find the next ROOT triple */
					if( (ISOP(tp->op,ROOT_OP) ) )
						break;
				}
				if( ISOP(tp->op,BRANCH_OP) ) {
					/* assign the expression to a tmp */
					tmp_leaf = new_temp( ((TRIPLE *)tp->left)->type );
					triple = append_triple( tp->tprev,
						IR_ASSIGN, (IR_NODE*)tmp_leaf,
						(IR_NODE*)tp->left, tmp_leaf->type );
					tp->left = (IR_NODE *)tmp_leaf;
					tp = triple;
				}
					
			}

			triple = append_triple( tp, IR_ASSIGN,
				(IR_NODE*)new_leaf, (IR_NODE*)old_leaf,
				old_leaf->type );
			if( bp->last_triple == tp )
				bp->last_triple = triple;
			continue;
		}
		/*
		 * if the op isn't an implicit def, the var
		 * def is explicit; replace the old leaf
		 * with the new register leaf
		 */
		switch(var_def->reftype) {
		case VAR_DEF:
		case VAR_AVAIL_DEF:
			tp->left = (IR_NODE *)new_leaf;
			break;
		default:
			quita("replace_leaves: unexpected var ref\n");
			/*NOTREACHED*/
		}
	}

	LFOR( lp, web->use ) {
		var_use = LCAST(lp, VAR_REF);
		tp = var_use->site.tp;
		bp = var_use->site.bp;

		if( tp->op == IR_IMPLICITUSE || tp->op == IR_EXITUSE ) {
			/* store it back into memory */
			triple = append_triple( tp, IR_ASSIGN,
				(IR_NODE*)old_leaf, (IR_NODE*)new_leaf,
				old_leaf->type );
			if( bp->last_triple == tp )
				bp->last_triple = triple;
			continue;
		}
		/*
		 * if the op isn't an implicit use, the
		 * use is explicit; so replace the old leaf
		 * with the new register leaf.
		 */
		switch(var_use->reftype) {
		case VAR_USE1:
		case VAR_EXP_USE1:
			tp->left = (IR_NODE *)new_leaf;
			break;
		case VAR_USE2:
		case VAR_EXP_USE2:
			tp->right = (IR_NODE *)new_leaf;
			break;
		default:
			quita("replace_leaves: unexpected var ref\n");
			/*NOTREACHED*/
		}
	}
}

static void
share_registers()
{
/*
 * merge those webs that can share the same registers.
 */

	LIST *lpa, *lpd, *lpf, *lp_next;
	SORT *sp;
	COST a_weight, d_weight, f_weight;
	
	lpa = sort_a_list ? sort_a_list->next : LNULL;
	lpd = sort_d_list ? sort_d_list->next : LNULL;
	lpf = sort_f_list ? sort_f_list->next : LNULL;

	while( (lpa && lpa->datap) || (lpd && lpd->datap ) || (lpf && lpf->datap) ) {
		f_weight = a_weight = d_weight = 0;

#if TARGET == MC68000 || TARGET == I386
		while( lpa && lpa->datap && a_weight == 0 ) { /* more webs in reg A list */
			sp = LCAST(lpa, SORT);
			if( sp->weight == 0 ) { /* visited */
				lpa = lpa->next;
				continue;
			}
			a_weight = sp->weight;
		}
#endif
		while( lpd && lpd->datap && d_weight == 0 ) { /*more webs in D register list */
			sp = LCAST(lpd, SORT);
			if( sp->weight == 0 ) { /* already visited */
				lpd = lpd->next;
				continue;
			}
			d_weight = sp->weight;
		}
		
		while( lpf && lpf->datap && f_weight == 0 ) { /*more webs in F register list */
			sp = LCAST(lpf, SORT);
			if( sp->weight == 0 ) { /* already visited */
				lpf = lpf->next;
				continue;
			}
			f_weight = sp->weight;
		}
		
		if( a_weight <= 0 && d_weight <= 0 && f_weight <= 0 ) {/* done */
			break;
		}

#if TARGET == MC68000 || TARGET == I386	
		if( a_weight > d_weight && a_weight > f_weight ) {
			/* merge A list until no more sharing can be done */
			merge_sorts( lpa, 1 );
			merge_sorts( lpa, 2 );
			merge_sorts( lpa, 3 );
			/* fix the order list. */
			lp_next = lpa->next;
			reorder_sorts( lpa, &sort_a_list );
			lpa = lp_next;
		}
		else
#endif
		{
			if( d_weight > f_weight && d_weight >= a_weight ) {
				/* merge D list until no more sharing can be done */
				merge_sorts( lpd, 1 );
				merge_sorts( lpd, 2 );
				merge_sorts( lpd, 3 );
				lp_next = lpd->next;
				reorder_sorts( lpd, &sort_d_list );
				lpd = lp_next;
			}
			else { /* merge F list */
				merge_sorts( lpf, 1 );
				merge_sorts( lpf, 2 );
				merge_sorts( lpf, 3 );
				lp_next = lpf->next;
				reorder_sorts( lpf, &sort_f_list);
				lpf = lp_next;
			}
		}
	}
	free_heap_set(interference);
}

