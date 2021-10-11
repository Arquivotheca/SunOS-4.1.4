#ifndef lint
static	char sccsid[] = "@(#)page.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "recordrefs.h"
#include "bit_util.h"
#include "make_expr.h"
#include "misc.h"
#include "page.h"
#include "loop.h"
#include "reg.h"
#include "iv.h"
#include "cf.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

/*
 * global data
 */
char	*heap_start;		/* beginning of heap, set at startup */

LISTQ	*proc_lq=NULL;		/* for lists whose duration spans all phases */
LISTQ	*tmp_lq=NULL;		/* for (cautious) use local to a phase */
LISTQ	*df_lq=NULL;		/* for explicit du/ud chains */
LISTQ	*loop_lq=NULL;		/* for loop structures */
LISTQ	*iv_lq=NULL;		/* for induction variable info */
LISTQ	*dependentexpr_lq=NULL;	/* for building expression table */
LISTQ	*cse_lq=NULL;		/* for cse */
LISTQ	*copy_lq=NULL;		/* for copy propagation */
LISTQ	*reg_lq=NULL;		/* for register allocation */

/* from libc */
extern char	*malloc();	/* yes, malloc */
extern int	free();
extern int	getrusage();

/* local */
static unsigned pgsize;		/* page size, or multiple thereof */
static PAGE	*misc_page;
static PAGE	*freepage_lifo = NULL;
static PAGE	*getpage(/*n, used_for, size*/);
static void	free_pages(/*head*/);
static PAGE	*refill_listq(/*lqp, used_for, size*/);
static unsigned	ir_file_size;

static char *pagetagnames[] = {
	"lists",
	"labelrecs",
	"triples",
	"exprs",
	"expr_refs",
	"var_refs",
	"webs",
	"expr_kill_refs",
	"miscellany",
	"heap set",
};

static int npages[] = {
	0, /* LISTPAGE */
	0, /* LABELRECPAGE */
	0, /* TRIPLEPAGE */
	0, /* EXPRPAGE */
	0, /* EXPRREFPAGE */
	0, /* VARREFPAGE */
	0, /* WEBPAGE */
	0, /* EXPRKILLPAGE */
	0, /* MISCPAGE */
	0, /* HEAPSETPAGE */
};

/*
 * print page usage statistics, for debugging
 */
void
pagestats()
{
	register PAGE *pp;
	register unsigned i;
	unsigned ir_pages;
	int freepages[NPAGETAGS];
	struct rusage rusage;
	static char fmt1[] = "%-16s	%8d (%8d bytes)\n";

	(void)printf("\n");
	(void)printf("list allocation summary:\n");
	if (i=queuesize(tmp_lq))
		 (void)printf(fmt1, "tmp_lq", i, i*pgsize);
	if (i=queuesize(cse_lq))
		 (void)printf(fmt1, "cse_lq", i, i*pgsize);
	if (i=queuesize(copy_lq))
		 (void)printf(fmt1, "copy_lq", i, i*pgsize);
	if (i=queuesize(reg_lq))
		 (void)printf(fmt1, "reg_lq", i, i*pgsize);
	if (i=queuesize(loop_lq))
		 (void)printf(fmt1, "loop_lq", i, i*pgsize);
	if (i=queuesize(proc_lq))
		 (void)printf(fmt1, "proc_lq", i, i*pgsize);
	if (i=queuesize(df_lq))
		 (void)printf(fmt1, "df_lq", i, i*pgsize);
	if (i=queuesize(dependentexpr_lq))
		 (void)printf(fmt1, "dependentexpr_lq", i, i*pgsize);
	if (i=queuesize(iv_lq))
		 (void)printf(fmt1, "iv_lq", i, i*pgsize);
	(void)printf("\n");
	(void)printf("page allocation summary:\n"); 
	ir_pages = roundup(ir_file_size, pgsize)/pgsize;
	(void)printf(fmt1, "ir file", ir_pages, ir_file_size);
	for(i = 0; i < (int)NPAGETAGS; i++ ) {
		if (npages[i]) {
			if (i == (int)MISCPAGE) {
				(void)printf(fmt1, pagetagnames[i],
				    npages[i] - ir_pages,
				    npages[i]*pgsize - ir_file_size);
			} else {
				(void)printf(fmt1, pagetagnames[i], npages[i],
				    npages[i]*pgsize);
			}
		}
		freepages[i] =0;
	}
	(void)printf("\n");
	(void)printf("free page summary:\n"); 
	for(pp=freepage_lifo;pp;pp=pp->next) {
		freepages[(int)pp->used_for]++;
	}
	for(i = 0; i < (int)NPAGETAGS; i++ ) {
		if (freepages[i])
			(void)printf(fmt1, pagetagnames[i], freepages[i],
				freepages[i]*pgsize);
	}
	(void)printf("\n");
  	(void)getrusage(RUSAGE_SELF, &rusage);
	(void)printf("%d page faults\n\n", rusage.ru_majflt);
}

