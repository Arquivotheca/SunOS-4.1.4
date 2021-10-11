#ifndef lint
static char sccs_id[] = "@(#)opt_deldead.c	1.1	94/10/31";
#endif
/*
 * Here we do simple optimizations that use the register set information
 * computed for each node. Examples are:
 *	deleting dead computations.
 */

#include <stdio.h>
#include "registers.h"
#include "structs.h"
#include "bitmacros.h"
#include "optimize.h"
#include "sym.h"
#include "opt_utilities.h"
#include "opt_statistics.h"
#include "opt_regsets.h"
#include "opt_deldead.h"
#include "opcode_ptrs.h"

/*****************************************************************************/

/* cannot coalesce expressions into %fp or %sp or %g0 */
#define CAN_COALESCE_INTO(r) ((r) != RN_FPTR && (r) != RN_SP && (r) != RN_G0 \
		              && !IS_CSR_OR_FSR(r))

#define CAN_COALESCE_PAIR(r1,r2) ( (!IS_AN_FP_REG(r1) && !IS_AN_FP_REG(r2)) || \
				   (IS_AN_FP_REG(r1) && IS_AN_FP_REG(r2)) )


Bool
second_operand_is_zero( np )
	register Node * np;
{
	if ( np->n_operands.op_imm_used &&
	     (np->n_operands.op_symp == NULL) && 
	     (np->n_operands.op_addend == 0) )
	{
		return TRUE;
	}
	else if ( !np->n_operands.op_imm_used &&
		  (np->n_operands.op_regs[O_RS2] == RN_G0) )
	{
		return TRUE;
	}
	else
		return FALSE;
}

Bool
second_operand_is_one( np )
	register Node * np;
{
	if ( np->n_operands.op_imm_used &&
	     (np->n_operands.op_symp == NULL) && 
	     (np->n_operands.op_addend == 1) )
	{
		return TRUE;
	}
	else 	return FALSE;
}

static Bool
is_extraneous_tst( np )
	register Node *np;
{
	/*
	 * can we avoid an instruction by setting the cc on
	 * some previous arithmetic instruction?
	 */
	register Regno r;
	register Node *setr;
	enum functional_group fg;
	
	/* first question: is it even a tst-type instruction? */
	/* (we assume np--> a subtraction instruction) */
	if ( !( OP_FUNC(np).f_setscc
		&& second_operand_is_zero(np)
		&& (np->n_operands.op_regs[O_RD] == RN_G0) )
	   )
	    	return FALSE;

	/* next, look backwards for the sucker that last set src register */
	/* make sure we don't tromp on a cc that's in use */
	r = np->n_operands.op_regs[O_RS1];
	for ( setr=np->n_prev; ; setr=setr->n_prev ){
		if (regset_in(REGS_LIVE(setr), RN_ICC))
			return FALSE; /* oops -- it's busy */
		switch (fg = OP_FUNC(setr).f_group ){
		case FG_MARKER:
		case FG_LABEL:
			return FALSE; /* forget it */
		}
		if (regset_in(REGS_WRITTEN(setr), r)){
			if ( fg == FG_ARITH ){
			    switch (OP_FUNC(setr).f_subgroup ){
			    case FSG_ADD:
			    case FSG_SUB:
			    case FSG_MULS:
			    case FSG_AND:
			    case FSG_ANDN:
			    case FSG_OR:
			    case FSG_ORN:
			    case FSG_XOR:
			    case FSG_XORN:
				/*
				 * this is the place.
				 * Make sure we're not in a CALL's (or JMPL)
				 * delay slot.
				 */
				if (NODE_IS_CALL(setr->n_prev))
					return FALSE;
				if (!OP_FUNC(setr).f_setscc){
					/* substitute in a version that does! */
					char name[20];
					sprintf(name,"%scc", setr->n_opcp->o_name);
					setr->n_opcp = opc_lookup(name);
					regset_setnode(setr);
				}
				return TRUE;
			    }
			}
			return FALSE;
		}
		if (regset_in(REGS_WRITTEN(setr), RN_ICC))
			return FALSE; /* oops -- it's dead */
	}
	/* NOTREACHED */
}

