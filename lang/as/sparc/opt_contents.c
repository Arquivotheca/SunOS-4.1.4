/*      @(#)opt_contents.c  1.1     94/10/31     Copyr 1986 Sun Microsystems */

#include <stdio.h>
#include "structs.h"
#include "registers.h"
#include "bitmacros.h"
#include "node.h"
#include "optimize.h"
#include "sym.h"	/* for opc_lookup() */
#include "opcode_ptrs.h"
#include "node.h"
#include "globals.h"
#include "opt_utilities.h"
#include "opt_statistics.h"
#include "opt_progstruct.h"
#include "opt_regsets.h"
#include "opt_context.h"

/******************************************************************************/
/***	      register contents -- manage regentry table		    ***/
/******************************************************************************/

static int
regfind( nm, basereg, size, firstreg, exact )
	struct name *nm;
	Regno basereg;
	int size;
	Regno firstreg;
	Bool *exact;
{
	/*
	 * ( lifted right out of the 68k version )
	 * look for a memory reference of the indicated size in the
	 * register table. Return the first register number
	 * found, if any. If the correspondence is exact, set
	 * *exact true, else false.
	 */
	register regentry *rp;
	register int membeg, memend;

	membeg = nm->nm_addend;
	memend = membeg + size;
	for ( rp = regtable + firstreg; rp < regtable+NREGS; rp += 1 ){
		if ( (*rp)[MEMORY].valid
		&& (*rp)[MEMORY].v.nm_symbol == nm->nm_symbol
		&& (*rp)[MEMORY].v.nm_addend < memend
		&& membeg < (*rp)[MEMORY].v.nm_addend+(*rp)[MEMORY].size )
			if ( nm->nm_symbol!= NULL
			|| (*rp)[MEMORY].basereg == basereg ){
				*exact=(*rp)[MEMORY].v.nm_addend==nm->nm_addend
				    && (*rp)[MEMORY].size       ==size;
				return rp-regtable;
			}
	}
	return -1;
}

static int
valfind( nm, firstreg, exact )
	struct name *nm;
	Regno firstreg;
	Bool *exact;
{
	/* look for a value in the register table */
	/* provide an exact value if possible */
	register regentry *rp, *rsave;

	rsave = NULL;
	for ( rp = regtable + firstreg; rp < regtable+NREGS; rp += 1 ){
		if ( (*rp)[VALUE].valid
		&&  !(*rp)[VALUE].hipart
		&& (*rp)[VALUE].v.nm_symbol == nm->nm_symbol
		&& thirteen_bits(magdiff((*rp)[VALUE].v.nm_addend , nm->nm_addend))){
			if ((*rp)[VALUE].v.nm_addend == nm->nm_addend){
				*exact = TRUE;
				return rp-regtable;
			}else
				rsave = rp;
		}
	}
	if (rsave != NULL){
		*exact = FALSE;
		return rsave-regtable;
	}
	return -1;
}

/* decompose address into its parts. return true if its cacheable, else false */
static Bool
compute_mem_address( np, nmp, brp )
	Node * np;
	struct name * nmp;
	Regno *brp;
{
	regentry *rp;
	if ( (! np->n_operands.op_imm_used) && (np->n_operands.op_regs[O_RS2] != RN_G0))
		return FALSE;   /* parse register-register form */
	rp = regtable + np->n_operands.op_regs[O_RS1];
	if (optimization_level >= 3 && (*rp)[VALUE].valid==TRUE){
		if ((*rp)[VALUE].v.nm_symbol == np->n_operands.op_symp
		 && (*rp)[VALUE].v.nm_addend == np->n_operands.op_addend
		 && (*rp)[VALUE].hipart == TRUE
		 && np->n_operands.op_reltype == REL_LO10){
		 	/* classical sethi/ld situation */
		 	*nmp = (*rp)[VALUE].v;
		 	/* and, assuming we can cache this thing!!... */
		 	return TRUE;
		} else if (np->n_operands.op_symp == NULL
		       && (*rp)[VALUE].hipart == FALSE ){
			/* classical sethi/add/ld situation */
			*nmp = (*rp)[VALUE].v;
			nmp->nm_addend += np->n_operands.op_addend;
			/* and, assuming we can cache this thing!!... */
		 	return TRUE;
		 }
	}
	/* it doesn't look like a name. It may be FP-relative */
	/* for the time being, don't believe any base register except FP & SP */
	if ( np->n_operands.op_regs[O_RS1] != RN_FPTR && 
	     np->n_operands.op_regs[O_RS1] != RN_SP )
		return FALSE;
	nmp->nm_symbol = np->n_operands.op_symp;
	nmp->nm_addend = np->n_operands.op_addend;
	*brp = np->n_operands.op_regs[O_RS1];
	return TRUE;
}

