/*	@(#)opt_statistics.h	1.1	10/31/94	*/

struct Optimizer_stats
{
	unsigned long int optimizer_main_loop_count;
	unsigned long int optimizer_inner_loop_count;
	unsigned long int max_main_loops_per_proc;
	unsigned long int max_inner_loops_per_proc;
	unsigned long int pre_procedures_optimized;
	unsigned long int procedures_optimized;

	unsigned long int redundant_labels_deleted;
	unsigned long int unreferenced_labels_deleted;
	unsigned long int dead_code_deleted_after_uncond_CTIs;
	unsigned long int dead_loops_deleted;		/* x2 for # of nodes*/
	unsigned long int branches_to_branches_simplified;
	unsigned long int trivial_branches_deleted;
	unsigned long int function_prologues_compacted; /*x2 for nodes deleted*/
	unsigned long int function_epilogues_compacted;
	unsigned long int leaf_routines_made;
	unsigned long int branch_targets_moved;
	unsigned long int branch_targets_moved_nodes_deleted;
	unsigned long int useless_NOPs_deleted;
	unsigned long int branches_around_branches_deleted_1;
	unsigned long int branches_around_branches_deleted_2;
	unsigned long int branches_around_branches_deleted_3;
	unsigned long int conditional_annuls_simplified;
	unsigned long int branches_sooner_successful;
	unsigned long int branches_sooner_nodes_deleted;
	unsigned long int loops_inverted;
	unsigned long int dead_arithmetic_deleted;
	unsigned long int sethi_eliminated;
	unsigned long int coalesces_on_rs1;
	unsigned long int coalesces_on_rs2;
	unsigned long int cmp_outcome_known_succesful;
        unsigned long int cmp_outcome_known_nodes_deleted;
	unsigned long int inc_tst_bcc_compacted;
	unsigned long int sll_sra_deleted;
};

extern	struct	Optimizer_stats optimizer_stats;

extern	void	report_optimizer_stats();