Bool
is_mov_equivalent( np )
	Node *np;
{
	/* Returns TRUE if this instruction is effectively a MOV or an FMOVS
	 * from either RS1 or RS2, to RD.
	 */

	switch (OP_FUNC(np).f_group){
	case FG_ARITH:
		switch (OP_FUNC(np).f_subgroup){
			/* For the commutative operations, either operand
			 * can be zero to effect a MOV.
			 */
		case FSG_ADD:
			if (NODE_READS_I_CC(np)) return FALSE;
		case FSG_OR:
		case FSG_XOR:
			return ( first_operand_is_zero( np ) ||
				 second_operand_is_zero( np ) );

			/* For the non-commutative operations, the SECOND
			 * operand must be zero to effect a MOV.
			 */
		case FSG_SUB:
			if (NODE_READS_I_CC(np)) return FALSE;
		case FSG_SLL:
		case FSG_SRL:
		case FSG_SRA:
			return second_operand_is_zero( np );

		default:
			return FALSE;
		}
	case FG_FLOAT2:
		return (OP_FUNC(np).f_subgroup == FSG_FMOV);
	default:
		return FALSE;
	}
}

static Bool
is_mov_equivalent_from_given_reg( np, whichreg )
	Node *np;
	int whichreg;
{
	/* Returns TRUE if this instruction is effectively a MOV (or an FMOVS)
	 * from the given register (O_RS1 or O_RS2), to RD.
	 */

	switch (whichreg)
	{
	case O_RS1:
		/* Is this a MOV-equivalent from RS1 to RD? */
		switch (OP_FUNC(np).f_group)
		{
		case FG_ARITH:
			/* Can have a a MOV-equivalent from RS1 with many of
			 * the arithmetic operations, commutative or not, as
			 * long as the second operand is zero.
			 */
			switch (OP_FUNC(np).f_subgroup)
			{
			case FSG_ADD:
			case FSG_SUB:
				if (NODE_READS_I_CC(np)) return FALSE;
			case FSG_OR:
			case FSG_XOR:
			case FSG_SLL:
			case FSG_SRL:
			case FSG_SRA:
  				return second_operand_is_zero( np );
			}
		}
		break;

	case O_RS2:
		/* Is this a MOV-equivalent from RS2 to RD? */
		switch (OP_FUNC(np).f_group)
		{
		case FG_ARITH:
			/* Can only have a MOV-equivalent from RS2 when the
			 * arithmetic operation is commutative, and the first
			 * operand is zero.
			 */
			switch (OP_FUNC(np).f_subgroup)
			{
			case FSG_ADD:
				if (NODE_READS_I_CC(np)) return FALSE;
			case FSG_OR:
			case FSG_XOR:
				return first_operand_is_zero(np);
			}
			break;
		case FG_FLOAT2:
			return (OP_FUNC(np).f_subgroup == FSG_FMOV);
		}
		break;
	}

	return FALSE;	/* the default case */
}


Bool
convert_mov_equivalent_into_real_mov( np )
	register Node *np;
{
	/* Massages a MOV-equivalent node into a "real" MOV ("or  %0,reg,rd")
	 * or a MOVCC-equivalent node into a "real" MOVCC ("orcc %0,reg,rd").
	 *
	 * Since none of the referenced registers or their tracked contents
	 * are changed, we don't bother to call regset_setnode() or
	 * regset_recompute() after this conversion.
	 *
	 * Returns TRUE if it swapped the operands, FALSE if it left them
	 * in their original order.
	 */


	if (np->n_opcp->o_func.f_setscc)
	{
		np->n_opcp = opcode_ptr[OP_ORCC];/* The "Real" MOVCC opcode. */
	}
	else	np->n_opcp = opcode_ptr[OP_OR];	 /* The "Real" MOV opcode. */

	if ( np->n_operands.op_regs[O_RS1] == RN_G0 )
	{
		/* The operands are already in the right order. */
		return FALSE;
	}
	else
	{
		/* We have "<blah> REG,%0,RD".
		 * The 2nd operand is the Zero one; reverse the operands.
		 */
		np->n_operands.op_regs[O_RS2] = np->n_operands.op_regs[O_RS1];
		np->n_operands.op_imm_used    = FALSE;
		np->n_operands.op_reltype     = REL_NONE;
		np->n_operands.op_regs[O_RS1] = RN_G0;

		return TRUE;
	}
	/*NOTREACHED*/
}


