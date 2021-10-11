static char sccs_id[] = "@(#)opt_statistics.c	1.1\t10/31/94";
/*
 * This file contains code which keeps and reports statistics on SPARC
 * code optimizations.
 */

#include <stdio.h>
#include "structs.h"
#include "globals.h"
#include "opt_utilities.h"
#include "opt_statistics.h"

struct Optimizer_stats optimizer_stats = { 0 /*,0...*/ };


static unsigned long int
percent_ratio(num, denom)
	register unsigned long int num;
	register unsigned long int denom;
{
	if (denom == 0)	return 0;
	else		return ((num * 1000) + 5) / (denom * 10);
}


static void
report_cti_fill_ratio(marker_np)
	register Node *marker_np;
{
	register Node *np;
	register unsigned long int cti_count = 0;
	register unsigned long int cti_actual_fill_count = 0;
#ifdef CHIPFABS
	register unsigned long int cti_possible_fill_count = 0;
#endif /*CHIPFABS*/

	for (np = marker_np->n_next;   np != marker_np;   np = np->n_next)
	{
		if ( NODE_HAS_DELAY_SLOT(np) )
		{
			cti_count++;
			if ( NODE_IS_NOP(np->n_next) )
			{
#ifdef CHIPFABS
				if ( (iu_fabno == 1)
						&&
				     (
				       /* the previous instruction was a LD */
				       (np->n_prev->n_opcp->o_func.f_group
						== FG_LD) ||
				       /* the instruction after the target
					* label was a LD.
					*/
				       ( NODE_IS_BRANCH(np) &&
				         BRANCH_IS_UNCONDITIONAL(np) &&
				         (next_instruction(np->n_operands.op_symp->s_def_node)
						->n_opcp->o_func.f_group
						== FG_LD
					 )
				       )
				     )
				   )
				{
					cti_possible_fill_count++;
				}
#endif /*CHIPFABS*/
			}
			else
			{
#ifdef CHIPFABS
				cti_possible_fill_count++;
#endif /*CHIPFABS*/
				cti_actual_fill_count++;
			}
		}
	}

	fprintf(stderr, "[opt:%4ld%% (%lu/%lu) delay slots filled",
		percent_ratio(cti_actual_fill_count, cti_count),
		cti_actual_fill_count, cti_count);
#ifdef CHIPFABS
	if (iu_fabno == 1)
	{
		fprintf(stderr,
		    "\n      (%2ld%% (%lu/%lu) possible for non-fab#1 IU chip)",
		    percent_ratio(cti_possible_fill_count, cti_count),
		    cti_possible_fill_count, cti_count);
	}
#endif /*CHIPFABS*/
	fputs("]\n", stderr);
}


