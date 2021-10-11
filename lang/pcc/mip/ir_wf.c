#ifndef lint
static	char sccsid[] = "@(#)ir_wf.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

# include "ir_misc.h"

/*
 * This source file implements the i/o interface to the IR file.
 * The main entry point is ir_write_file(), which is called at the end
 * of each routine to write the per-procedure IR structures to the IR
 * file.  Note that other data (e.g., global variable definitions) may
 * occur between functions; these must be written to a separate file.
 */

/* import from libc */
extern char *sys_errlist[];
extern int errno;
extern char *malloc(/*nbytes*/);
extern void free(/*p*/);

/* import from ir_alloc */
extern void free_blocks();
extern void free_leaves();
extern void free_triples();
extern void free_strings();
extern void free_segments();
extern void free_lists();
extern void free_chains();

/* import from ir_misc */
extern void fill_can_access();
extern void quita(/*a1,a2,a3,a4,a5*/);

/* import from ir_debug */
extern void dump_blocks();
extern void dump_leaves();
extern void dump_triples();
extern void dump_strings();
extern void dump_segments();

/* export */
HEADER hdr;
void ir_write_file(/*irfile, opt_level*/);
int aliaser = 0;
void chk_fseek(/*f, offset, from*/);
void chk_fwrite(/*buf, size, n, f*/);

/* private */
static long top_list;
static long bot_list;
static long start;
static int find_offsets();
static void write_segments(/*irfile*/);
static void write_strings(/*irfile*/);
static void write_triples(/*irfile*/);
static void write_blocks(/*irfile*/);
static void write_leaves(/*irfile*/);
static int enindex_node(/*p*/);
static void write_lists(/*irfile*/);
static LIST *get_list_buffer(/*irfile*/);
static long enindex_list(/*irfile,list*/);
static int enindex_stringptr(/*cp*/);
static void free_all();

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

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * macros for converting pointers to file-relative offsets
 */

#define roundup(a,b)    ( b * ( (a+b-1)/b) )

#define ENINDEX_TRIPLE(ptr) \
    ((ptr) == NULL ? (TRIPLE*)(-1) \
	: (TRIPLE*)(hdr.triple_offset + (ptr)->tripleno*sizeof(TRIPLE)))

#define ENINDEX_BLOCK(ptr) \
    ((ptr) == NULL ? (BLOCK*)(-1) \
	: (BLOCK*)(hdr.block_offset + (ptr)->blockno*sizeof(BLOCK)))

#define ENINDEX_LEAF(ptr) \
    ((ptr) == NULL ? (LEAF*)(-1) \
	: (LEAF*)(hdr.leaf_offset + (ptr)->leafno*sizeof(LEAF)))

#define ENINDEX_SEGMENT(ptr) \
    ((ptr) == NULL ? (SEGMENT*)(-1) \
	: (SEGMENT*)(hdr.seg_offset + (ptr)->segno*sizeof(SEGMENT)))

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * write out the data representing the current routine to an IR file.
 * First determine how big the data will be by counting everything except
 * list blocks, renumbering structures as we go.  From this we can calculate
 * the offset at which triples will start, where blocks will start, etc.
 * Then re-initialize all IR structures for the next routine.
 */