static Bool
convert_tst_equivalent_into_real_tst( np )
	register Node *np;
{
	/* Massages a MOVCC-equivalent node with a null_effect() into a "real"
	 * TST ("orcc %0,reg|const,%0").
	 *
	 * Since none of the referenced registers or their tracked contents
	 * are changed, we don't bother to call regset_setnode() or
	 * regset_recompute() after this conversion.
	 *
	 * Returns TRUE if it swapped the operands, FALSE if it left them
	 * in their original order.
	 */

	static char *fcn_name = "convert_tst_equivalent_into_real_tst()";

	/* We don't need to worry about np->n_operands.op_symp referring to
	 * some symbol, since a MOVCC-equivalent instruction would only be
	 * referencing registers (and maybe an immediate value).
	 */
#ifdef DEBUG
	if (np->n_operands.op_symp != NULL)
		internal_error( "%s: symp != NULL", fcn_name);
#endif /*DEBUG*/

	if (np->n_opcp->o_func.f_setscc)
	{
		np->n_opcp = opcode_ptr[OP_ORCC];/* The "Real" MOVCC opcode. */
	}
	else	internal_error( "%s: !setscc", fcn_name);

	if ( np->n_operands.op_regs[O_RS1] == RN_G0 )
	{
		/* The operands are already in the right order. */
		return FALSE;
	}
	else
	{
		/* We have "<blah> REG,%0,RD".
		 * The 2nd operand is the Zero one; reverse the operands.
		 */
		np->n_operands.op_regs[O_RS2] = np->n_operands.op_regs[O_RS1];
		np->n_operands.op_imm_used    = FALSE;
		np->n_operands.op_reltype     = REL_NONE;
		np->n_operands.op_regs[O_RS1] = RN_G0;

		return TRUE;
	}
	/*NOTREACHED*/
}


/* Replace any occurence of any register in rold_set  by the corresponding
 * register in rnew_set.
 */
static void
replace_reg_in_range( nfirst, nlast, rold_set, rnew_set, optno, reg_offset, sign )
	Node *nfirst, *nlast;
	register_set rold_set, rnew_set;
	OptimizationNumber optno;
        Regno reg_offset;
        Bool sign;
{
	register int i, lower_i, upper_i;
	register Bool fixed;
	register Node *p, *npast, *last_fixed;

	npast = nlast->n_next;
	last_fixed = nlast;      /* just a default */
	lower_i = O_RD;          /* first node, only rewrite destination */
	upper_i = O_N_OPS-1;
	for (p = nfirst; p!= npast; p = p->n_next){
		fixed = FALSE;
		if (p == nlast) { 
			switch(OP_FUNC(p).f_group){
			/* last node */
			case FG_ST: /* store reads "dest" reg */
				upper_i = O_RD; /* as was already the case*/
				break; 
			default:
				upper_i = O_RS2; /* don't rewrite destination*/
			}
	        } 
		if ( p->n_opcp->o_func.f_node_union == NU_OPS ) {
			for (i = lower_i; i <= upper_i; i++ ){
				/* Replace references to the old register with
				 * references to the new one, being careful to
				 * not tamper with an unused RS2 register.
				 */
				if (!((i == O_RS2) && p->n_operands.op_imm_used) &&
				    (regset_in(rold_set, p->n_operands.op_regs[i]) ||
				     (sign && regset_in(rnew_set, p->n_operands.op_regs[i]+reg_offset)) ||
				     (!sign && (p->n_operands.op_regs[i] > reg_offset) &&
				      regset_in(rnew_set,p->n_operands.op_regs[i]-reg_offset)))){
					p->n_operands.op_regs[i] =
				                          sign ? (p->n_operands.op_regs[i]+reg_offset)
							       : (p->n_operands.op_regs[i]-reg_offset);
					fixed = TRUE;
				}
			}
		}
		if (fixed) {
			regset_setnode(p);
			last_fixed = p;
#ifdef DEBUG
			p->n_optno = optno;
#endif /*DEBUG*/
		}
		lower_i = O_RS1;
	}
	regset_recompute(last_fixed);
}