/* for DEBUGGING only! */
static void
print_regname( regno )
	int regno;
{
	extern char *regnames[]; /* from disassemble.c */
	if (IS_A_GP_REG(regno)){
		printf("%s",regnames[regno]);
	} else if (IS_AN_FP_REG(regno)){
		printf("%%f%u", regno - RN_FP(0));
	} else {
		printf("%%r%u", regno );
	}
}

void
print_regentry( rp, ve )
	struct regtable_entry *rp;
{
	if (rp->valid){
		if (rp->hipart) printf("%%hi(");
		if (rp->v.nm_symbol != NULL){
			printf("%s",rp->v.nm_symbol->s_symname);
		} else if (ve == MEMORY ){
			print_regname(rp->basereg);
		}
		if ( rp->v.nm_symbol == NULL ||  rp->v.nm_addend != 0){
			printf("+%d",rp->v.nm_addend);
		}
		if (rp->hipart) printf(")");
	}else{
		if (ve == VALUE ) printf("\t\t\t\t");
	}
}

void
regtable_dump(){
	Regno r;

	printf("reg	value				address\n");
	for ( r = 0 ; r < NREGS; r += 1 ){
		if (regtable[r][VALUE].valid || regtable[r][MEMORY].valid){
			print_regname(r);
			printf("	");
			print_regentry( &regtable[r][VALUE], VALUE );
			print_regentry( &regtable[r][MEMORY], MEMORY );
			printf("\n");
		}
	}
}

static void
regtable_forget_all_memory()
{
	register regentry *r;
	for ( r = regtable; r < regtable + NREGS; r += 1 )
		(*r)[MEMORY].valid = FALSE;
}

static void
regtable_forget_based_memory(br)
	Regno br;
{
	register regentry *r;
	for ( r = regtable; r < regtable + NREGS; r += 1 )
		if ( (*r)[MEMORY].v.nm_symbol == NULL
		  && (*r)[MEMORY].basereg   == br   )
			(*r)[MEMORY].valid = FALSE;
}

void
regtable_forget_all()
{
	register regentry *r;

	(*regtable)[VALUE].valid = TRUE;
        (*regtable)[VALUE].v.nm_symbol = NULL;                 
        (*regtable)[VALUE].v.nm_addend = 0;                     
	(*regtable)[MEMORY].valid = FALSE;   /* these are for %g0 */
	for ( r = regtable + 1; r < regtable + NREGS; r += 1 ){
		(*r)[VALUE].valid  = FALSE;
		(*r)[MEMORY].valid = FALSE;
	}
}

/******************************************************************************/
/***	      register contents -- discover and exploit			    ***/
/******************************************************************************/

