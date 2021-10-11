/*	@(#)opt_progstruct.h	1.1	94/10/31	Copyr 1986 Sun Microsystems */

/*
 * routines and structures for discovering and manipulating the program
 * structure.
 */

/*
 * basic block: one-entry, one-exit
 * since the nodes are already threaded together, there
 * is no need to have a membership list as for more general intervals
 */
struct basic_block {
	Node *	bb_dominator;	/* entry */
	Node *  bb_trailer;	/* last block */
	int	bb_npred;	/* # of predecessors */
	struct basic_block **
		bb_predlist;	/* list of predecessors */
	int	bb_nsucc;	/* # of successors */
	struct basic_block **
		bb_succlist;	/* list of successors */
	struct basic_block *
		bb_chain;	/* linked list of them */
	int	bb_name;	/* "name" of the node for debug printing */
	struct context *
		bb_context;	/* all the interesting stuff about this block */

};

struct basic_block * make_blocklist ( /* Node * marker_np */ );
void 		     print_blocklist( /* struct basic_block *bbp */ ); /* debug only */
void		     free_blocklist ( /* struct basic_block *bbp */ );