static Bool
operand_width_ok ( np, regset, op_width, check_source )
	register Node * np;
	register_set regset;
	OperandWidth op_width;
	Bool check_source;   /* tell whether to check source or destination registers */
{
	if (check_source)
	{
		OperandWidth source_width;
		register_set source_rset;

		/* Special case for stores because rd is a "source" (read)
		 * register.
		 */
		if ( NODE_IS_STORE(np) )
		{
			source_width = BYTES_TO_WORDS(OP_FUNC(np).f_rd_width);
			if (source_width == SN) source_width++;
			source_rset = regset_mkset_multiple(np->n_operands.op_regs[O_RD], source_width);
			if ( !regset_empty(regset_intersect(regset,source_rset)) )
			{
				if ( op_width != source_width ) return FALSE; 
			}
		}
		/* Special case is faster */
		if (np->n_operands.op_imm_used)
		{
			source_width = BYTES_TO_WORDS(OP_FUNC(np).f_rs1_width);
			if (source_width == SN) source_width++;
			if ( op_width != source_width ) return FALSE; 
		}
		else /* have to figure if it is rs1 or rs2 */
		{
			source_width = BYTES_TO_WORDS(OP_FUNC(np).f_rs1_width);
			if (source_width == SN) source_width++;
			source_rset = regset_mkset_multiple(np->n_operands.op_regs[O_RS1], source_width);
			if ( !regset_empty(regset_intersect(regset,source_rset)) )
			{
				if ( op_width != source_width ) return FALSE; 
			}
			source_width = BYTES_TO_WORDS(OP_FUNC(np).f_rs2_width);
			if (source_width == SN) source_width++;
			source_rset = regset_mkset_multiple(np->n_operands.op_regs[O_RS2], source_width);
			if ( !regset_empty(regset_intersect(regset,source_rset)) )
			{
				if ( op_width != source_width ) return FALSE; 
			}
			
		}
	}
	else
	{
	        OperandWidth rd_width = BYTES_TO_WORDS(OP_FUNC(np).f_rd_width);
		if (rd_width == SN) rd_width++;
		if ( op_width != rd_width ) return FALSE;
	}
	return TRUE;
	
}

/* Determine if this is a useless move:
 *
 * We're looking for patterns such as these:
 *
 *	mov	%i5,%o0			ld	[%i5],%o0
 *	add	%o0,1,%i4		mov	%o0,%i5
 *	! <<o0 dead here>>		! <<o0 dead here>>
 *
 * Which we can turn into:
 *
 *	mov	%i5,%i5			ld	[%i5],%i5
 *	add	%i5,1,%i4		mov	%i5,%i5
 *
 * and hope that the selfmove ("mov %i5,%i5") is deleted later.
 *
 *
 * More generally, we can look for a list of nodes like this:
 *	(1)	a -> x
 *	(2)	<sequence of instructions>
 *	(3)	x' -> b 
 * where x' is the same register as x, perhaps with some arithmetic done
 * to it, and this is the end of the lifetime for register x.
 * The last instruction could be just a test or store, in which
 * case there is no (b). We seem to have these choices:
 *
 *	let
 *	   A == "register 'a' dead over sequence (2)"
 *			["source_dead"]
 *	   A2== "register 'a' unmodified over sequence (2)"
 *			["source_unchanged"]
 *	   B == "there exists a register 'b', dead over sequence (2)"
 *			["target_dead"]
 *	   C == "x'==x (no computation)"
 *			["comp_unchanged"]
 *
 *		|   --- C ---   || --- not C --- |
 *		|  B    | not B ||  B    | not B |
 *		+-------+-------++-------+-------+
 *	    A	|  (p)  |  (q)  ||  (p)  |  (q)  |
 *		+-------+-------++-------+-------+
 * not A and A2	|  (p)  |  (q)  ||  (p)  |  (X)  |
 *		+-------+-------++-------+-------+
 * not A not A2	|  (p)  |  (X)  ||  (p)  |  (X)  |
 *		+-------+-------++-------+-------+
 *
 *	where
 *	  (p)   replace register x ("comp_reg") by register b ("final_target")
 *			throughout
 *	  (q)   replace register x ("comp_reg") by register a ("sourcereg")
 *			throughout
 *	  (X)   do nothing
 *
 * NOTE:
 * To generalize (to allow floating point register coalescing), we deal with
 * register sets.  In the case of integer or single precision floating point
 * intructions our register sets will be singletons.  In case of double 
 * (or higher) percision instructions our registers will be (unordered)
 * pairs (quads) of registers.
 */

