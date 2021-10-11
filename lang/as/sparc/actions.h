/*	@(#)actions.h	1.1	10/31/94	*/

/* actions_misc.c */
extern	Node		*eval_equate();
extern	Node		*eval_comment();
extern	unsigned int	 eval_asi();
extern	Node		*create_label();
extern	Node		*eval_label();
extern	struct operands *eval_RpR();
extern	struct operands *eval_RpC();
extern	struct operands *eval_REG_to_ADDR();
extern  struct operands *get_operands13();

/* actions_util.c */
extern  void		chk_absolute();
extern  Bool		chk_redefn();
extern  void		abs_error();
extern  unsigned int	get_regno();
extern  void		define_symbol();
extern  void		convert_to_fp();
extern  void		ps_reserve();
/*** extern  void	abbr_ps_reserve(); ***/
extern  void		ps_common();

/* actions_instr.c */
extern	Node *	eval_opc0ops();
extern	Node *	eval_ldst();
extern	Node *	eval_sfa_ldst();
extern	Node *	eval_ldsta();
extern	Node *	eval_arithNOP();
extern	Node *	eval_arithRR();
extern	Node *	eval_arithRRR();
extern	Node *	eval_arithRCR();
extern	Node *	eval_arithCRR();
extern	Node *	eval_3ops();
extern  Node *  eval_4ops();
extern	Node *	eval_ER();
extern	Node *	eval_EE();
extern	Node *	eval_2opsSfaR();
extern	Node *	eval_RR();
extern	Node *	eval_2opsR();
extern	Node *	eval_2opsRE();
extern	Node *	eval_1opsNOARG();
extern	Node *	eval_1opsA();
extern	Node *	eval_call_AE();
extern	Node *	eval_j_ARE();
extern	Node *	eval_j_RRE();
extern	Node *	eval_j_ERE();
extern	Node *	eval_1opsR();
extern	Node *	eval_call_RE();
extern	Node *	eval_1opsE();
extern	Node *	eval_clr();
extern	Node *	eval_sfa_clr();
extern	Node *	eval_pseudo();

/* actions_expr */
extern	struct val_list_head *eval_elist();
extern	struct value	*eval_hilo();
extern	struct value	*symbol_to_value();
extern	struct value	*num_to_string();
extern	struct value	*eval_add();
extern	struct value	*eval_bin();
extern	struct value	*eval_bitnot();
extern	struct value	*eval_uadd();