/*
 * set up the heap for a new procedure.  The initial misc_page
 * allocates storage for the IR file structures, using the remainder
 * for other structures which cannot be reclaimed until heap_reset time.
 */
char *
heap_setup(irf_size)
	unsigned irf_size;
{
	PAGE *p;
	char *ir_file_space;

	if(pgsize == 0) {
		pgsize = (unsigned)getpagesize();
	}
	p = getpage(irf_size, MISCPAGE, sizeof(char));
	ir_file_space = p->alloc;
	ir_file_size = irf_size;	/* for pagestats() */
	(void)bzero(ir_file_space, (int)ir_file_size);
	p->alloc += irf_size;
	p->count -= irf_size;
	misc_page = p;
	return (ir_file_space);
}

/*
 * allocate storage for (n) objects of a given size, rounding
 * up the request to a multiple of pgsize, and setting a count
 * for subsequent suballocation. We keep a cache of single pages,
 * since they are most frequently used, for sub-allocation of
 * numerous but small structures.
 *
 * If we get a multi-page request, we dump the cache with
 * the hope of coalescing free pages into a chunk big
 * enough to satisfy the request without having to expand
 * the heap.
 */
static PAGE*
getpage(n, used_for, size)
	unsigned n;		/* # of objects */
	PAGETAG used_for;	/* what the page is to be used for */
	unsigned size;		/* size of structures to be sub-allocated */
{
	PAGE *page;
	char *heap_top;
	unsigned nbytes;

	nbytes = roundup(n*size + sizeof(PAGE), pgsize);
	if(nbytes == pgsize && freepage_lifo != NULL) {
		page = freepage_lifo;
		freepage_lifo = page->next;
	} else {
		while(freepage_lifo != NULL) {
			page = freepage_lifo;
			freepage_lifo = freepage_lifo->next;
			(void)free((char*)page);
		}
		page = (PAGE*)malloc(nbytes);
		if(page == NULL) {
			if (hdr.procname != NULL) {
				(void)fprintf(stderr, "Routine %s too big: \n",
					(char *) hdr.procname);
			}
			(void)fprintf(stderr, "use lower optimization level");
			(void)fprintf(stderr, " or increase swap space \n");
			perror("getpage");
			if(debugflag[PAGE_DEBUG]) {
				extern char *sbrk();
				heap_top = sbrk(0);
				(void)printf("%d bytes of storage used\n",
					(heap_top-heap_start));
				pagestats();
			}
			quit("getpage: out of memory");
			/*NOTREACHED*/
		}
	}
	page->used_for = used_for;
	page->next = NULL;
	page->alloc = (char*)page + sizeof(PAGE);
	page->count = (nbytes - sizeof(PAGE))/size;
	npages[(int)used_for] += nbytes/pgsize;
	return page;
}

/*
 * recycle a list of pages, by putting
 * them on the freepage_lifo.
 */
static void
free_pages(head)
	register PAGE *head;
{
	register PAGE *next,*pp;

	for(pp = head; pp; pp = next) {
		next = pp->next;
		pp->next = freepage_lifo;
		npages[(int)pp->used_for]--;
		freepage_lifo = pp;
	}
}

/*
 * allocate a new listq structure.
 * This is a handle for allocation of the list nodes
 * themselves, which permits their pages to be reused
 * when you are done with them.
 */
LISTQ *
new_listq()
{
	register LISTQ *listq;

	listq = (LISTQ*) ckalloc(1,sizeof(LISTQ));
	return(listq);
}