static Bool
coalesce_registers( np, whichreg )
	register Node *np;	/* The first node in the sequence. */
	int	whichreg;	/* The register on which to try coalescing. */
{
	register_set source_regset;    /* register 'a' */
	register Regno source_reg;
	register_set comp_regset;      /* register 'x' */
	register Regno comp_reg;
	register_set final_target_set; /* register 'b' */
	register Regno final_target;
	register Node *use, *p;
	Regno reg_offset;
	Bool sign;
	Node *brdest;
	Bool source_dead;		/* condition A */
	Bool source_unchanged;		/* condition A2 */
	Bool target_dead;		/* condition B */
	Bool comp_unchanged;		/* condition C */
	Bool whichreg_is_rd;            /* special case when whichreg is same
					   as rd */
	OperandWidth sourcereg_width, compreg_width, final_target_width;


	/* assume it's a 3-address instruction. */

	switch (whichreg)
	{
	case O_RS1:
		if (!do_opt[OPT_COALESCE_RS1])	return FALSE;
		break;
	case O_RS2:
		if (!do_opt[OPT_COALESCE_RS2])	return FALSE;
		break;
	default:
		internal_error("coalesce_registers(): whichreg=%d?", whichreg);
	}

	/* Can't coalesce if the instruction doesn't even use the register
	 * on which we're to coalesce.
	 */ 
	if ((whichreg == O_RS2) && np->n_operands.op_imm_used)	return FALSE;

	/* Get the comp_regset. */
	comp_reg = np->n_operands.op_regs[O_RD];
	compreg_width =	BYTES_TO_WORDS(np->n_opcp->o_func.f_rd_width);
	if (compreg_width == SN) compreg_width++;
	comp_regset = regset_mkset_multiple(comp_reg, compreg_width);

	/* Get the source_regset. */
	source_reg = np->n_operands.op_regs[whichreg];
	sourcereg_width = BYTES_TO_WORDS((whichreg == O_RS1) ? 
						 np->n_opcp->o_func.f_rs1_width :
						 np->n_opcp->o_func.f_rs2_width);
	if (sourcereg_width == SN) sourcereg_width++;
	if (source_reg == RN_G0) source_regset = zero_regset;
	else source_regset = regset_mkset_multiple(source_reg, sourcereg_width);

	if (comp_reg == RN_G0) return FALSE;
	if (opt_node_is_in_delay_slot(np)) return FALSE;
	if (!regset_empty(regset_intersect( comp_regset, REGS_READ(np) ))) return FALSE;


	source_dead = CAN_COALESCE_INTO(source_reg);
	source_unchanged = TRUE;
	comp_unchanged = is_mov_equivalent_from_given_reg( np, whichreg );
        whichreg_is_rd = FALSE;

	for (use = np->n_next;
	     /* no loop termination test here */;
	     use = use->n_next){
		switch (OP_FUNC(use).f_group){
		
		case FG_MARKER:
		case FG_LABEL:
			return FALSE; /* cannot cope across labels */
		
		case FG_BR:
			/* Check to see if target is live at destination.
			 * If it is, we can do nothing.
			 * If it is not, we will soon have the end of our range.
			 */
                        if ((use->n_next->n_opcp != opcode_ptr[OP_NOP]) &&
                            BRANCH_IS_ANNULLED(use))
                                brdest = use->n_next;
                        else    brdest = use->n_operands.op_symp->s_def_node;
	 	        if (!regset_empty(regset_intersect(comp_regset, REGS_LIVE(brdest)))){
				return FALSE;
			} else {
				continue;
			}
		
		case FG_JMPL:	
		case FG_CALL:
			/* don't try to optimize away parameters! */
			if (!regset_empty(regset_intersect(comp_regset, REGS_READ(use))))
				return FALSE;
		case FG_JMP:
		case FG_RET:
			/* forget this noise */
			return FALSE;
		}
		
		/*
		 * The order of these tests is important,
		 * since sourcereg==final_target is possible.
		 */
		if (source_dead && !regset_empty(regset_intersect(source_regset, REGS_LIVE(use))))
			source_dead = FALSE;
		whichreg_is_rd = !regset_empty(regset_intersect(REGS_WRITTEN(use), 
								REGS_READ(use))) &&
				 !regset_empty(regset_intersect(comp_regset, 
								REGS_WRITTEN(use)));

		if (!regset_empty(regset_intersect(comp_regset, REGS_READ(use))))
		        if (!operand_width_ok(use, comp_regset, compreg_width, TRUE)) return FALSE;

		if (regset_empty(regset_intersect(comp_regset, following_liveset(use))) ||
		    whichreg_is_rd)
			break; /* found end-of-life */

		if (!regset_empty(regset_intersect(source_regset, REGS_READ(use))))
		{
			if (!operand_width_ok(use, source_regset, sourcereg_width, TRUE))
			{
				source_dead = FALSE;
				source_unchanged = FALSE;
			}
		}
			
		if ( (source_unchanged||source_dead) &&
		     !regset_empty(regset_intersect(source_regset, REGS_WRITTEN(use))) ){
			source_unchanged = FALSE;
			source_dead = FALSE;
		}
		if (comp_unchanged ||
		    !regset_empty(regset_intersect(comp_regset, REGS_WRITTEN(use))))
		{
			if (!operand_width_ok(use, comp_regset, compreg_width, FALSE)) return FALSE;
			comp_unchanged = FALSE;
		}
	}
	
	/* now that we've found the range, see if there is a target register */
	switch (OP_FUNC(use).f_group){
	case FG_ARITH:
	case FG_SETHI:
	case FG_RD:
	case FG_WR:
	case FG_FLOAT2:
	case FG_FLOAT3:
	case FG_LD:
		/* Get final_target_set. */
 	        final_target = use->n_operands.op_regs[O_RD];
		final_target_width = BYTES_TO_WORDS(use->n_opcp->o_func.f_rd_width);
		if (final_target_width == SN) final_target_width++;
		final_target_set = regset_mkset_multiple(final_target, compreg_width);

		if (CAN_COALESCE_INTO(final_target)){
			target_dead = !whichreg_is_rd;
			for ( p = np->n_next; p != use; p = p->n_next ){
				if ( (!regset_empty(regset_intersect(final_target_set, REGS_LIVE(p)))) ||
				     (!regset_empty(regset_intersect(final_target_set, REGS_WRITTEN(p)))) ){
					target_dead = FALSE;
					break;
				}
			}
			if ( target_dead &&
			     !regset_empty(regset_intersect(final_target_set, REGS_READ(use))) ){
				target_dead = FALSE;
			}
		} else {
			target_dead = FALSE;
	        }
		break;
	default:
		target_dead = FALSE;
		break;
	}


	if (target_dead && (comp_reg != final_target) && 
	    (compreg_width == final_target_width) &&
	    CAN_COALESCE_PAIR(final_target,comp_reg)){
	        sign = (final_target > comp_reg);
	        reg_offset = sign ? (final_target - comp_reg) : (comp_reg - final_target);
	        replace_reg_in_range(np, use, comp_regset, final_target_set,
				     ((whichreg==O_RS1)
				      ? OPT_COALESCE_RS1
				      : OPT_COALESCE_RS2), reg_offset, sign);
	} else if ( ( source_dead  || (source_unchanged && comp_unchanged)) &&
		    (comp_reg != source_reg) && (compreg_width == sourcereg_width) &&
		    CAN_COALESCE_PAIR(source_reg,comp_reg) && CAN_COALESCE_INTO(source_reg) ){
	        sign = (source_reg > comp_reg);
	        reg_offset = sign ? (source_reg - comp_reg) : (comp_reg - source_reg);
		replace_reg_in_range(np, use, comp_regset, source_regset,
				     ((whichreg==O_RS1)
				      ? OPT_COALESCE_RS1
				      : OPT_COALESCE_RS2), reg_offset, sign);
	} else	return FALSE;
	
	/* By the time we get here, all UNsuccessful attempts at coalescing
	 * have returned from the routine, with a FALSE value.  So, if we're
	 * here, we succeeded.
	 */

	if (whichreg == O_RS1)	optimizer_stats.coalesces_on_rs1++;
	else			optimizer_stats.coalesces_on_rs2++;

	return TRUE;
}