Bool
replace_by_mov( np, srcreg, destreg )
	Node *np;
	Regno srcreg, destreg;
{
	/* replace the designated node by a "mov srcreg,destreg" */
	if (IS_AN_FP_REG(srcreg) || IS_AN_FP_REG(destreg) || 
	    IS_CSR_OR_FSR(srcreg) || IS_CSR_OR_FSR(destreg)) return FALSE;
	np->n_opcp = opcode_ptr[OP_OR];
#ifdef DEBUG
	np->n_optno = OPT_CONSTPROP;
#endif
	if ( np->n_operands.op_symp != NULL)
		delete_node_reference_to_label(np, np->n_operands.op_symp);
	np->n_operands.op_symp = NULL;
	np->n_operands.op_regs[O_RS1] = RN_G0;
	np->n_operands.op_imm_used = FALSE;
	np->n_operands.op_addend   = 0;
	np->n_operands.op_reltype  = REL_NONE;
	np->n_operands.op_regs[O_RS2] = srcreg;
	np->n_operands.op_regs[O_RD]  = destreg;
	regset_setnode( np );
	regset_recompute( np );
	regtable[destreg][VALUE] = regtable[srcreg][VALUE];
	if (regtable[srcreg][MEMORY].basereg == destreg)
		regtable[srcreg][MEMORY].valid = FALSE;
       	regtable[destreg][MEMORY] = regtable[srcreg][MEMORY];
	return TRUE;
}

Bool
replace_by_add( np, srcreg, destreg, val )
	Node *np;
	Regno srcreg, destreg;
	int val;
{
	/* replace the designated node by a "add srcreg,val,destreg" */
	if (IS_AN_FP_REG(srcreg) || IS_AN_FP_REG(destreg) ||
            IS_CSR_OR_FSR(srcreg) || IS_CSR_OR_FSR(destreg)) return FALSE;
	np->n_opcp = opcode_ptr[OP_ADD];
#ifdef DEBUG
	np->n_optno = OPT_CONSTPROP;
#endif
	if ( np->n_operands.op_symp != NULL)
		delete_node_reference_to_label(np, np->n_operands.op_symp);
	np->n_operands.op_symp = NULL;
	np->n_operands.op_regs[O_RS1] = srcreg;
	np->n_operands.op_imm_used = TRUE;
	np->n_operands.op_addend   = val;
	np->n_operands.op_reltype  = REL_13;
	np->n_operands.op_regs[O_RD]  = destreg;
	regset_setnode( np );
	regset_recompute( np );
	regtable[destreg][VALUE] = regtable[srcreg][VALUE];
	regtable[destreg][VALUE].v.nm_addend += val;
	regtable[destreg][MEMORY].valid = FALSE;
	return TRUE;

}

static Bool
lookup_and_replace( np, nm, add_ok )
	Node *np;
	struct name *nm;
	Bool add_ok;
{
	/* have a node and the register it writes. Try to replace with mov */
	Regno rname = np->n_operands.op_regs[O_RD];
	Bool exact;
	int sameval;

	if (is_mov_equivalent(np) || NODE_READS_I_CC(np)) 
		return FALSE; /* already minimal */
	sameval = valfind( nm, RN_G0, &exact );
/*	if (sameval == rname)
/*		sameval = valfind( nm, sameval+1, &exact );
 */
	if (sameval >= 0 ){
		if ( exact ){
			return replace_by_mov( np, sameval, rname);
		} else if (add_ok){
			return replace_by_add( np, sameval, rname,
				nm->nm_addend -
					regtable[sameval][VALUE].v.nm_addend );
		}
	}
	return FALSE;
}