/*
 * Free the pages of list nodes on a listq structure.
 * We put them on a free list, so that they may be reused
 * easily for more structures of similar size.
 */
void
empty_listq(listqp)
	LISTQ *listqp;
{
	if (listqp != NULL) {
		free_pages(listqp->head);
		listqp->head = listqp->tail = (PAGE*) NULL;
	}
}

/*
 * put a new page on the tail of the allocation queue lqp.
 */
static PAGE *
refill_listq(lqp, used_for, size)
	register LISTQ *lqp;
	PAGETAG used_for;
	unsigned size;
{
	register PAGE *p = getpage(1, used_for, size);
	if (lqp->tail) {
		lqp->tail->next = p;
	}
	if (lqp->head == NULL) {
		lqp->head = p;
	}
	lqp->tail = p;
	return p;
}

/*
 * queuesize(lqp) = # of pages on the list page queue lqp
 */
int
queuesize(lqp)
	LISTQ* lqp;
{
	register PAGE *pp;
	register n;

	n = 0;
	if (lqp != NULL) {
	    for(pp=lqp->head; pp; pp=pp->next)
		n++;
	}
	return n;
}

/*
 * allocate a new list node from a given list page queue.
 */
LIST *
NEWLIST(lqp) 
	register LISTQ *lqp;
{
	register PAGE *p;
	register LIST *tmp;

	if( (p=lqp->tail) == NULL || p->count == 0 ) {
		p = refill_listq(lqp, LISTPAGE, sizeof(LIST));
	}
	(p->count)--;
	tmp = (LIST*)p->alloc;
	p->alloc += sizeof(LIST);
	tmp->next = tmp;
	return tmp;
}

/*
 * allocate a new label record.  Active pages are kept
 * on a queue and freed if flush==IR_TRUE.
 */
LABELREC *
new_labelrec(flush)
	BOOLEAN flush;
{
	static LISTQ lq;
	LABELREC *labelrec;
	PAGE *p;

	if(flush) {
		empty_listq(&lq);
		return NULL;
	}
	if( (p=lq.tail) == NULL || p->count == 0 ) {
		p = refill_listq(&lq, LABELRECPAGE, sizeof(LABELREC));
	}
	labelrec = (LABELREC*)p->alloc;
	p->alloc = (char*)(labelrec+1);
	(p->count)--;
	(void)bzero((char*)labelrec, sizeof(LABELREC));
	return labelrec;
}

/*
 * allocate a new triple.  Active pages are kept
 * on a queue and freed if flush==IR_TRUE.
 */
TRIPLE *
new_triple(flush)
	BOOLEAN flush;
{
	static LISTQ lq;
	TRIPLE *tp;
	PAGE *p;

	if(flush) {
		empty_listq(&lq);
		return NULL;
	}
	if( (p=lq.tail) == NULL || p->count == 0 ) {
		p = refill_listq(&lq, TRIPLEPAGE, sizeof(TRIPLE));
	}
	tp = (TRIPLE*)p->alloc;
	p->alloc = (char*)(tp+1);
	(p->count)--;
	return tp;
}

/*
 * allocate a new expression node.  Active pages are kept
 * on a queue and freed if flush==IR_TRUE.
 */
EXPR *
new_expr(flush)
	BOOLEAN flush;
{
	static LISTQ lq;
	EXPR *expr;
	PAGE *p;

	if(flush) {
		empty_listq(&lq);
		return NULL;
	}
	if( (p=lq.tail) == NULL || p->count == 0 ) {
		p = refill_listq(&lq, EXPRPAGE, sizeof(EXPR));
	}
	expr = (EXPR*)p->alloc;
	p->alloc = (char*)(expr+1);
	(p->count)--;
	return expr;
}

/*
 * allocate a new var_ref node.  Active pages are kept
 * on a queue and freed if flush==IR_TRUE.
 */