static Bool
null_effect( np, sccp )
	register Node *np;
	Bool *sccp;
{
	/* check for nops in disguise */
	/* looking for self-moves with no effect */
	if (!is_mov_equivalent(np)) return FALSE;

	*sccp = (OP_FUNC(np).f_setscc == TRUE);

	/*
	 * If we got here, then one of the two source operands is %g0
	 * (thanks to is_mov_equivalent() above).
	 * Check if the destination register is also %g0,
	 * or if the destinations register is the same register as the
	 * non-%g0 operand.  If so, then the instruction is effectively a nop.
	 */
	return ( (np->n_operands.op_regs[O_RD] == RN_G0)
				||
	         ( (np->n_operands.op_regs[O_RD] == np->n_operands.op_regs[O_RS1]) &&
		   !NODE_ACCESSES_FPU(np) )
				||
                 ( (np->n_operands.op_regs[O_RD] == np->n_operands.op_regs[O_RS2]) && 
		   NODE_ACCESSES_FPU(np) ) 
                                ||
	         ( (!np->n_operands.op_imm_used) &&
		   (np->n_operands.op_regs[O_RD]==np->n_operands.op_regs[O_RS2])
		 )
	       );
}

static void
out_with_the_old(np)
	register Node *np;
{
	/* Deal with any symbol references or value lists in a node.
	 * This should be done before hammering a node into a new operation.
	 */
	switch ( np->n_opcp->o_func.f_node_union )
	{
	case NU_OPS:
		delete_node_symbol_reference(np, np->n_operands.op_symp);
		np->n_operands.op_symp = NULL;
		break;
	case NU_VLHP:
		free_value_list(np->n_vlhp);
		break;
	}
}

