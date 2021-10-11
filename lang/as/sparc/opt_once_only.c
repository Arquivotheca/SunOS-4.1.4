static char sccs_id[] = "@(#)opt_once_only.c	1.1\t10/31/94";
/*
 * This file contains code which performs SPARC code optimizations
 * which are only done once, i.e. "once-only" optimizations.
 * Also sinc this stuff will always execute before scheduling we will
 * use the same pass through the input to clean the ld2/st2 and fmovd
 * (that means turning them into pairs of ld/st or fmovs).
 */

#include <stdio.h>
#include "structs.h"
#include "node.h"
#include "optimize.h"
#include "opcode_ptrs.h"
#include "sym.h"
#include "opt_fcn_epilogue.h"
#include "opt_fcn_prologue.h"
#include "opt_patterns.h"
#include "opt_utilities.h"
#include "opt_regsets.h"

void
once_only_optimizations(marker_np)
	register Node *marker_np;
{
	register Node *np;
	register Node *next_np;
	register Node *np2;

	/* Make a pass through, doing optimizations which only need to be done
	 * once.
	 */
	for ( np = marker_np->n_next;
	      next_np = np->n_next, np != marker_np;
	      np = next_np
	    )
	{
		switch (np->n_opcp->o_func.f_group)
		{
		      case FG_EQUATE:
			/* If there are no references to an Equate, it's OK
			 * to delete it.
			 */
			if (np->n_operands.op_symp->s_ref_count == 0)
			{
				delete_node(np);
			}
			break;
			
		      case FG_ARITH:
			switch ( np->n_opcp->o_func.f_subgroup )
			{
			case FSG_SAVE:
				compact_function_prologue(np);
				break;
			case FSG_REST:
				compact_function_epilogue(np);
				break;
			case FSG_ADD:
			case FSG_SUB:
				if ( ( np->n_operands.op_regs[O_RD] ==
                        	       np->n_operands.op_regs[O_RS1] ) &&
                          	     (np->n_operands.op_symp == NULL) )
					find_and_replace_inc_tst_bcc(np);
				break;
			case FSG_SLL:
				if ( np->n_operands.op_imm_used &&
				     BYTE_OR_HW_OFFSET(np->n_operands.op_addend) ){
				        np = np->n_prev;
					replace_sll_sra(np->n_next);
					
					if ((np->n_next->n_next != next_np) &&
					    (np->n_next != next_np))
					        next_np = np->n_next;
				}
				break;
			}
			break;

		      /* This stuff turns ld2, st2 pseudo-instructions into pairs
		       * of ld's and st's.
		       */
		
		      case FG_ST:
		      case FG_LD:
			if (np->n_opcp->o_binfmt == BF_LDST2) {
				if (*np->n_opcp->o_name == '*') {
					if (OP_FUNC(np).f_group == FG_LD)
						np->n_opcp = opcode_ptr[OP_LDF];
					else 
						np->n_opcp = opcode_ptr[OP_STF];
				} else {
					if (OP_FUNC(np).f_group == FG_LD)
						np->n_opcp = opc_lookup("ld");
					else
						np->n_opcp = opcode_ptr[OP_ST];
				}
				np2 = new_node(np->n_lineno, np->n_opcp);
				copy_node(np, np2, OPT_NONE);
				np2->n_operands.op_regs[O_RD]++;     /* Change the register */
				if (np2->n_operands.op_imm_used)     /* Change the address  */
					np2->n_operands.op_addend += 4;
				else {
					np2->n_operands.op_addend = 4;
					np2->n_operands.op_imm_used = TRUE;
					np2->n_operands.op_reltype = REL_13;
				}
				if (np->n_operands.op_regs[O_RS1] == np->n_operands.op_regs[O_RD])
					insert_before_node(np, np2);
				else
					insert_after_node(np, np2);
				regset_setnode(np);
				regset_setnode(np2);
				regset_recompute(np2);
				next_np = np2->n_next;
			}
			break;
			
		      /* This stuff turns fmovd fnegd fabsd pseudo-instructions
		       * into the corresponding pairs of instructions.
		       * Handling for xtended pseudo-ins should be added when the
		       * optimizer is taught to handle xtended instructions in
		       * general.
		       */
		      case FG_FLOAT2:
			if ( np->n_opcp->o_binfmt == BF_FMOV2 ) 
				np->n_opcp = opcode_ptr[OP_FMOVS];
			else if ( np->n_opcp == opc_lookup("fabsd") )
				np->n_opcp = opc_lookup("fabss");
			else if ( np->n_opcp == opc_lookup("fnegd") )
				np->n_opcp = opc_lookup("fnegs");
			else break;
			if (np->n_operands.op_regs[O_RS2] != np->n_operands.op_regs[O_RD]) {
				np2 = new_node(np->n_lineno, np->n_opcp);
				copy_node(np, np2, OPT_NONE);
				np2->n_opcp = opcode_ptr[OP_FMOVS]; /* To make sure */
				np2->n_operands.op_regs[O_RS2]++; /* Change the regs */
				np2->n_operands.op_regs[O_RD]++;  
				if (np->n_operands.op_regs[O_RS2] == 
				    (np->n_operands.op_regs[O_RD] - 1))
					insert_after_node(np, np2);
				else
					insert_before_node(np, np2);
				regset_setnode(np);
				regset_setnode(np2);
				regset_recompute(np2);
				next_np = np2->n_next;
			}
			break;
		}
	}
#ifdef DEBUG
	opt_dump_nodes(marker_np, "after once_only_optimizations()");
#endif /*DEBUG*/
}
