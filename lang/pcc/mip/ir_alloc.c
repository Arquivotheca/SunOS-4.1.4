#ifndef lint
static char sccsid[] = "@(#)ir_alloc.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "ir_misc.h"

/*
 * Storage allocation routines.
 *
 * Fixed-size nodes are handled by a layer built on top of malloc(3)
 * and coded for locality of references to the "mainline" IR structures.
 * This also provides an efficient means of deallocating all IR data
 * at the end of the procedure.
 *
 * Strings are handled by a similar mechanism, although with a slightly
 * different structure to handle variable lengths.
 *
 * Note that chaos will result if an attempt is made to free anything
 * on its own.
 */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * the data structure for well-behaved (constant size) nodes
 */

typedef struct pool {
    struct pool *next;		/* next block in the pool, if any */
    int count;
    char *nextblk;
    /* plus a lot of other garbage */
} POOL;

#define POOLCOUNT 200		/* number of records/block in a pool */

static POOL *triple_pool;
static POOL *leaf_pool;
static POOL *block_pool;
static POOL *segment_pool;
static POOL *list_pool;
static POOL *chain_pool;

/* hash table for strings */

# define STRING_HASH_SIZE 307

static CHAIN *string_hash_tab[STRING_HASH_SIZE];

/*
 * list headers -- the active members of each data type
 * are kept on a FIFO list, whose head and tail are
 * kept in static variables of the appropriate type.
 */

STRING_BUFF *first_string_buff, *string_buff;
LEAF	*first_leaf, *last_leaf;
BLOCK	*first_block, *current_block;
SEGMENT	*first_seg, *last_seg;
static	LIST *recycled_list;

/* allocation counters */
int nsegs, nleaves, ntriples, nblocks, nlists, npointers, naliases;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* from libc */

extern char *malloc(/*nbytes*/);
extern void free(/*p*/);

/* from ir_misc */

extern	BOOLEAN ir_chk_ovflo();

/* export */

IR_NODE	*new_triple(/*op,arg1,arg2,type*/);
BLOCK	*new_block(/*s, labelno, is_entry, is_global*/);
LEAF	*new_leaf(/*class*/);
SEGMENT	*new_segment(/*segname, segdescr, segbase*/);
char	*new_string(/*string*/);
char	*add_bytes(/*strp,len*/);
LIST	*new_list();
void	recycle_list(/*listnode*/);
CHAIN	*new_chain(/*item,link*/);
void	free_triples();
void	free_blocks();
void	free_leaves();
void	free_segments();
void	free_strings();
void	free_lists();
void	free_chains();


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * allocate size bytes from a pool, adding a
 * new block to the pool if necessary
 */