static void
tst_it( np )
	register Node *np;
{
	/* Replace this node by a tst, let someone else deal with deleting. */
	/* We assume is_mov_equivalent(np) is TRUE, and that the only effect of
	 * the instruction is to set the condition codes (per null_effect()).
	 * So, turn it into a regular "tst".
	 */

	/* Gracefully clean out remnants of the old node. */
	out_with_the_old(np);

	(void) convert_tst_equivalent_into_real_tst(np);
	np->n_operands.op_regs[O_RD] = RN_G0;

	regset_setnode(np);
	regset_recompute(np);
}

void
nop_it( np )
	register Node *np;
{
	/* Replace this node by a nop; let someone else deal with deleting. */

	/* Gracefully clean out remnants of the old node. */
	out_with_the_old(np);

	np->n_opcp = opcode_ptr[OP_NOP];
	regset_setnode(np);
	regset_recompute(np);

	optimizer_stats.dead_arithmetic_deleted += 1;
}


/*
 * Note:
 * Although these routines will initially be applied to entire object routines,
 * the intent is that they be applicable to smaller regions, such as
 * basic blocks.
 */
Bool
opt_dead_arithmetic( np_first, np_last )
	Node *np_first, *np_last;
{
	register Node *np;
	Bool did_something = FALSE;
	Bool sets_cc;

	if (!do_opt[OPT_DEADARITH]) return FALSE;
	for ( np = np_last; np != np_first; np = np->n_prev ){
		/* this applies to arithmetic and register transfer only */
		switch (OP_FUNC(np).f_group){
		case FG_LD:
			if ( coalesce_registers(np, O_RS1) ||
			     coalesce_registers(np, O_RS2) ){
				did_something = TRUE;
			}
			break;
		case FG_FLOAT2:
		case FG_FLOAT3:
		case FG_ARITH:
			if (OP_FUNC(np).f_group == FG_ARITH)
			        switch (OP_FUNC(np).f_subgroup){
				case FSG_SAVE:
				case FSG_REST:
				       /* don't try to delete these! */
				       continue;
				case FSG_SUB:
				       if (is_extraneous_tst(np)){
					      /* useless */
					      /* but may be in a delay slot */
					      nop_it(np);
					      did_something = TRUE;
					      continue;
				       }
				       break;
			       }
			if ( coalesce_registers(np, O_RS1)  ||
			     coalesce_registers(np, O_RS2) ){
				did_something = TRUE;
			}
			if (null_effect(np, &sets_cc)){
				if (sets_cc && regset_in(following_liveset(np),RN_ICC)){
					/* rewrite as tst */
					tst_it(np);
				} else {
					/* rewrite as bona-fide nop */
					nop_it(np);
				}
				continue;
			}
			/* others FALL THROUGH */
		case FG_SETHI:
		case FG_RD:
		case FG_WR:
			if (regset_empty(regset_intersect(REGS_WRITTEN(np),
					    following_liveset(np)))){ 
					/* useless */
				nop_it(np); 
				did_something = TRUE;
			} 
			break;
		}
	}
	return did_something;
}
