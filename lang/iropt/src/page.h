/*	@(#)page.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __page__
#define __page__

/*
 * Memory is managed in large blocks or PAGES, which may actually
 * be a multiple of the system pagesize.  Pages are used in the
 * following ways:
 *
 * 1. A page may contain a large number of structures of a single type:
 *    lists, triples, exprs, etc. fall into this category.
 *
 * 2. 1 or more contiguous pages may contain a large structure, such
 *    as a bit matrix for data flow computations.
 *
 * 3. MISCPAGE pages hold sundry structures with indefinite unknown
 *    lifetimes.  These  are retained until end of processing the
 *    current procedure.  Profligate use of these pages is discouraged.
 *
 * Reuse of storage within a procedure is generally done at the page level
 * - eg at the end of each transformation all pages that hold list blocks
 * for the canreach/reachdef chains are added to the free page list.  At
 * the end of the current procedure, all heap storage is reclaimed.
 */

typedef enum {
	LISTPAGE=0,	/* lists */
	LABELRECPAGE,	/* labelrecs */
	TRIPLEPAGE,	/* triples */
	EXPRPAGE,	/* exprs */
	EXPRREFPAGE,	/* expr_refs */
	VARREFPAGE,	/* var_refs */
	WEBPAGE,	/* webs */
	EXPRKILLPAGE, /* expr_killref records */
	MISCPAGE,	/* assorted small structures */
	HEAPSETPAGE,	/* large structures (ir file, bit vectors) */
	NPAGETAGS,	/* # of preceding tag values in this enumeration */
} PAGETAG;

typedef struct page {
	PAGETAG used_for;
	struct page *next;	/* next page in the list */
	char *alloc;		/* next structure within the page*/
	int count;		/* # of structures left in this page */
} PAGE;

/*
 * A list page queue is a fifo list for managing active pages,
 * which in turn are suballocated into masses of structures
 * of identical type.  At the end of a phase, the pages may
 * all be freed at once for use by other phases.
 */
typedef struct listq {
	PAGE *head, *tail;
} LISTQ;

/* list queue headers for various optimization phases */
extern LISTQ
	*proc_lq,	/* for lists whose duration spans all phases */
	*tmp_lq,	/* for (cautious) use local to a phase */
	*df_lq,		/* for explicit du/ud chains */
	*loop_lq,	/* for loop structures */
	*iv_lq,		/* for induction variable info */
	*dependentexpr_lq,  /* for building expression table */
	*cse_lq,	/* for cse */
	*copy_lq,	/* for copy propagation */
	*reg_lq;	/* for register allocation */

/* allocate a new list page queue */
extern LISTQ	*new_listq();

/* allocate a new list node from a list page queue */
extern LIST	*NEWLIST(/*listq*/);

/* recycle the pages of a list queue */
extern void	empty_listq(/*listqp*/);

/* # pages on a list queue */
extern int	queuesize(/*listqp*/);

/* print page info, for debugging */
extern void	pagestats();

/*
 * allocation functions for lists, triples, exprs, var_refs, etc.
 * Each function returns one object, if flush==IR_FALSE.
 * If called with flush==IR_TRUE, recycle the pages allocated
 * to active objects.
 */
extern LIST		*NEWLIST(/*listq*/);
extern LABELREC		*new_labelrec(/*flush*/);
extern TRIPLE		*new_triple(/*flush*/);
extern EXPR		*new_expr(/*flush*/);
extern VAR_REF		*new_var_ref(/*flush*/);
extern EXPR_REF		*new_expr_ref(/*flush*/);
extern EXPR_KILLREF	*new_expr_killref(/*flush*/);
extern WEB		*alloc_new_web(/*flush*/);

/*
 * new_heap_set allocates a (presumably large) set on the heap.
 * free_heap_set frees it.
 */
extern SET_PTR		new_heap_set(/*nrows, ncols*/);
extern void		free_heap_set(/*set*/);

/*
 * ckalloc() allocates n objects of uniform size.  Objects created
 * by ckalloc reside in MISCPAGE pages and are freed at end of processing.
 * Their storage cannot be reused in any other way.
 */
extern char	*ckalloc(/*n,size*/);

/*
 * allocate temporary storage for an object of arbitrary length
 * and brief lifetime.  Such objects are explicitly returned to
 * the system without going through the PAGE layer.
 */
extern char	*heap_tmpalloc(/*nbytes*/);
extern void	heap_tmpfree(/*ptr*/);

/* initialize the heap, at beginning of processing the current procedure. */
extern char	*heap_setup(/*irf_size*/);

/* reset the heap; called at the end of processing the current procedure. */
extern void	heap_reset();

/* beginning of heap -- determined at startup and used for accounting */
extern char	*heap_start;

#endif