void
ir_write_file(irfile, opt_level)
    FILE *irfile;		/* ir file, assumed open */
    int opt_level;
{
    if (optimize >= IROPT_ALIAS_LEVEL) {
	aliaser = 1;
    }
    if (!aliaser) {
	fill_can_access();
    }
    if (ir_debug) {
	dump_segments();
	dump_blocks();
	dump_leaves();
	dump_triples();
    }
    hdr.major_vers = IR_MAJOR_VERS;
    hdr.minor_vers = IR_MINOR_VERS;
    hdr.nptrs = npointers;
    hdr.nels = naliases;
    hdr.opt_level = opt_level;
#ifdef CG_PROC
    if (opt_level < PARTIAL_OPT_LEVEL) {
	cg_proc(hdr, nleaves, first_leaf, first_seg, first_block);
	free_all();
	return;
    }
#endif
    start = ftell(irfile);
    find_offsets();
    hdr.procname = (char*)enindex_stringptr(hdr.procname);
    hdr.source_file = (char*)enindex_stringptr(hdr.source_file);
    top_list = hdr.list_offset;
    bot_list = top_list;

    /* save space for the header - don't know nlists yet*/
    chk_fseek(irfile, sizeof(HEADER),1); 
    write_triples(irfile);
    write_blocks(irfile);
    write_leaves(irfile);
    write_segments(irfile);
    write_strings(irfile);
    write_lists(irfile);

    free_all();

    chk_fseek(irfile, start, 0);
    hdr.proc_size = top_list;
    hdr.file_status = UNOPTIMIZED;
    hdr.lang = C;
    chk_fwrite(&hdr,sizeof(HEADER),1,irfile);
    bzero(&hdr, sizeof(hdr));
    chk_fseek(irfile, start+top_list, 0);
}

void
chk_fseek(f, offset, from)
    FILE *f;
    long offset;
    int from;
{
    if (fseek(f, offset, from) == -1) {
	quita(stderr, "write_irfile: %s\n", sys_errlist[errno]);
	/*NOTREACHED*/
    }
}

void
chk_fwrite(buf, size, n, f)
    char *buf;
    int size, n;
    FILE *f;
{
    if (fwrite(buf, size, n, f) != n) {
	quita(stderr, "write_irfile: %s\n", sys_errlist[errno]);
	/*NOTREACHED*/
    }
}

static
find_offsets()
{
    BLOCK *bp;
    TRIPLE *tp, *first;
    LEAF *leafp;
    int charno, tripleno, leafno, blockno;
    STRING_BUFF *sbp;

    charno =  tripleno =  leafno =  blockno = 0;

    hdr.triple_offset = sizeof(HEADER);
    for(bp=first_block; bp; bp=bp->next) {
	first = (TRIPLE*) NULL;
	if(bp->last_triple) first=bp->last_triple->tnext;
	TFOR(tp,first){
	    if(ISOP(tp->op,NTUPLE_OP)) {
		register TRIPLE *tp3, *tp2 = (TRIPLE*) tp->right;
		TFOR(tp3, tp2) {
		    tp3->tripleno = tripleno++;
		}
	    }
	    tp->tripleno = tripleno++;
	}
    }
    if(tripleno != ntriples) {
	quita("find_offsets: ntriples wrong");
    }

    hdr.block_offset = hdr.triple_offset + tripleno*sizeof(TRIPLE);
    for(bp=first_block; bp; bp = bp->next) {
	bp->blockno = blockno++;
    }
    if(blockno != nblocks) {
	quita("find_offsets: nblocks wrong");
    }

    hdr.leaf_offset = hdr.block_offset + nblocks*sizeof(BLOCK);
    for(leafp=first_leaf; leafp; leafp = leafp->next_leaf) {
	leafp->leafno = leafno++;
    }
    if(leafno != nleaves) {
	quita("find_offsets: nleaves wrong");
    }

    hdr.seg_offset = hdr.leaf_offset + nleaves*sizeof(LEAF);
    hdr.string_offset = hdr.seg_offset + nsegs*sizeof(SEGMENT);
    for(sbp=first_string_buff; sbp; sbp=sbp->next) {
	charno += (sbp->top - sbp->data);
    }

    hdr.list_offset = hdr.string_offset + roundup(charno,sizeof(long));
}

static void
write_segments(irfile)
    FILE *irfile;
{
    register SEGMENT *seg, *next_seg;

    for(seg = first_seg; seg; seg = next_seg) {
	seg->name = (char*) enindex_stringptr(seg->name);
	next_seg = seg->next_seg;
	seg->next_seg = ENINDEX_SEGMENT(next_seg);
	chk_fwrite((char*)seg, sizeof(SEGMENT), 1, irfile);
    }
}