static Bool
replace_cmp_and_branch( np, nm, changed_bb_struct, V )
	Node *np;
	struct name *nm;
	Bool *changed_bb_struct;
	Bool V; /* overflow bit */
{
        Bool N, Z, always;
	Node *br_np, *last_np;


	N = (nm->nm_addend < 0);
	Z = (nm->nm_addend == 0);
	/* find the next branch */
	for (br_np = np->n_next; OP_FUNC(br_np).f_group != FG_BR; br_np = br_np->n_next)
		if (OP_FUNC(br_np).f_setscc) return FALSE; /* can't do anything */

/*   See if the branch is effectively a branch always (or never) */

	switch (OP_FUNC(br_np).f_subgroup){
		case FSG_E:
			always = Z;
			break;
		case FSG_NE:
			always = !Z;
			break;
		case FSG_L:
			always = N ^ V;
			break;
		case FSG_NEG:
			always = N;
			break;
		case FSG_GE:
			always = !(N ^ V);
			break;
		case FSG_POS:
			always = !N;
			break;
		case FSG_LE:
			always = Z || (N ^ V);
			break;
		case FSG_G:
			always = !(Z || (N ^ V));
			break;
		case FSG_VC:
			always = !V;
			break;
		case FSG_VS:
			always = V;
			break;
		default:
			return FALSE;
	}
	/*  By the time we get here the branch is either always or never.
	 *  We can delete the cmp node.  If the branch is ALWAYS we have to
	 *  change the branch to always.  If it is NEVER we can delete it
	 *  carefully.
	 *
         * We actually nop_it (cmp) instead of deleteing so when we return to
	 * regtable_scan_blcok we don't have a null pointer problem.
         */
	nop_it(np);
	regset_setnode(np);
	optimizer_stats.cmp_outcome_known_succesful++;
	optimizer_stats.cmp_outcome_known_nodes_deleted++;
	if (always) {
		if (BRANCH_IS_ANNULLED(br_np)) br_np->n_opcp = opc_lookup("ba,a");
		else    br_np->n_opcp = opcode_ptr[OP_BA];		
		*changed_bb_struct = TRUE;
		regset_setnode(br_np);
		last_np = br_np;
	} else {      /* We can delete the branch as well */
		if (BRANCH_IS_ANNULLED(br_np)){ /* Delete delay slot also since
					         * its never executed.
						 */
		 	nop_it(br_np->n_next); 
			regset_setnode(br_np->n_next);
			optimizer_stats.cmp_outcome_known_nodes_deleted++;
			nop_it(br_np); 
			regset_setnode(br_np);
			last_np = br_np->n_next;
			optimizer_stats.cmp_outcome_known_nodes_deleted++;
			*changed_bb_struct = TRUE;
		} else { 		         /*  Delay slot must be left alone */
			nop_it(br_np);
			regset_setnode(br_np);
			last_np = br_np;
			optimizer_stats.cmp_outcome_known_nodes_deleted++;
			*changed_bb_struct = TRUE;
		}
	}
	regset_recompute(last_np);
	return TRUE;
}

/* record effects of sethi in register table */

static Bool
regtable_sethi( np )
	register Node * np;
{
	register regentry *r;
	r = regtable + np->n_operands.op_regs[O_RD];
	if ( r == regtable+RN_G0 ) return FALSE; /* has no effect */
	(*r)[MEMORY].valid = FALSE;
	(*r)[VALUE].valid = TRUE;
	(*r)[VALUE].v.nm_symbol = np->n_operands.op_symp;
	(*r)[VALUE].v.nm_addend = np->n_operands.op_addend;
	if (set_shifts_addend( np ) ){
		(*r)[VALUE].hipart = FALSE;
		(*r)[VALUE].v.nm_addend <<= sethi_shift();
	} else {
		(*r)[VALUE].hipart = TRUE;
	}
	return FALSE;
}

/* record effects of set in register table */

static Bool
regtable_set( np )
	Node * np;
{
	regentry *r;
	struct name nm;

	r = regtable + np->n_operands.op_regs[O_RD];
	if ( r == regtable+RN_G0 ) return FALSE; /* has no effect */
	nm.nm_symbol = np->n_operands.op_symp;
	nm.nm_addend = np->n_operands.op_addend;
	if ( lookup_and_replace( np, &nm, TRUE )) return TRUE;
	regtable_sethi( np );
	(*r)[VALUE].hipart = FALSE;

	return FALSE;
}

