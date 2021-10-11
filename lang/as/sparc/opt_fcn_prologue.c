static char sccs_id[] = "@(#)opt_fcn_prologue.c	1.1\t10/31/94";
/*
 * This file contains code which compacts function prologues during SPARC
 * code optimization.
 */

#include <stdio.h>
#include "structs.h"
#include "bitmacros.h"
#include "registers.h"
#include "optimize.h"
#include "opt_utilities.h"
#include "opt_regsets.h"
#include "opt_statistics.h"


void
compact_function_prologue(save_np)
	register Node *save_np;
{
	/* (ASSUME structure of Function Prologue here)
	 *
	 *      !#PROLOGUE# 0	 !(optional)
	 *	sethi	%hi(LABEL),%g1		<-- sethi_np
	 *	add     %g1,%lo(LABEL),%g1	<-- add_np
	 *	save    %sp,%g1,%sp		<-- save_np
	 *      !#PROLOGUE# 1	 !(optional)
	 *
	 * and if the value of LABEL is >= -4096 and <= 4095, we can reduce
	 * the above to:
	 *      !#PROLOGUE# 0	 !(optional)
	 *	save    %sp,LABEL,%sp		<-- save_np
	 *      !#PROLOGUE# 1	 !(optional)
	 */
	register Node *add_np, *sethi_np;

	if ( !do_opt[OPT_PROLOGUE] )	return;
#ifdef DEBUG
	if ( ! ((save_np->n_opcp->o_func.f_group    == FG_ARITH) &&
	        (save_np->n_opcp->o_func.f_subgroup == FSG_SAVE))
	   ) internal_error("compact_function_prologue(): save_np!=SAVE?");
#endif /*DEBUG*/

	add_np   = save_np->n_prev;	/* the "add" node?	*/
	sethi_np = add_np->n_prev;	/* the "sethi" node?	*/

	if (
	     ( (sethi_np->n_opcp->o_func.f_group == FG_SETHI) &&
	       (sethi_np->n_operands.op_reltype == REL_HI22) )
				&&
	     ( (add_np->n_opcp->o_func.f_group    == FG_ARITH) &&
	       (add_np->n_opcp->o_func.f_subgroup == FSG_ADD) &&
	       (add_np->n_operands.op_reltype == REL_LO10) )
				&&
	     (sethi_np->n_operands.op_symp == add_np->n_operands.op_symp)
				&&
	     ( (((long)(sethi_np->n_operands.op_symp->s_value)) >= -4096) &&
	       (((long)(sethi_np->n_operands.op_symp->s_value)) <= 4095) )

	     /* if we REALLY wanted to check it out, we could also check all
	      * of the register numbers here to make sure they matched...
	      */
	   )
	{
#ifdef DEBUG
		if ( !SYMBOL_IS_ABSOLUTE_VALUE(sethi_np->n_operands.op_symp) )
		{
			internal_error(
				"compact_function_prologue(): label in seg %d?",
				(int)(sethi_np->n_operands.op_symp->s_segment) );
		}
#endif /*DEBUG*/
		/* OK, the pattern matches!
		 * Now, reduce it by moving the symbol value into the SAVE node.
		 */
		save_np->n_operands.op_imm_used    = TRUE;
		save_np->n_operands.op_regs[O_RS2] = RN_NONE;

		/* Dropping the reference to the symbol should be safe here,
		 * since function prologues only reference ABSOLUTE (equated)
		 * symbol values.
		 * And since the input pass is long done before we get here,
		 * we can just grab the value the symbol was set to.
		 * (If it might be a relocatable value, then it wouldn't have
		 *  a value assigned yet and we'd have to retain the symbol
		 *  reference and let the linker fill in the value)
		 */
		save_np->n_operands.op_addend  =
					sethi_np->n_operands.op_symp->s_value;
		save_np->n_operands.op_reltype = REL_13;

		/* And delete the old SETHI and ADD nodes.
		 * Note that if the SETHI and ADD nodes were the only ones
		 * which referenced the label, that label's defining node will
		 * also be deleted during the second "delete_node()".
		 */
		delete_node(sethi_np);
		delete_node(add_np);

		/* Make sure register-usage information is left OK. */
		regset_recompute(save_np);

		optimizer_stats.function_prologues_compacted++;
	}
}
