#ifndef lint
static char sccs_id[] = "@(#)opt_patterns.c 1.1     94/10/31";
#endif
 
/*
 * This file contains code which compacts sequences of the form
 * 
 *      inc(dec)  reg
 *      tst       reg
 *      blt(bge) 
 *
 * into qequences of the form 
 * 
 *      inc(dec)cc  reg  
 *      bpos(bneg) 
 * 
 * during SPARC code optimization.  
 */
 
#include <stdio.h>
#include "structs.h"
#include "bitmacros.h"
#include "registers.h"
#include "optimize.h"
#include "opt_utilities.h"
#include "opt_regsets.h" 
#include "opt_regsets.h"
#include "opt_statistics.h"
#include "sym.h"
#include "opcode_ptrs.h"

 
void
find_and_replace_inc_tst_bcc(inc_np)  
        register Node *inc_np;
{
        register Node *tst_np, *bcc_np; 
	Bool tst_found = FALSE;
	Bool bcc_found = TRUE;
	Bool keep_looking = TRUE;
	Regno inc_rd = inc_np->n_operands.op_regs[O_RD];
 

	/* Look for the tst node first */
	for (tst_np = inc_np->n_next; ; tst_np = tst_np->n_next){
		
		switch (OP_FUNC(tst_np).f_group){
			case FG_MARKER:
			case FG_LABEL:
			case FG_BR:
			case FG_CALL:
			case FG_JMPL:
			case FG_RET:
			case FG_JMP:
				keep_looking = FALSE;
				break;
		        default:
				if ( (OP_FUNC(tst_np).f_subgroup == FSG_OR)  &&
		     		     (NODE_SETSCC(tst_np)) &&
		     		     (tst_np->n_operands.op_regs[O_RS1] == RN_G0)  &&
		                     (tst_np->n_operands.op_regs[O_RD] == RN_G0) &&
				     (tst_np->n_operands.op_regs[O_RS2] == inc_rd) ) {
					tst_found = TRUE; 
					keep_looking = FALSE;
				} else if ( NODE_SETSCC(tst_np) ||
					    (regset_in(REGS_WRITTEN(tst_np), inc_rd)) ){ 
					keep_looking = FALSE;
				}	
				break;
		}
		if (!keep_looking) break;
	}

	/*  Look for the bcc node next */
	if (tst_found) {
		keep_looking = TRUE;
		for (bcc_np = tst_np->n_next; keep_looking && !NODE_IS_BRANCH(bcc_np) ; bcc_np = bcc_np->n_next){
			switch (OP_FUNC(bcc_np).f_group){
                        	case FG_MARKER:  
                       	 	case FG_LABEL:
                        	case FG_CALL:
                        	case FG_JMPL:
                        	case FG_RET:
                       		case FG_JMP:
					bcc_found = FALSE;
                                	keep_looking = FALSE;
				default:
					break;
			}
		}

		/*  The node is a branch.  Now test for the 
		 *  right kind of branch. 
		 */
		if ( bcc_found &&
		     ((OP_FUNC(bcc_np).f_subgroup == FSG_L) ||
		      (OP_FUNC(bcc_np).f_subgroup == FSG_GE)) ) {
			if (OP_FUNC(bcc_np).f_subgroup == FSG_L){
				
				/* OK. Change the branch condition */
				if (BRANCH_IS_ANNULLED(bcc_np))
					bcc_np->n_opcp = opc_lookup("bneg,a");
				else 
					bcc_np->n_opcp = opcode_ptr[OP_BNEG];
			} 
			else if (OP_FUNC(bcc_np).f_subgroup == FSG_GE){
				
				/* OK. Change the branch condition */
				if (BRANCH_IS_ANNULLED(bcc_np))
					bcc_np->n_opcp = opc_lookup("bpos,a");
				else 
					bcc_np->n_opcp = opcode_ptr[OP_BPOS];
		        }
				/* OK. Get rid of the tst and make the
				 * inc(dec) into and inc(dec)cc.
				 */
				nop_it(tst_np);
			        if (OP_FUNC(inc_np).f_subgroup == FSG_ADD)
				        inc_np->n_opcp = opcode_ptr[OP_ADDCC];
			        else 
				        inc_np->n_opcp = opcode_ptr[OP_SUBCC];

				optimizer_stats.inc_tst_bcc_compacted++;
			        regset_setnode(inc_np);
			        regset_setnode(bcc_np);
			        regset_recompute(bcc_np);
		}
	      
	}
}

