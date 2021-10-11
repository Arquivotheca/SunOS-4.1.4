/*	@(#)check_nodes.h	1.1	10/31/94	*/

extern	Bool	 node_is_in_delay_slot();
extern	void	 check_list_for_errors();
extern	void	 append_a_NOP();

#ifdef CHIPFABS
extern	void	 setup_fab_opcodes();
extern	void	 perform_fab_workarounds();
#endif /*CHIPFABS*/
