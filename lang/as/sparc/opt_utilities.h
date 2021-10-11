/*	@(#)opt_utilities.h	1.1	10/31/94	*/

extern	Bool		is_jmp_at_top_of_cswitch();
extern	Node	       *previous_code_generating_node();
extern	Node	       *previous_code_generating_node_or_label();
extern	Node	       *next_code_generating_node();
extern	Node	       *next_code_generating_node_or_label();
extern	Node	       *next_instruction();
extern	Node	       *previous_instruction_or_label_node();
extern	Node	       *next_non_directive();
extern	Bool		opt_node_is_in_delay_slot();
extern	Bool		is_an_unconditional_control_transfer();
extern	Bool		can_fall_into();
extern	void		append_block_after_node();
extern	Node	       *new_optimizer_NOP_node();
extern  struct opcode  *reverse_condition_opcode();
extern	void		change_branch_target();
extern	struct opcode  *branch_with_toggled_annul_opcp();
extern	Bool		node_depends_on_register();
extern	Bool		instr1_depends_on_instr2();
extern	Bool		instrs_are_mutually_independent();
extern	Bool		branch_is_last_reference_to_target_label();
extern	Bool		nodes_are_equivalent();
extern	void		copy_node();

#define cannot_fall_into(np)	(!can_fall_into(np))
#define BRANCH_IS_UNCONDITIONAL(np)  ((np)->n_opcp->o_func.f_subgroup == FSG_A)
#define BRANCH_IS_CONDITIONAL(np)    ((np)->n_opcp->o_func.f_subgroup != FSG_A)
#define BRANCH_IS_ANNULLED(np)	     ((np)->n_opcp->o_func.f_br_annul)
#define BRANCH_IS_NOT_ANNULLED(np)   (!BRANCH_IS_ANNULLED(np))

#define DELAY_SLOT_WILL_BE_EXECUTED_IF_TAKEN(np)	\
		(!BRANCH_IS_ANNULLED(np) || BRANCH_IS_CONDITIONAL(np))
#define DELAY_SLOT_WILL_BE_EXECUTED_IF_UNTAKEN(np)	\
		(!BRANCH_IS_ANNULLED(np) || BRANCH_IS_UNCONDITIONAL(np))
#define DELAY_SLOT_WILL_ALWAYS_BE_EXECUTED(np)	\
		(!BRANCH_IS_ANNULLED(np))
#define DELAY_SLOT_WILL_NEVER_BE_EXECUTED(np)	\
		(BRANCH_IS_ANNULLED(np) && BRANCH_IS_UNCONDITIONAL(np))
#define DELAY_SLOT_MAY_BE_EXECUTED(np)	\
		(!DELAY_SLOT_WILL_NEVER_BE_EXECUTED(np))

#ifdef DEBUG
  extern Node *opt_new_node();
#else /*DEBUG*/
#  define opt_new_node(lineno, opcp, optno)	new_node(lineno, opcp)
#endif /*DEBUG*/