static void
write_strings(irfile)
    FILE *irfile;
{
    STRING_BUFF *sbp;
    int n, strtabsize;
    long zeroes = 0x00000000;

    strtabsize =0;
    for(sbp=first_string_buff; sbp; sbp=sbp->next) {
	if( n = sbp->top - sbp->data ) {
	    strtabsize += n;
	    chk_fwrite(sbp->data, sizeof(char), n, irfile);
	}
    }
    if(n=(strtabsize%sizeof(long))) {
	chk_fwrite(&zeroes,sizeof(char),sizeof(long)-n,irfile);
    }
}

static void
write_triples(irfile)
    FILE *irfile;
{
    BLOCK *bp;
    register TRIPLE *first, *tp;
    TRIPLE *tp_tnext;
    register TRIPLE *tpr;
    TRIPLE  *tpr_tnext;

    for(bp=first_block; bp; bp=bp->next) {
	first = (TRIPLE*) NULL;
	if(bp->last_triple) first=bp->last_triple->tnext;
	for(tp=first; tp; tp = (tp_tnext == first ? TNULL : tp_tnext)){
	    if(ISOP(tp->op,NTUPLE_OP)) {
		tpr=(TRIPLE*)tp->right;
		while(tpr != TNULL) {
		    tpr->left = (IR_NODE *) enindex_node(tpr->left);
		    tpr->right = (IR_NODE *) enindex_node(tpr->right);
		    tpr->tprev = (TRIPLE *) ENINDEX_TRIPLE(tpr->tprev);
		    tpr_tnext = tpr->tnext;
		    tpr->tnext = (TRIPLE *) ENINDEX_TRIPLE(tpr->tnext);
		    tpr->can_access =(LIST *)enindex_list(irfile,tpr->can_access);
		    chk_fwrite(tpr,sizeof(TRIPLE),1,irfile);
		    if (tpr_tnext == (TRIPLE*)tp->right) {
			tpr = TNULL;
		    } else {
			tpr = tpr_tnext;
		    }
		}
	    }
	    tp->left = (IR_NODE *) enindex_node(tp->left);
	    tp->right = (IR_NODE *) enindex_node(tp->right);
	    tp->tprev = (TRIPLE *) ENINDEX_TRIPLE(tp->tprev);
	    tp_tnext = tp->tnext;
	    tp->tnext = (TRIPLE *) ENINDEX_TRIPLE(tp->tnext);
	    tp->can_access = (LIST *) enindex_list(irfile,tp->can_access);
	    chk_fwrite(tp,sizeof(TRIPLE),1,irfile);
	}
    }
}

static void
write_blocks(irfile)
    FILE *irfile;
{
    BLOCK *bp, *bp_next;

    for(bp=first_block; bp; bp=bp_next) {
	bp->entryname = (char *) enindex_stringptr(bp->entryname);
	bp->last_triple = ENINDEX_TRIPLE(bp->last_triple);
	bp->pred = (LIST*) enindex_list(irfile,bp->pred);
	bp->succ = (LIST*) enindex_list(irfile,bp->succ);
	bp_next = bp->next;
	bp->next = (BLOCK*) ENINDEX_BLOCK(bp->next);
	chk_fwrite(bp,sizeof(BLOCK),1,irfile);
    }
}

