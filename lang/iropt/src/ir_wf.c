#ifndef lint
static	char sccsid[] = "@(#)ir_wf.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

# include "iropt.h"
# include "page.h"
# include "misc.h"
# include "reg.h"
# include "auto_alloc.h"
# include "ir_wf.h"
# include <stdio.h>

#define SIZEOF_TRIPLE (sizeof(TRIPLE) - IROPT_TRIPLE_FIELDS*sizeof(char*))
#define SIZEOF_LEAF (sizeof(LEAF) -  IROPT_LEAF_FIELDS*sizeof(char*))
#define SIZEOF_BLOCK (sizeof(BLOCK) - IROPT_BLOCK_FIELDS*sizeof(char*))

#define ENINDEX_TRIPLE(ptr)  (TRIPLE*) (ptr ? (headr.triple_offset + \
			((TRIPLE*)ptr)->tripleno*SIZEOF_TRIPLE ) : -1 )

#define ENINDEX_BLOCK(ptr)  (BLOCK*) ( ptr ? (headr.block_offset + \
			((BLOCK*)ptr)->blockno*SIZEOF_BLOCK ) : -1 )

#define ENINDEX_LEAF(ptr)  (LEAF*) (ptr ? (headr.leaf_offset + \
			((LEAF*)ptr)->leafno*SIZEOF_LEAF ) : -1 ) 

#define ENINDEX_SEGMENT(ptr)  (SEGMENT*) (ptr ? (headr.seg_offset + \
			(((SEGMENT*)ptr)->segno)*sizeof(SEGMENT) ) : -1 )

/* local */
static HEADER	headr;
static FILE	*irfile;
static long	top_list;
static long	bot_list;
static long	start;
static void	chk_fseek(/*f, offset, from*/);
static void	chk_fwrite(/*buf, size, n, f*/);
static void	find_offsets();
static void	write_segments();
static void	write_strings();
static void	write_triples();
static void	write_blocks();
static void	write_leaves();
static long	enindex_node(/*p*/);
static LIST     *get_list_buffer();
static void	write_lists();
static long	enindex_list(/*list*/);
static long	enindex_stringptr(/*cp*/);

/* from libc */
extern char	*sys_errlist[];
extern int	errno;

/*
 * buffers for conversion of list cells prior to output
 */

#define LIST_BUFSIZ 1024
#define MAX_LIST_BUFFERS 16

typedef struct list_buffer {
	struct list_buffer *nextbuf;	/* next buffer */
	int nlists;			/* # of list cells in this buffer */
	LIST buf[LIST_BUFSIZ];		/* storage for converted list cells */
} LIST_BUFFER;

static struct list_buffer *first_list_buffer=NULL;
static struct list_buffer *last_list_buffer=NULL;
static int n_list_buffers = 0;

static void
chk_fseek(f, offset, from)
	FILE *f;
	long offset;
	int from;
{
	if (fseek(f, offset, from) == -1) {
		quita("write_irfile: %s\n", sys_errlist[errno]);
		/*NOTREACHED*/
	}
}

static void
chk_fwrite(buf, size, n, f)
	char *buf;
	int size, n;
	FILE *f;
{
	if (fwrite(buf, size, n, f) != n) {
		quita("write_irfile: %s\n", sys_errlist[errno]);
		/*NOTREACHED*/
	}
}