/* record effects of an arithmetic instruction in register table */
/*
 * here are the combinations we can hack:
 *
 *	       || === op is + ===  || === op is - === ||== op is other == ||
 *     src1    || src2 symbolic?   || src2 symbolic   || src2 symbolic?   ||
 *   symbolic? ||  yes   |  no     ||  yes   |  no    ||  yes   |   no    ||
 * ------------++--------+---------++--------+--------++--------+---------++
 *	       ||	 |	   ||	     |	      ||	|	  ||
 *	yes    ||   (a)  |  (b)    ||   (U)  |  (b)   ||  (U)   |  (U)    ||
 * ------------++--------+---------++--------+--------++--------+---------++
 *	       ||	 |	   ||	     |	      ||	|	  ||
 *	no     ||   (b)  |  (c)    ||   (U)  |  (c)   ||  (U)   |  (c)    ||
 * ------------++--------+---------++--------+--------++--------+---------++
 *
 * Where:
 *	(a) 	symbol + symbol (where all the action is)
 *		Additional constraints:
 *		    it must reference the same NAME (includes addend!)
 *		    src1 must be high-part only
 *		    src2 must be low-part only
 *		Result: valid, value is full NAME
 *
 *	(b)	symbol + constant, constant + symbol, symbol - constant
 *		Additional constraints:
 *		    symbol must be full value.
 *		Result:  valid, full NAME+/-constant value
 *
 *	(c)	constant <op> constant
 *		Result: valid, constant value computable.
 *
 *	(U)	other combinations, and above cases that don't meet the
 *		constraints.
 *		Result: not valid (UNKNOWN).
 */