static void
write_leaves(irfile)
    FILE *irfile;
{
    LEAF *p, *p_next;

    for(p = first_leaf; p ; p = p_next ) {
	if(p->class == VAR_LEAF || p->class == ADDR_CONST_LEAF)  {
	    p->val.addr.seg = ENINDEX_SEGMENT(p->val.addr.seg);
	} else {
	    if(p->val.const.isbinary == IR_TRUE) {
		p->val.const.c.bytep[0] = 
		    (char *) enindex_stringptr(p->val.const.c.bytep[0]);
	    } else if(IR_ISCHAR(p->type.tword) || IR_ISPTRFTN(p->type.tword)) {
		p->val.const.c.cp = 
		    (char *) enindex_stringptr(p->val.const.c.cp);
	    } else if( PCC_ISFLOATING(p->type.tword) ) {
		p->val.const.c.fp[0] = 
		    (char *) enindex_stringptr(p->val.const.c.fp[0]);
	    }
	}
	p_next = p->next_leaf;
	p->next_leaf = ENINDEX_LEAF( p->next_leaf );
	p->pass1_id = (char *) enindex_stringptr(p->pass1_id);
	p->overlap = (LIST *) enindex_list(irfile,p->overlap);
	p->neighbors = (LIST *) enindex_list(irfile,p->neighbors);
	p->addressed_leaf = ENINDEX_LEAF( p->addressed_leaf );
	chk_fwrite(p,sizeof(LEAF),1,irfile);
    }
}

static int
enindex_node(p)
    register IR_NODE *p;
{
    if (p != NULL) {
	switch(p->operand.tag) {
	case ISTRIPLE:
	    return( hdr.triple_offset + ((TRIPLE*)p)->tripleno*sizeof(TRIPLE) );
	case ISLEAF:
	    return( hdr.leaf_offset + ((LEAF*)p)->leafno*sizeof(LEAF) );
	case ISBLOCK:
	    return( hdr.block_offset + ((BLOCK*)p)->blockno*sizeof(BLOCK) );
	default:
	    quita("enindex_node: bad tag");
	}
    }
    return(-1);
}

/*
 * return buffer space for an enindexed copy of a LIST node
 */
static LIST *
get_list_buffer(irfile)
    FILE *irfile;
{
    LIST_BUFFER *new_list_buffer;

    if (last_list_buffer == NULL || last_list_buffer->nlists == LIST_BUFSIZ) {
	if (n_list_buffers == MAX_LIST_BUFFERS) {
	    long current_offset = ftell(irfile);
	    write_lists(irfile);
	    chk_fseek(irfile, current_offset, 0);
	}
	new_list_buffer = (LIST_BUFFER*)malloc((unsigned)sizeof(LIST_BUFFER));
	if (new_list_buffer == NULL) {
	    quita("enindex_list: out of memory\n");
	    /*NOTREACHED*/
	}
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
write_lists(irfile)
    FILE *irfile;
{
    LIST_BUFFER *bp, *nextbp;

    bp = first_list_buffer;
    chk_fseek(irfile, start+bot_list, 0);
    while(bp != NULL) {
	chk_fwrite((char*)bp->buf, sizeof(LIST), bp->nlists, irfile);
	bot_list += sizeof(LIST)*bp->nlists;
	nextbp = bp->nextbuf;
	free((char*)bp);
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
enindex_list(irfile,list)
    FILE *irfile;
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
	newl = get_list_buffer(irfile);
	newl->next = (LIST*) (top_list + sizeof(LIST));
	newl->datap = (LDATA*) enindex_node((IR_NODE*)lp->datap);
	top_list += sizeof(LIST);
    }
    newl->next = (LIST*)first_offset;
    return first_offset;
}

/*
 * turn a pointer to a string in a string_buffer into an ir file offset
 */
static int
enindex_stringptr(cp)
    register char *cp;
{
    register STRING_BUFF *sbp;
    register int offset;

    if(!cp) return(-1);

    offset = hdr.string_offset;
    for(sbp = first_string_buff;sbp;sbp=sbp->next) {
	if(cp >= sbp->data && cp < sbp->top ) {
	    return( offset + (cp - sbp->data));
	}
	offset += (sbp->top - sbp->data);
    }
    quita("enindex_stringptr : string not in a STRING_BUFF");
}

static void
free_all()
{
    free_triples();
    free_blocks();
    free_leaves();
    free_segments();
    free_strings();
    free_lists();
    free_chains();
}