VAR_REF *
new_var_ref(flush)
	BOOLEAN flush;
{
	static VAR_REF *last_var_ref;
	static LISTQ lq;
	VAR_REF *rp;
	PAGE *p;

	if(flush) {
		empty_listq(&lq);
		var_ref_head = last_var_ref = NULL;
		return NULL;
	}
	if( (p=lq.tail) == NULL || p->count == 0 ) {
		p = refill_listq(&lq, VARREFPAGE, sizeof(VAR_REF));
	}
	rp = (VAR_REF*)p->alloc;
	p->alloc = (char*)(rp+1);
	(p->count)--;
	rp->next_vref = rp->next = (VAR_REF*) NULL;
	rp->refno = nvarrefs++;
	if( var_ref_head == NULL ) {
		var_ref_head = rp;
	}
	if( last_var_ref ) {
		last_var_ref->next = rp;
	}
	last_var_ref = rp;
	return rp;
}

/*
 * allocate a new expr_ref node.  Active pages are kept
 * on a queue and freed if flush==IR_TRUE.
 */
EXPR_REF *
new_expr_ref(flush)
	BOOLEAN flush;
{
	static LISTQ lq;
	register EXPR_REF *rp;
	PAGE *p;

	if(flush) {
		empty_listq(&lq);
		return NULL;
	}
	if( (p=lq.tail) == NULL || p->count == 0 ) {
		p = refill_listq(&lq, EXPRREFPAGE, sizeof(EXPR_REF));
	}
	rp = (EXPR_REF*)p->alloc;
	p->alloc = (char*)(rp+1);
	(p->count)--;
	rp->next_eref = (EXPR_REF*) NULL;
	rp->refno = nexprrefs++;
	return rp;
}

/*
 * allocate a new expr_killref node.  Active pages are kept
 * on a queue and freed if flush==IR_TRUE.
 */
EXPR_KILLREF *
new_expr_killref(flush)
	BOOLEAN flush;
{
	static LISTQ lq;
	register EXPR_KILLREF *rp;
	PAGE *p;

	if(flush) {
		empty_listq(&lq);
		return NULL;
	}
	if( (p=lq.tail) == NULL || p->count == 0 ) {
		p = refill_listq(&lq, EXPRKILLPAGE, sizeof(EXPR_KILLREF));
	}
	rp = (EXPR_KILLREF*)p->alloc;
	p->alloc = (char*)(rp+1);
	(p->count)--;
# ifdef DEBUG
	rp->refno = nexprrefs++;
# endif
	rp->next_eref =  (EXPR_REF*) NULL;
	return rp;
}

/*
 * allocate a new web.  Active pages are kept
 * on a queue and freed if flush==IR_TRUE.
 */
WEB *
alloc_new_web(flush)
	BOOLEAN flush;
{
	static LISTQ lq;
	register WEB *wp;
	PAGE *p;

	if(flush) {
		empty_listq(&lq);
		return NULL;
	}
	if( (p=lq.tail) == NULL || p->count == 0 ) {
		p = refill_listq(&lq, WEBPAGE, sizeof(WEB));
	}
	wp = (WEB*)p->alloc;
	p->alloc = (char*)(wp+1);
	(p->count)--;
	return wp;
}

/*
 * Allocate a set on the heap, rounding the size up to
 * a multiple of pgsize().  We assume that sets are few
 * in number, but large. The intent is to make the storage
 * reusable when we are done with it; see free_heap_set() below.
 */
SET_PTR
new_heap_set(nrows, ncols)
	int nrows;
	int ncols;
{
	register unsigned bit_rowsize, word_rowsize, nbytes;
	register SET_PTR set;

	bit_rowsize = roundup((unsigned)ncols,(unsigned)BPW); 
	word_rowsize = bit_rowsize / BPW; 
	nbytes = word_rowsize * nrows * sizeof(unsigned);
	set = (SET_PTR)getpage(nbytes, HEAPSETPAGE, sizeof(char));
	set->nrows = nrows; 
	set->bit_rowsize = bit_rowsize; 
	set->word_rowsize = word_rowsize; 
	set->bits = (SET)((char*)set + sizeof(struct set_description));
	(void)bzero((char*)set->bits, (int)nbytes);
	return set;
}

/*
 * free the storage occupied by a set.
 * Here we just put it back in the heap by calling
 * free().  You might also try chopping it into pages
 * and putting them on the freepage_lifo.
 */
void
free_heap_set(set)
	SET_PTR set;
{
	unsigned nbytes;

	nbytes = set->nrows*set->word_rowsize*sizeof(set->bits[0]);
	npages[(int)HEAPSETPAGE] -= roundup(nbytes,pgsize)/pgsize;
	(void)free((char*)set);
}