/*	write out an IR file.  First determine how big the file will be by
**	counting everything except list blocks, renumber structures as we go.
**	From this we can calculate the offset at which triples will start, where 
**	blocks will start, etc.  Then use  the structure number to calculate its offset
*/
void
write_irfile(procno, procname, proc_type, source_file, source_lineno,
    opt_level, filep)
	int procno;
	char *procname;
	TYPE proc_type;
	char *source_file;
	int source_lineno;
	int opt_level;
	FILE *filep;
{
	(void)bzero((char*)&headr,sizeof(HEADER));
	irfile = filep;

	headr.major_vers = IR_MAJOR_VERS;
	headr.minor_vers = IR_MINOR_VERS;
	headr.procno = procno;
	headr.procname = (char*) new_string(procname);
	headr.source_file = (char*) new_string(source_file);
	headr.source_lineno = source_lineno;
	headr.regmask = regmask;
#if TARGET == SPARC
	headr.fregmask = fregmask;
#endif
	headr.proc_type = proc_type;
	headr.opt_level = opt_level;
	
	/*
	 * move exit_block to end -- this is a minor nudge
	 * to assist loop inversion in c2.
	 */
	if (exit_block != last_block) {
		exit_block->prev->next = exit_block->next;
		exit_block->next->prev = exit_block->prev;
		last_block->next = exit_block;
		exit_block->next = NULL;
		exit_block->prev = last_block;
		last_block = exit_block;
	}
	start = ftell(irfile);
	find_offsets();
	headr.procname = (char*) enindex_stringptr(headr.procname);
	headr.source_file = (char*) enindex_stringptr(headr.source_file);
	top_list = headr.list_offset;
	bot_list = top_list;
		/* save space for the header - don't know nlists yet*/
	chk_fseek(irfile, (long)sizeof(HEADER), 1); 
	write_triples();
	write_blocks();
	write_leaves();
	write_segments();
	write_strings();
	write_lists();

	chk_fseek(irfile, start, 0);
	headr.proc_size = top_list;
	headr.file_status = OPTIMIZED;
	chk_fwrite((char*)&headr, sizeof(HEADER), 1, irfile);
	chk_fseek(irfile, start+top_list, 0);
}


static void
find_offsets()
{
	BLOCK *bp;
	TRIPLE *tp, *first, *tp2, *tp3;
	LEAF *leafp;
	int charno, tripleno, leafno, blockno;
	STRING_BUFF *sbp;
	SEGMENT *seg;
	int segno;

	charno =  tripleno =  leafno =  blockno = 0;
	headr.triple_offset = sizeof(HEADER);
	for(bp=entry_block; bp; bp=bp->next) {
		first = (TRIPLE*) NULL;
		if(bp->last_triple) first=bp->last_triple->tnext;
		TFOR(tp,first){
			if(ISOP(tp->op,NTUPLE_OP)) {
				tp2 = (TRIPLE*) tp->right;
				TFOR(tp3, tp2) {
					tp3->tripleno = tripleno++;
				}
			}
			tp->tripleno = tripleno++;
		}
	}
	headr.block_offset = headr.triple_offset + tripleno*SIZEOF_TRIPLE;
	for(bp=entry_block; bp; bp = bp->next) {
		bp->blockno = blockno++;
	}
	if(blockno != nblocks) {
		quit("find_offsets: nblocks wrong");
	}
	headr.leaf_offset = headr.block_offset + nblocks*SIZEOF_BLOCK;
	for(leafp=leaf_tab; leafp; leafp = leafp->next_leaf) {
		leafp->leafno = leafno++;
	}
	nleaves -= nleaves_deleted;
	if(leafno != nleaves) {
		quit("find_offsets: nleaves wrong");
	}
	headr.seg_offset = headr.leaf_offset + nleaves*SIZEOF_LEAF;
	segno = 0;
	for(seg = seg_tab; seg != NULL; seg = seg->next_seg) {
		seg->segno = segno++;
	}
	if (segno != nseg - nsegs_deleted) {
		quit("find_offsets: nseg - nsegs_deleted wrong");
	}
	nseg = segno;
	headr.string_offset = headr.seg_offset + nseg*sizeof(SEGMENT);
	for(sbp=first_string_buff; sbp; sbp=sbp->next) {
		charno += (sbp->top - sbp->data);
	}
	headr.list_offset = headr.string_offset + roundup((unsigned)charno,sizeof(long));
}

static void
write_segments()
{
	register SEGMENT *seg, *next_seg;

	for(seg = seg_tab; seg != NULL; seg = next_seg) {
		seg->name = (char*) enindex_stringptr(seg->name);
		next_seg = seg->next_seg;
		seg->next_seg = ENINDEX_SEGMENT(seg->next_seg);
		chk_fwrite((char*)seg, sizeof(SEGMENT), 1, irfile);
	}
}