static Bool
regtable_arith( np, changed_bb_struct )
	register Node * np;
	Bool *changed_bb_struct;
{
	register regentry *dest, *srcreg;
	struct name src1, src2;
	struct symbol *sympart;
	RELTYPE src1rel, src2rel;
	long int result;
	int sameval;
	Bool is_cmp;	/* for special case of cmp reg(rs1),src2 instruction */
	Bool V_bit;     /* simulate overflow bit */
	Bool sign_1, sign_2, sign_res;
	

	/* We are guaranteed that OP_FUNC(np).f_group == FG_ARITH, here. */
#ifdef DEBUG
	if (OP_FUNC(np).f_group != FG_ARITH)
		internal_error("regtable_arith(): not ARITH");
#endif /*DEBUG*/
	
	is_cmp = FALSE;
	dest = regtable + np->n_operands.op_regs[O_RD];
	if (dest == regtable+RN_G0){
		if (OP_FUNC(np).f_subgroup == FSG_SUB) is_cmp = TRUE;
		else return FALSE;	/* has no effect */ 
	}

	if (!is_cmp) (*dest)[MEMORY].valid = FALSE; /* blasts the destination reg */

	/*
	 * We will make the gross assumption that
	 * the result somhow involves the source register(s).
	 * Thus, if they are undefined, the result will be, too
	 */
	srcreg = regtable + np->n_operands.op_regs[O_RS1];
	if ((*srcreg)[VALUE].valid == FALSE){
		(*dest)[VALUE].valid = FALSE;
		return FALSE;
	}

	src1 = (*srcreg)[VALUE].v;
	src1rel = ((*srcreg)[VALUE].hipart) ? REL_HI22 : REL_NONE;
	result = src1.nm_addend;
	if (np->n_operands.op_imm_used){
		src2.nm_symbol = np->n_operands.op_symp;
		src2.nm_addend = np->n_operands.op_addend;
		src2rel        = np->n_operands.op_reltype;
	} else {
		srcreg = regtable + np->n_operands.op_regs[O_RS2];
		if ((*srcreg)[VALUE].valid == FALSE){
			(*dest)[VALUE].valid = FALSE;
			return FALSE;
		}
		src2 = (*srcreg)[VALUE].v;
		src2rel = ((*srcreg)[VALUE].hipart) ? REL_HI22 : REL_NONE;
	}

	if ( is_mov_equivalent(np) ) {
		/* This instruction is effectively a MOV, which always falls
		 * under case (b).  First, convert it into a "real" MOV,
		 * i.e. "or  %0,REG|CONSTANT,RD".
		 */
		if ( convert_mov_equivalent_into_real_mov(np) ) {
			/* It swapped the operands; so swap our notion of them,
			 * too, and recompute the usage information.
			 * Since we KNOW that RS1 is now %g0, we don't really
			 * have to do a full swap of the contents of src1 and
			 * src2; we can use NULL, 0, and REL_NONE instead of
			 * saved copies of src2.nm_symbol, src2.nm_addend, and
			 * src2rel, respectively.
			 */
			src2    = src1;		/* gets nm_symbol & nm_addend.*/
			src2rel = src1rel;

			src1.nm_symbol = NULL;
			src1.nm_addend = 0;
			src1rel        = REL_NONE;

			regset_setnode( np );
			regset_recompute( np );
		}
		else /* It left the operands alone; do nothing. */ ; 

		goto case_b_MOV;
	}

	/* the two major cases are with and without symbol part */
	if ( src2.nm_symbol == NULL ){
		/* regular-like arithmetic */
		switch (OP_FUNC(np).f_subgroup){
		case FSG_ADD:
			result = result + src2.nm_addend;
			if (src1.nm_symbol != NULL){
				sympart = src1.nm_symbol;
				goto case_b;
			}
			break;
		case FSG_SUB:
			if (is_cmp) {
				sign_1 = result >= 0;
				sign_2 = src2.nm_addend >= 0;
			}
			result = result - src2.nm_addend;
			if (is_cmp) {
				sign_res = result >= 0;
				V_bit = ( sign_1 && !sign_2 && !sign_res ) ||
					( !sign_1 && sign_2 && sign_res );
			}
			if (src1.nm_symbol != NULL){
				sympart = src1.nm_symbol;
				goto case_b;
			}
			break;
		case FSG_MULS:
		case FSG_TADD:
		case FSG_TSUB:
		case FSG_SAVE:
		case FSG_REST:
			/* don't know what to do in compiled code */
			(*dest)[VALUE].valid = FALSE;
			return FALSE;
		case FSG_AND:
			result = result & src2.nm_addend;
			break;
		case FSG_ANDN:
			result = result & ~src2.nm_addend;
			break;
		case FSG_OR:
			result = result |  src2.nm_addend;
			break;
		case FSG_ORN:
			result = result | ~src2.nm_addend;
			break;
		case FSG_XOR:
			result = result ^  src2.nm_addend;
			break;
		case FSG_XORN:
			result = result ^ ~src2.nm_addend;
			break;
		case FSG_SLL:
			result = result <<  src2.nm_addend;
			break;
		case FSG_SRL:
			result = (unsigned)result >>  src2.nm_addend;
			break;
		case FSG_SRA:
			result = result >>  src2.nm_addend;
			break;
		}
		if (src1.nm_symbol == NULL){
			goto case_c;
		}else{
			goto UNDEFINED;
		}
	} else {
		/* src2 symbolic: "+" is ok; others: cannot cope */
		switch (OP_FUNC(np).f_subgroup){
		case FSG_ADD:
			result = result + src2.nm_addend;
			if (src1.nm_symbol == NULL){
				sympart = src2.nm_symbol;
				goto case_b;
			} else {
				goto case_a;
			}
			break;
		default:
			goto UNDEFINED;
		}

	}
	/* now that we've determined the upper level conditions, here are
	 * the four further cases, with their additional constraints
	 */
case_a:
	if ( (src1.nm_symbol != src2.nm_symbol)
	     || (src1rel != REL_HI22) || (src2rel != REL_LO10) )
		goto UNDEFINED;
	src1.nm_addend = result;
	if (lookup_and_replace( np, &src1, TRUE) ) return TRUE;
        if (dest != regtable)   
	{
		(*dest)[VALUE].valid       = TRUE;
		(*dest)[VALUE].v	   = src1;
		(*dest)[VALUE].hipart      = FALSE;
	}
	return FALSE;

case_b:
	if ( (src1rel != REL_NONE) || 
	     ( (src2rel != REL_NONE) && (src2rel != REL_13) ) )
		goto UNDEFINED;
	src1.nm_symbol = sympart;
	src1.nm_addend = result;
	if (is_cmp) return replace_cmp_and_branch( np, &src1, changed_bb_struct, V_bit );
	if (lookup_and_replace( np, &src1, FALSE) ) return TRUE;
        if (dest != regtable)    
        { 
		(*dest)[VALUE].valid       = TRUE;
		(*dest)[VALUE].v	   = src1;
		(*dest)[VALUE].hipart      = FALSE;
	}
	return FALSE;

case_b_MOV:
	/* Special subcase of case (b);
	 * mainly, saves a call to lookup_and_replace().
	 */
	if ( (src1rel != REL_NONE) ||
	     ( (src2rel != REL_NONE) && (src2rel != REL_13) ) )
		goto UNDEFINED;
        if (dest != regtable)    
        { 
		(*dest)[VALUE].valid       = TRUE;
		(*dest)[VALUE].v	   = src2;
		(*dest)[VALUE].hipart      = FALSE;
	}
	return FALSE;

case_c:
	src1.nm_symbol = NULL;
	src1.nm_addend = result;
	if (is_cmp) return replace_cmp_and_branch( np, &src1, changed_bb_struct, V_bit );
	if (lookup_and_replace( np, &src1, FALSE) ) return TRUE;
        if (dest != regtable)    
        { 
		(*dest)[VALUE].valid       = TRUE;
		(*dest)[VALUE].v	   = src1;
		(*dest)[VALUE].hipart      = FALSE;
	}
	return FALSE;

UNDEFINED: /* case U */
	(*dest)[VALUE].valid       = FALSE;
	return FALSE;

}