/*
 * allocate (n) contiguous objects of (size) bytes,
 * initializing them to zero.   These have unknown
 * lifespan, up to the end of processing the current
 * procedure.
 *
 * misc_page maintains a chain of the pages of such objects,
 * which allows us to free them when the time comes.
 */
char *
ckalloc(n,size)
	unsigned n,size;
{
	unsigned nbytes;
	PAGE *new_page;
	char *result;

#ifdef sparc
	nbytes = roundup(n*size, sizeof(double));
#else
	nbytes = roundup(n*size, sizeof(long));
#endif
	if(nbytes > pgsize) {
		if(debugflag[PAGE_DEBUG]) {
			if(misc_page && misc_page->count)
				(void)printf("ckalloc %d allocated %d wasted\n",
					nbytes, misc_page->count);
		}
		new_page = getpage(nbytes, MISCPAGE, sizeof(char));
		new_page->next = misc_page;
		misc_page = new_page;
	} else if (!misc_page || misc_page->count < nbytes) {
		new_page = getpage(nbytes, MISCPAGE, sizeof(char));
		if(misc_page && new_page == (PAGE*) ((char*)misc_page+pgsize)) {
			/* the next page is contiguous */
			misc_page->count += pgsize;
		} else {
			if(debugflag[PAGE_DEBUG]
			    && misc_page && misc_page->count) {
				(void)printf("ckalloc %d allocated %d wasted\n",
					nbytes, misc_page->count);
			}
			new_page->next = misc_page;
			misc_page = new_page;
		}
	}
	result = misc_page->alloc;
	misc_page->alloc += nbytes;
	misc_page->count -= nbytes;
	(void)bzero(result, (int)nbytes);
	return result;
}

/*
 * Allocate temporary storage on the heap.
 * Storage is initialized to zero.
 * Caller MUST free explicitly before returning.
 * This replaces "ckalloca" in earlier versions of iropt.
 */
char *
heap_tmpalloc(nbytes)
	unsigned nbytes;
{
	char *p;
	p = malloc(nbytes);
	if (p == NULL) {
	    quita("heap_tmpalloc: cannot allocate %d bytes of storage\n",
		nbytes);
	    /*NOTREACHED*/
	}
	(void)bzero(p, (int)nbytes);
	return p;
}

/*
 * free temporary storage.
 * DO NOT USE THIS TO FREE OBJECTS ALLOCATED BY ckalloc!
 */
void
heap_tmpfree(ptr)
	char *ptr;
{
	(void)free(ptr);
}

/*
 * reset the heap to the "empty" state; called
 * at end of processing for the current procedure.
 */
void
heap_reset()
{
	register PAGE *p, *nextp;

	if(debugflag[PAGE_DEBUG]) {
		pagestats();
	}
	(void)new_expr(IR_TRUE);
	(void)new_expr_ref(IR_TRUE);
	(void)new_expr_killref(IR_TRUE);
	(void)new_var_ref(IR_TRUE);
	(void)new_triple(IR_TRUE);
	(void)alloc_new_web(IR_TRUE);
	(void)new_labelrec(IR_TRUE);

	empty_listq(dependentexpr_lq); dependentexpr_lq = NULL;
	empty_listq(cse_lq);	cse_lq = NULL;
	empty_listq(copy_lq);	copy_lq = NULL;
	empty_listq(reg_lq);	reg_lq = NULL;
	empty_listq(df_lq);	df_lq = NULL;
	empty_listq(proc_lq);	proc_lq = NULL;
	empty_listq(tmp_lq);	tmp_lq = NULL;
	empty_listq(loop_lq);	loop_lq = NULL;
	empty_listq(iv_lq);	iv_lq = NULL;

	free_triple_lifo = TNULL;
	free_block_lifo = (BLOCK*)NULL;
	loop_cleanup();
	iv_cleanup();
	for(p = misc_page; p != NULL; p = nextp) {
		nextp = p->next;
		(void)free((char*)p);
	}
	npages[(int)MISCPAGE] = 0;
	misc_page = (PAGE*) NULL;
	for(p = freepage_lifo; p != NULL; p = nextp) {
		nextp = p->next;
		(void)free((char*)p);
	}
	freepage_lifo = NULL;
}