void
report_optimizer_stats(marker_np)
	register Node *marker_np;
{
	fprintf(stderr,"[opt:%4ld redundant labels deleted]\n",
		optimizer_stats.redundant_labels_deleted);
	fprintf(stderr,"[opt:%4ld unreferenced labels deleted]\n",
		optimizer_stats.unreferenced_labels_deleted);
	fprintf(stderr,
		"[opt:%4ld nodes deleted as dead code after unconditional branches]\n",
		optimizer_stats.dead_code_deleted_after_uncond_CTIs);
	fprintf(stderr,
		"[opt:%4ld unreachable loops to self deleted (%ld nodes deleted)]\n",
		optimizer_stats.dead_loops_deleted,
		2*optimizer_stats.dead_loops_deleted);
	fprintf(stderr,"[opt:%4ld branches to branches simplified]\n",
		optimizer_stats.branches_to_branches_simplified);
	fprintf(stderr,"[opt:%4ld trivial branches deleted]\n",
		optimizer_stats.trivial_branches_deleted);
	fprintf(stderr,
		"[opt:%4ld function prologues compacted (%ld nodes deleted)]\n",
		optimizer_stats.function_prologues_compacted,
		2*optimizer_stats.function_prologues_compacted);
	fprintf(stderr,
		"[opt:%4ld function epilogues compacted (%ld nodes deleted)]\n",
		optimizer_stats.function_epilogues_compacted,
		optimizer_stats.function_epilogues_compacted);
	fprintf(stderr,
		"[opt:%4ld functions made into leaf routines (%2d%%)]\n",
		optimizer_stats.leaf_routines_made,
		percent_ratio(optimizer_stats.leaf_routines_made,
			      optimizer_stats.procedures_optimized) );
	fprintf(stderr,
		"[opt:%4ld branch targets moved, replacing branch (%ld nodes deleted)]\n",
		optimizer_stats.branch_targets_moved,
		optimizer_stats.branch_targets_moved_nodes_deleted);
	fprintf(stderr,"[opt:%4ld useless NOP's deleted]\n",
		optimizer_stats.useless_NOPs_deleted);
	fprintf(stderr,
		"[opt:%4ld useless branches around branches deleted (%ld nodes deleted)]\n",
		optimizer_stats.branches_around_branches_deleted_1,
		3*optimizer_stats.branches_around_branches_deleted_1);
	fprintf(stderr,
		"[opt:%4ld branches around branches deleted (%ld nodes deleted)]\n",
		optimizer_stats.branches_around_branches_deleted_2,
		2*optimizer_stats.branches_around_branches_deleted_2);
	fprintf(stderr,
		"[opt:%4ld branches around branches deleted, annul used (%ld nodes deleted)]\n",
		optimizer_stats.branches_around_branches_deleted_3,
		2*optimizer_stats.branches_around_branches_deleted_3);
	fprintf(stderr,
		"[opt:%4ld instructions deleted by simplifying annulled conditional branches]\n",
		optimizer_stats.conditional_annuls_simplified);
	fprintf(stderr,
		"[opt:%4ld branches to earlier targets successful (%ld nodes deleted)]\n",
		optimizer_stats.branches_sooner_successful,
		optimizer_stats.branches_sooner_nodes_deleted);
	fprintf(stderr,"[opt:%4ld loops inverted]\n",
		optimizer_stats.loops_inverted);
	fprintf(stderr,"[opt:%4ld useless arithmetic instructions deleted]\n",
		optimizer_stats.dead_arithmetic_deleted);
	fprintf(stderr,"[opt:%4ld sethi instructions deleted]\n",
		optimizer_stats.sethi_eliminated);
	fprintf(stderr,"[opt:%4ld coalesces on register RS1]\n",
		optimizer_stats.coalesces_on_rs1);
	fprintf(stderr,"[opt:%4ld coalesces on register RS2]\n",
		optimizer_stats.coalesces_on_rs2);
        fprintf(stderr,"[opt:%4ld cmp with known outcome (%ld nodes deleted)]\n",
                optimizer_stats.cmp_outcome_known_succesful,
		optimizer_stats.cmp_outcome_known_nodes_deleted);
        fprintf(stderr,"[opt:%4ld inc(dec),tst,bcc to inc(dec)cc,bcc]\n", 
                optimizer_stats.inc_tst_bcc_compacted); 
        fprintf(stderr,"[opt:%4ld sll, sra deleted]\n", 
                optimizer_stats.sll_sra_deleted); 

	/**putc('\n', stderr);**/
	report_cti_fill_ratio(marker_np);


	fprintf(stderr,"[opt:%4ld procedures optimized]\n",
		optimizer_stats.procedures_optimized);
	fprintf(stderr,"[opt:%4ld pre-procedures optimized]\n",
		optimizer_stats.pre_procedures_optimized);
	fprintf(stderr,"[opt:%4ld iterations of optimizer main  loop (max %ld per procedure)]\n",
		optimizer_stats.optimizer_main_loop_count,
		optimizer_stats.max_main_loops_per_proc);
	fprintf(stderr,"[opt:%4ld iterations of optimizer inner loop (max %ld per procedure)]\n",
		optimizer_stats.optimizer_inner_loop_count,
		optimizer_stats.max_inner_loops_per_proc);
}