/* record effects of store in register table */
static Bool
regtable_st( np )
	Node *np;
{
	Bool exact;
	int  regno, findreg;
	struct name nm;
	Regno basereg;
	int size;
	Bool redundant;
	int nfound;

	if (!compute_mem_address( np, &nm, &basereg )){
		regtable_forget_all_memory(); /* could have clobbered anything */
		return FALSE;
	}
	
	size  = np->n_opcp->o_func.f_rd_width;
	regno = np->n_operands.op_regs[O_RD];
	redundant = FALSE;
	nfound = 0;


	/* delete all known misinformation */
	for ( findreg = regfind( &nm, basereg, size, RN_G0, &exact);
	      findreg != -1;
	      findreg = regfind( &nm, basereg, size, findreg+1, &exact)){
		/* check for the (somewhat) special case of storing back into our source */
		if ( exact && findreg==regno ){
			redundant = TRUE;
		} else
			regtable[findreg][MEMORY].valid = FALSE;
		nfound += 1;
	}

	/* add new misinformation to table */
	regtable[regno][MEMORY].valid   = TRUE;
	regtable[regno][MEMORY].v       = nm;
	regtable[regno][MEMORY].basereg = basereg;
	regtable[regno][MEMORY].size    = size;

	if (redundant && ( (size <= 4 && nfound == 1) ||
			   (size == 8 && nfound == 2) ))
	{
			  
		/* store whence we came, AND no aliases */
		nop_it(np);
		return TRUE;
	} else 
		return FALSE;
}