static char *
new(pp, size)
    POOL **pp;
    unsigned size;
{
    register POOL *p,*q;
    char *retblk;

    p = *pp;
    if (p == NULL || p->count == 0) {
	q = (POOL*)malloc(POOLCOUNT*size + sizeof(POOL));
	if (q == NULL) {
		quita("memory allocation exceeded");
		/*NOTREACHED*/
	}
	q->next = p;
	q->count = POOLCOUNT;
	q->nextblk = (char*)q + sizeof(POOL);
	p = q;
	*pp = p;
    }
    (p->count)--;
    retblk = p->nextblk;
    p->nextblk += size;
    bzero(retblk, size);
    return retblk;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* free all of the blocks in a pool */

static void
free_pool(pp)
    POOL **pp;
{
    register POOL *p,*q;

    p = *pp;
    while (p != NULL) {
	q = p->next;
	free((char*)p);
	p = q;
    }
    *pp = NULL;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

IR_NODE *
new_triple(op,arg1,arg2,type)
    IR_OP op;
    IR_NODE *arg1,*arg2;
    TYPE type;
{
    register TRIPLE *p;

    p = (TRIPLE*)new(&triple_pool, sizeof(TRIPLE));
    p->tag=ISTRIPLE;
    p->op=op;
    p->left= arg1;
    p->right= arg2;
    p->type = type;
    p->tripleno = ntriples++;
    p->tprev = p->tnext = p;
    p->visited = TRUE;
    p->chk_ovflo = ir_chk_ovflo();

    switch(op) {
    case IR_LABELREF: 
    case IR_PARAM:
	break;

    default:
	TAPPEND(current_block->last_triple,p);
    }
    if (ir_debug > 3) {
	print_triple(p,0);
    }
    return((IR_NODE*)p);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

LEAF *
new_leaf(class)
    LEAF_CLASS class;
{
    register LEAF *p;

    p = (LEAF*)new(&leaf_pool, sizeof(LEAF));
    p->tag = ISLEAF;
    p->class = class;
    p->leafno = nleaves++;
    p->pointerno = -1;
    p->elvarno = -1;
    p->next_leaf = NULL;
    if (last_leaf == NULL) {
	first_leaf = last_leaf = p;
    } else {
	last_leaf->next_leaf = p;
	last_leaf = p;
    }
    return(p);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

LIST *
new_list()
{
    register LIST *p;

    if (recycled_list == NULL) {
	p= (LIST*)new(&list_pool, sizeof(LIST));
	nlists++;
    } else {
	p = recycled_list;
	recycled_list = recycled_list->next;
    }
    p->next = p;
    return(p);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void
recycle_list(listnode)
    LIST *listnode;
{
    listnode->next = recycled_list;
    recycled_list = listnode;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

BLOCK *
new_block(s, labelno, is_entry, is_global)
    char *s;
    int labelno;
    BOOLEAN is_entry;
    BOOLEAN is_global;
{
    register BLOCK *p;

    p= (BLOCK*)new(&block_pool, sizeof(BLOCK));
    p->tag=ISBLOCK;
    if (current_block == NULL) {
	first_block = current_block = p;
    } else {
	current_block->next = p;
	current_block = p;
    }
    p->blockno = nblocks++;
    if (s != NULL) {
	p->entryname = new_string(s);
    }
    p->labelno = labelno;
    p->is_ext_entry = is_entry;
    p->entry_is_global = is_global;
    (void)ir_labeldef(p->labelno);
    return p ;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

SEGMENT *
new_segment(segname, segdescr, segbase)
    char *segname;
    struct segdescr_st *segdescr;
    int segbase;
{
    register SEGMENT *p;

    p = (SEGMENT*)new(&segment_pool, sizeof(SEGMENT));
    if (last_seg == NULL) {
	first_seg = last_seg = p;
    } else {
	last_seg->next_seg = p;
	last_seg = p;
    }
    p->name = new_string(segname);
    p->descr = *segdescr;
    p->base = segbase;
    p->segno = nsegs++;
    p->len = 0;
    return p;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * return a new chain node, initialized with an item pointer
 * (pointer to anything) and a link (pointer to another chain node).
 * This is useful for lists that don't have to be written to the
 * IR file (e.g., hash chains)
 */

CHAIN *
new_chain(item, link)
    char *item;
    CHAIN *link;
{
    register CHAIN *r;
    r = (CHAIN*)new(&chain_pool, sizeof(CHAIN));
    r->datap = item;
    r->nextp = link;
    return r;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * add bytes to the current string buffer.  Get a new one if needed.
 */
char *
add_bytes(strp,len)
    register char *strp;
    register int len;
{
    int size;
    STRING_BUFF *tmp;
    register char *start, *cursor;

    if( !string_buff || ((string_buff->top + len -1) >= string_buff->max)) {
	size = ( len > STRING_BUFSIZE ? len : STRING_BUFSIZE);
	tmp = (STRING_BUFF*) malloc(sizeof(STRING_BUFF)+size);
	if (tmp == NULL) {
		quita("memory allocation exceeded");
		/*NOTREACHED*/
	}
	tmp->next = (STRING_BUFF*) NULL;

	if(!string_buff) {
	    first_string_buff = tmp;
	} else {
	    string_buff->next = tmp;
	}
	string_buff = tmp;

	string_buff->data= string_buff->top = (char*)tmp + sizeof(STRING_BUFF);
	string_buff->max = string_buff->top + size;
    }
    cursor=string_buff->top;
    start = cursor;
    string_buff->top += len;
    while( len-- > 0) *cursor++ = *strp++;
    return start ;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * allocate string buffer storage.  strings of length < 64 are
 * hashed, other strings are just copied into buffers.
 */
char *
new_string(strp)
    register char *strp;
{
    register int len;
    register unsigned hash_index;
    CHAIN *cp;
    char *start;
    int i;

    len = strlen(strp)+1;
    if(len < 64) {
	hash_index = 0;
	for (i = 0; i < len ; i++) {
	    hash_index = (hash_index<<1) + ((unsigned char*)strp)[i];
	}
	hash_index %= STRING_HASH_SIZE;
	for(cp=string_hash_tab[hash_index];cp;cp=cp->nextp) {
	    if(cp->datap[0] == strp[0] && strncmp(cp->datap,strp,len) == 0)
		return((char*)cp->datap); 
	}
	start=add_bytes(strp,len);
	string_hash_tab[hash_index]= new_chain(start,string_hash_tab[hash_index]);
    } else {
	start=add_bytes(strp,len);
    }
    return start ;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void free_triples()
{
    free_pool(&triple_pool); 
    ntriples = 0;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void free_lists()
{
    free_pool(&list_pool); 
    nlists = 0;
    recycled_list = NULL;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void free_blocks()
{
    free_pool(&block_pool); 
    first_block = NULL;
    current_block = NULL;
    nblocks = 0;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void free_chains()
{
    free_pool(&chain_pool);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void free_leaves()
{
    register i;

    free_pool(&leaf_pool); 
    first_leaf = NULL;
    last_leaf = NULL;
    nleaves = 0;
    for(i=0; i < LEAF_HASH_SIZE; i++) {
	leaf_hash_tab[i] = NULL;
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void free_strings()
{
    register i;
    register STRING_BUFF *p, *q;

    p = first_string_buff;
    while(p != NULL) {
	q = p->next;
	free(p);
	p = q;
    }
    first_string_buff = NULL;
    string_buff = NULL;
    for(i = 0; i < STRING_HASH_SIZE; i++) {
	string_hash_tab[i] = NULL;
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void free_segments()
{
    register i;

    free_pool(&segment_pool);
    first_seg = NULL;
    last_seg = NULL;
    for(i = 0; i < SEG_HASH_SIZE; i++) {
	seg_hash_tab[i] = NULL;
    }
    nsegs = 0;
}
