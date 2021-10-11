/*      @(#)misc.h 1.1 94/10/31 SMI      */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __misc__
#define __misc__

/* free lists for blocks and triples */
extern TRIPLE	*free_triple_lifo;
extern BLOCK	*free_block_lifo;

/* type structures for standard IR types */
extern TYPE	inttype;
extern TYPE     floattype;
extern TYPE     doubletype;
extern TYPE	undeftype;

/* print warning message with up to 6 arguments, unless -w is specified */
extern void warning(/*tp, msg, a1,a2,a3,a4,a5,a6*/);

/* print message without arguments and abort */
extern void quit(/*msg*/);

/* print message with up to 6 arguments and abort */
extern void quita(/*msg, a1,a2,a3,a4,a5,a6*/);

/* round up a to a multiple of b */
extern unsigned roundup(/*a,b*/);

/*
 * functions to remove triples from the flow graph.
 *
 * All of them do the same thing, slightly differently. They
 * vary in the amount of stuff that gets thrown out along with
 * the deleted triple.
 *
 * In addition, some take a 'delay' parameter and are used in a
 * two-phase 'mark & release' algorithm, in cases where deleting
 * on the fly is considered dangerous.
 *
 * All take a pointer to the block containing the triple, in order
 * to handle the case where the last triple in a block is deleted.
 */

TRIPLE *free_triple(/*tp, bp*/);
	/* just deallocates tp, updates bp if necessary */

void unlist_triple(/*delay, tp, bp*/);
	/* calls fix_del_reachdef(), fix_canreach() */

void delete_triple(/*delay, tp, bp*/); 
	/* calls unlist_triple(), deletes triples on implicit use list */

void remove_triple(/*tp, bp*/);
	/* removes triple, left and right subtrees, and triples that become
	   dead as a result of deleting this one.  Does not use expr info,
	   assumes the triples do not form a dag, and does not delay */

/* miscellaneous functions */

extern BOOLEAN  leaves_overlap(/*leafA, leafB*/);
extern LIST	*copy_list();
extern void	remove_dead_triples();
extern TRIPLE	*append_triple(/*after,op,arg1,arg2,type*/);
extern int	new_label();
extern void	move_block(/*after,block*/);
extern BLOCK	*new_block();
extern LIST	*copy_list(/*tail,lqp*/); 
extern LIST	*order_list(/*tail,lqp*/); 
extern LIST	*merge_lists(/*list1,list2*/);
extern BOOLEAN	insert_list(/*lastp,item,lqp*/);
extern int	hash_leaf(/*class,location*/);
extern LEAF	*leaf_lookup(/*class,type,location*/);
extern LEAF	*ileaf(/*i*/);
extern LEAF	*ileaf_t(/*i,t*/);
extern LEAF     *fleaf_t(/*f,t*/);
extern LEAF	*cleaf(/*s*/);
extern LEAF	*reg_leaf(/*leafp,segno,regno*/);
extern LEAF	*new_temp(/* type */);
extern void	fix_del_reachdef(/* delay, triple, reachdef, base */);
extern void	fix_reachdef(/* delay, triple, reachdef, base */);
extern void	fix_canreach(/* triple, can_reach */);
extern int	type_align(/*type*/);
extern int	type_size(/*type*/);
extern BOOLEAN	same_irtype(/*p1,p2*/);
extern void	delete_block(/*bp*/);
extern void	delete_list(/* tail, lp */);
extern BOOLEAN	can_delete(/* tp */);
extern BOOLEAN	may_cause_exception(/* tp */);
extern BOOLEAN	no_change(/* from, to, var */); 
extern long	l_align(/*cp1*/);
extern void	delete_implicit();
extern void	delete_entry(/* leafp */);
extern LIST	*copy_and_merge_lists(/*list1,list2,lqp*/);
extern char	*new_string(/*str*/);

#endif