/* record effects of load in register table */
static Bool
regtable_ld( np )
	Node *np;
{
	Bool		exact;
	int		regno,i;
	struct name	nm;
	Regno		basereg;
	Regno		destreg;
	int		size;

	destreg = np->n_operands.op_regs[O_RD];
	size = np->n_opcp->o_func.f_rd_width;
	if (!compute_mem_address( np, &nm, &basereg ) ){
		for (i = 0; i < BYTES_TO_WORDS(size); i++){
			regtable[destreg+i][VALUE].valid  = FALSE;
			regtable[destreg+i][MEMORY].valid = FALSE;
		};
		return FALSE;
	}
	
	/* first, see if we even care about this */
	if (regset_empty(regset_intersect(following_liveset(np), REGS_WRITTEN(np))) &&
	    regset_empty(regset_intersect(REGS_READ(np->n_next), REGS_WRITTEN(np)))){
		/* the deal's off! */
		/*
		 * we'd like to have done this eariler, but couldn't
		 * properly run compute_mem_address without knowning
		 * register contents.
		 */
		nop_it(np);
		return TRUE;
	}

	/* see if we already know what is in the memory location */
	/*
	 * HACK: this will only work for full-word accesses
	 * e.g. it shouldn't touch
	 *	sth	%o0,...
	 *	lduh	...,%o0
	 * because the lduh zero-extends the contents. Someday,
	 * perhaps, we'll replace the ld by shifting & masking.
	 */
	if ( size == 4 ) {
		for ( regno = regfind( &nm, basereg, size, RN_G0, &exact);
		      regno != -1;
		      regno = regfind( &nm, basereg, size, regno+1, &exact))
			    if ( exact ){
				if ( replace_by_mov( np, regno, destreg))
					return TRUE;
			    }
	}
	for (i = 0; i < BYTES_TO_WORDS(size); i++){
		regtable[destreg+i][VALUE].valid    = FALSE;
		/* add new information to the table */
	}
	if (regset_empty(regset_intersect(REGS_READ(np), REGS_WRITTEN(np)))){
		for (i = 0; i < BYTES_TO_WORDS(size); i++){
			regtable[destreg+i][MEMORY].valid   = TRUE;
			regtable[destreg+i][MEMORY].v       = nm;
			regtable[destreg+i][MEMORY].basereg = basereg;
			regtable[destreg+i][MEMORY].size    = size;
		};
	}
	else {
		for ( i = 0; i < BYTES_TO_WORDS(size); i++)
			regtable[destreg+i][MEMORY].valid = FALSE;
	}
	return FALSE;	
}


/* record effects of CALL in register table */
Bool
regtable_call( p )
	Node *p;
{
	/* 
	 * this is easy :
	 * 1) assume all memory dead.
	 * 2) assume all outs and globals dead.
	 */
	 register Regno r;
	 regtable_forget_all_memory();
	 for (r = RN_G0; r<=RN_CALLPTR; r+= 1)
	 	regtable[r][VALUE].valid = FALSE;
	 return FALSE;
}

Bool
regtable_scan_block( bbp, changed_bb_struct )
	struct basic_block *bbp;
	Bool *changed_bb_struct;
{
	Node *ultimate;
	register Node *p;
	Bool did_work;
	int regno;

	did_work = FALSE;
	for ( p = bbp->bb_dominator, ultimate = bbp->bb_trailer->n_next;
	      p != ultimate;
	      p = p->n_next){
		switch ( OP_FUNC(p).f_group ){
		case FG_SET:
			did_work |= regtable_set( p ); break;
		case FG_SETHI:
			did_work |= regtable_sethi( p ); break;
		case FG_ARITH:
			did_work |= regtable_arith( p, changed_bb_struct ); break;
		case FG_LD:
			did_work |= regtable_ld( p ); break;
		case FG_ST:
			did_work |= regtable_st( p ); break;
		case FG_JMPL:
		case FG_CALL:
			did_work |= regtable_call( p ); break;
		default:
			/* anything this instruction writes is dead */
			for ( regno = regset_select( REGS_WRITTEN(p), RN_G0 );
			      regno != -1;
			      regno = regset_select( REGS_WRITTEN(p), regno+1 )
			    ){
			      	    regtable[regno][VALUE].valid  = FALSE;
			      	    regtable[regno][MEMORY].valid = FALSE;
			}
			break;
		}
	}
	return did_work;
}
