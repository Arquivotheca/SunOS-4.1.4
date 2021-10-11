static char sccs_id[] = "@(#)opt_fcn_epilogue.c	1.1\t10/31/94";
/*
 * This file contains code which compacts function epilogues during SPARC
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
compact_function_epilogue(restore_np)
	register Node *restore_np;
{
	/* (ASSUME structure of Function Epilogue here)
	 *
	 *	mov	<blah>,<ireg>		<-- mov_np
	 *	ret				<-- ret_np
	 *	restore				<-- restore_np
	 *
	 * We can reduce the above to:
	 *	
	 *	ret				<-- ret_np
	 *	restore	%g0,<blah>,<ireg-16>	<-- restore_np
	 */
	register Node *mov_np, *ret_np;

	if ( !do_opt[OPT_EPILOGUE] )	return;

	ret_np = restore_np->n_prev;	/* the "ret" node?	*/
	mov_np = ret_np->n_prev;	/* the "mov" node?	*/

	if (
	     ( /* It's a MOV instruction: OR, with RS1 == %g0 */
	       (mov_np->n_opcp->o_func.f_group    == FG_ARITH) &&
	       (mov_np->n_opcp->o_func.f_subgroup == FSG_OR)   &&
	       (mov_np->n_operands.op_regs[O_RS1] == RN_GP(0)) &&
	       /* ...and the Rd register is an IN register */
	       ( IS_GP_IN_REG(mov_np->n_operands.op_regs[O_RD]) ) &&
	       /* ...and no funny relocation modes are being used */
	       ( ( mov_np->n_operands.op_imm_used &&
	           (mov_np->n_operands.op_reltype == REL_13)
		 ) ||
	         ( (!mov_np->n_operands.op_imm_used) &&
	           (mov_np->n_operands.op_reltype == REL_NONE)
		 )
	       )
	     )
				&&
	     /* It's a RET instruction */
	     (ret_np->n_opcp->o_func.f_group == FG_RET)
				&&
	     /* It's a trivial RESTORE instruction  */
	     (!restore_np->n_operands.op_imm_used) &&
	     ( (restore_np->n_operands.op_regs[O_RS1] == RN_GP(0)) &&
	       (restore_np->n_operands.op_regs[O_RS2] == RN_GP(0)) &&
	       (restore_np->n_operands.op_regs[O_RD ] == RN_GP(0)) )
	   )
	{
		/* OK, the pattern matches!
		 * Now, reduce it by moving the "mov" operation into the
		 * "restore" node.
		 * The easiest way to do that is to change the "mov" node
		 * into a "restore" node, delete the old "restore" node,
		 * and move the converted "mov" node to after the "ret" node.
		 */

		/* Convert MOV to RESTORE */
		mov_np->n_opcp = restore_np->n_opcp;

		/* Convert IN register number to OUT register number (since
		 * RESTORE's RD is in the next window, not the current one).
		 */
		mov_np->n_operands.op_regs[O_RD] +=
			( RN_GP_OUT(0) - RN_GP_IN(0) );

		/* Delete the old RESTORE node. */
		delete_node(restore_np);

		/* Move the newly-converted RESTORE into place after RET. */
		make_orphan_node(mov_np);
		insert_after_node(ret_np, mov_np);

		optimizer_stats.function_epilogues_compacted++;

		/* Make sure register-usage information is left OK. */
		regset_setnode(mov_np);
		regset_recompute(mov_np);
	}
}