static void
write_strings()
{
	STRING_BUFF *sbp;
	int n, strtabsize;
	long zeroes = 0x00000000;

	strtabsize = 0;
	for(sbp=first_string_buff; sbp; sbp=sbp->next) {
		if( n = sbp->top - sbp->data ) {
			strtabsize += n;
			chk_fwrite(sbp->data, sizeof(char), n, irfile);
		}
	}
	if(n=(strtabsize%sizeof(long))) {
		chk_fwrite((char*)&zeroes, sizeof(char), sizeof(long)-n,
		    irfile);
	}
}

static void
write_triples()
{
	BLOCK *bp;
	register TRIPLE *first, *tp;
	TRIPLE *tp_tnext;

	for(bp=entry_block; bp; bp=bp->next) {
		first = (TRIPLE*) NULL;
		if(bp->last_triple) first=bp->last_triple->tnext;
		for(tp=first; tp; tp = (tp_tnext == first ? TNULL : tp_tnext)){
			if(ISOP(tp->op,NTUPLE_OP)) {
			register TRIPLE *tpr;
			TRIPLE  *tpr_tnext;

				for(tpr=(TRIPLE*)tp->right; tpr; 
						tpr = (tpr_tnext == (TRIPLE*) tp->right ? TNULL : tpr_tnext)){
					tpr->left = (IR_NODE *) enindex_node(tpr->left);
					tpr->right = (IR_NODE *) enindex_node(tpr->right);
					tpr->tprev = (TRIPLE *) ENINDEX_TRIPLE(tpr->tprev);
					tpr_tnext = tpr->tnext;
					tpr->tnext = (TRIPLE *) ENINDEX_TRIPLE(tpr->tnext);
					tpr->can_access = (LIST *) enindex_list(tpr->can_access);
					chk_fwrite((char*)tpr,SIZEOF_TRIPLE, 1, irfile);
				}
			}

			tp->left = (IR_NODE *) enindex_node(tp->left);
			tp->right = (IR_NODE *) enindex_node(tp->right);
			tp->tprev = (TRIPLE *) ENINDEX_TRIPLE(tp->tprev);
			tp_tnext = tp->tnext;
			tp->tnext = (TRIPLE *) ENINDEX_TRIPLE(tp->tnext);
			tp->can_access = (LIST *) enindex_list(tp->can_access);
			chk_fwrite((char*)tp, SIZEOF_TRIPLE, 1, irfile);
		}
	}
}

static void
write_blocks()
{
	BLOCK *bp, *bp_next;

	for(bp=entry_block; bp; bp=bp_next) {
		bp->entryname = (char *) enindex_stringptr(bp->entryname);
		bp->last_triple = ENINDEX_TRIPLE(bp->last_triple);
		bp->pred = (LIST*) enindex_list(bp->pred);
		bp->succ = (LIST*) enindex_list(bp->succ);
		bp_next = bp->next;
		bp->next = (BLOCK*) ENINDEX_BLOCK(bp->next);
		chk_fwrite((char*)bp,SIZEOF_BLOCK, 1, irfile);
	}
}

static void
write_leaves()
{
	LEAF *p, *p_next;

	for(p = leaf_tab; p ; p = p_next ) {
		if(p->class == VAR_LEAF || p->class == ADDR_CONST_LEAF)  {
			p->val.addr.seg = ENINDEX_SEGMENT(p->val.addr.seg);
		} else {
			if(p->val.const.isbinary == IR_TRUE) {
				p->val.const.c.bytep[0] = 
					(char *) enindex_stringptr(p->val.const.c.bytep[0]);
				if( ISCOMPLEX(p->type.tword ) ) {
					p->val.const.c.bytep[1] = 
						(char *) enindex_stringptr(p->val.const.c.bytep[1]);
				}
			} else if( ISCHAR(p->type.tword) || ISPTRFTN(p->type.tword) )  {
				p->val.const.c.cp = 
					(char *) enindex_stringptr(p->val.const.c.cp);
			} else if( ISREAL(p->type.tword) ){
				p->val.const.c.fp[0] = 
					(char *) enindex_stringptr(p->val.const.c.fp[0]);
			} else if( ISCOMPLEX(p->type.tword ) ) {
				p->val.const.c.fp[0] = 
					(char *) enindex_stringptr(p->val.const.c.fp[0]);
				p->val.const.c.fp[1] = 
					(char *) enindex_stringptr(p->val.const.c.fp[1]);
			}
		}
		p_next = p->next_leaf;
		p->next_leaf = ENINDEX_LEAF( p->next_leaf );
		p->pass1_id = (char *) enindex_stringptr(p->pass1_id);
		p->overlap = (LIST *) enindex_list(p->overlap);
		p->neighbors = (LIST *) enindex_list(p->neighbors);
		p->addressed_leaf = ENINDEX_LEAF( p->addressed_leaf );
		chk_fwrite((char*)p, SIZEOF_LEAF, 1, irfile);
	}
}