void
replace_sll_sra(sll_np)
	register Node *sll_np;
{
	register Node *sra_np;
	register Node *store_np;
	Bool keep_looking = TRUE;
	Bool sra_found = FALSE;	
	Regno sll_rd = sll_np->n_operands.op_regs[O_RD];


		/* Look for the sra node first */
	for (sra_np = sll_np->n_next;  ; sra_np = sra_np->n_next){
		switch (OP_FUNC(sra_np).f_group){
			case FG_MARKER:
			case FG_LABEL:
			case FG_BR:
			case FG_CALL:
			case FG_JMPL:
			case FG_RET:
			case FG_JMP:
				keep_looking = FALSE;
				break;
			case FG_ARITH:
				if ( (OP_FUNC(sra_np).f_subgroup == FSG_SRA) &&
		     		     (sra_np->n_operands.op_imm_used)  &&
		                     (sra_np->n_operands.op_addend ==
				      sll_np->n_operands.op_addend) &&
				     (sra_np->n_operands.op_regs[O_RS1] ==
				      sll_rd) ) {

					sra_found = TRUE; 
					keep_looking = FALSE;
					break;
				}
			default: 
				if( (regset_in(REGS_READ(sra_np), sll_rd)) ||
				    (regset_in(REGS_WRITTEN(sra_np), sll_rd)) ){
					keep_looking = FALSE;
					break;
				}
		}
		if (!keep_looking) break;  /* out of loop */
	}

	/*  Look for the store node next and delete the sll and sra
	 * if the conditions are met.
	 */
	if (sra_found) {
		keep_looking = TRUE;
		for (store_np = sra_np->n_next; ; store_np = store_np->n_next){
			switch (OP_FUNC(store_np).f_group){
                        	case FG_MARKER:  
                       	 	case FG_LABEL:
				case FG_BR:
                        	case FG_CALL:
                        	case FG_JMPL:
                        	case FG_RET:
                       		case FG_JMP:
                                	keep_looking = FALSE;
					break;
				case FG_ST:
					if ( (store_np->n_operands.op_regs[O_RD] ==
					      sra_np->n_operands.op_regs[O_RD]) &&
					     (!regset_in(following_liveset(store_np),sll_rd)) &&
					     (!regset_in(following_liveset(store_np),sra_np->n_operands.op_regs[O_RD])) &&
					     ( ((store_np->n_opcp == opcode_ptr[OP_STH]) &&
						HALFW_OFFSET(sll_np->n_operands.op_addend)) ||
					       ((store_np->n_opcp == opcode_ptr[OP_STB]) &&
						BYTE_OFFSET(sll_np->n_operands.op_addend)) ) ) {
					        store_np->n_operands.op_regs[O_RD] =
						sll_np->n_operands.op_regs[O_RS1];
						delete_node(sll_np);
						delete_node(sra_np);
						regset_setnode(store_np);
						regset_recompute(store_np);
						optimizer_stats.sll_sra_deleted++;
						keep_looking = FALSE;
						break;
					}
				default:
					if( regset_in(REGS_READ(store_np), sll_rd) ||
					    regset_in(REGS_WRITTEN(store_np), sll_rd) ||
					    regset_in(REGS_READ(store_np), sra_np->n_operands.op_regs[O_RD]) ||
					    regset_in(REGS_WRITTEN(store_np),sra_np->n_operands.op_regs[O_RD]) ){

						keep_looking = FALSE;
						break;
					}
			}
			if (!keep_looking) break; /* out of loop */
		}
	}
}