static long
enindex_node(p)
	register IR_NODE *p;
{
	if(p == (IR_NODE*)NULL) return(-1);
	switch(p->operand.tag) {
		case ISTRIPLE:
			return( headr.triple_offset + ((TRIPLE*)p)->tripleno*SIZEOF_TRIPLE );

		case ISLEAF:
			return( headr.leaf_offset + ((LEAF*)p)->leafno*SIZEOF_LEAF );

		case ISBLOCK:
			return( headr.block_offset + ((BLOCK*)p)->blockno*SIZEOF_BLOCK );

		default:
			quit("enindex_node: bad tag");
	}
	/*NOTREACHED*/
}

/*
 * return buffer space for an enindexed copy of a LIST node
 */
static LIST *
get_list_buffer()
{
	LIST_BUFFER *new_list_buffer;

	if (last_list_buffer == NULL
	    || last_list_buffer->nlists == LIST_BUFSIZ) {
		if (n_list_buffers == MAX_LIST_BUFFERS) {
			long current_offset = ftell(irfile);
			write_lists();
			chk_fseek(irfile, current_offset, 0);
		}
		new_list_buffer = (LIST_BUFFER*)
			heap_tmpalloc((unsigned)sizeof(LIST_BUFFER));
		new_list_buffer->nextbuf = NULL;
		new_list_buffer->nlists = 0;
		n_list_buffers++;
		if (last_list_buffer == NULL) {
			first_list_buffer = new_list_buffer;
			last_list_buffer = new_list_buffer;
		} else {
			last_list_buffer->nextbuf = new_list_buffer;
			last_list_buffer = new_list_buffer;
		}
	}
	return &last_list_buffer->buf[last_list_buffer->nlists++];
}

/*
 * write the contents of the enindexed list buffers.
 * update the bot_list file offset to account for
 * the buffers written.
 */
static void
write_lists()
{
	LIST_BUFFER *bp, *nextbp;

	bp = first_list_buffer;
	chk_fseek(irfile, start+bot_list, 0);
	while(bp != NULL) {
		chk_fwrite((char*)bp->buf, sizeof(LIST), bp->nlists, irfile);
		bot_list += sizeof(LIST)*bp->nlists;
		nextbp = bp->nextbuf;
		heap_tmpfree((char*)bp);
		bp = nextbp;
	}
	first_list_buffer = NULL;
	last_list_buffer = NULL;
	n_list_buffers = 0;
}

/*
 * copy a list into an output buffer and change pointers to indices
 */
static long
enindex_list(list)
	LIST *list;
{
	register LIST *lp;
	register LIST *newl;
	long first_offset;

	if(list == LNULL) {
		return -1;
	}
	first_offset = top_list;
	LFOR(lp,list) {
		newl = get_list_buffer();
		newl->next = (LIST*) (top_list + sizeof(LIST));
		newl->datap = (LDATA*) enindex_node((IR_NODE*)lp->datap);
		top_list += sizeof(LIST);
	}
	newl->next = (LIST*)first_offset;
	return first_offset;
}

/* turn a pointer to astring in a string_buffer into an ir file offset*/
static long
enindex_stringptr(cp)
	register char *cp;
{
	register STRING_BUFF *sbp;
	register int offset;

	if(!cp) return(-1);

	offset = headr.string_offset;
	for(sbp = first_string_buff;sbp;sbp=sbp->next) {

		if(cp >= sbp->data && cp < sbp->top ) {
			return( offset + (cp - sbp->data));
		}
		offset += (sbp->top - sbp->data);
	}
	quit("enindex_stringptr : string not in a STRING_BUFF");
	/*NOTREACHED*/
}
