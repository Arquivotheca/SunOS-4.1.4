#ifndef lint
static  char sccsid[] = "@(#)read_ir.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "page.h"
#include "misc.h"
#include "cf.h"
#include "read_ir.h"
#include "complex_expansion.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/uio.h>

/* global */

STRING_BUFF	*string_buff;
STRING_BUFF	*first_string_buff;
BLOCK		*entry_tab;
BLOCK		*entry_top;

/* local */

static int	nlists, strtabsize;
static char	*string_tab,*string_top;
static LIST	*list_tab, *list_top;
static char	*deindex_ldata(/*index*/);
static void	launder_strings(/*irfd*/);
static void	launder_segs(/*irfd*/);
static void	launder_lists(/*irfd*/);
static void	launder_leaves(/*irfd*/);
static void	launder_entries(/*irfd*/);
static void	make_null_entry();
static void	launder_triples(/*irfd*/);
static void	ir_setup();
static void	sort_append(/*seg, item*/);

#define NULL_INDEX -1
#define SIZEOF_SHORT_TRIPLE (sizeof(TRIPLE) - IROPT_TRIPLE_FIELDS*sizeof(char*) )
#define SIZEOF_SHORT_LEAF (sizeof(LEAF) -  IROPT_LEAF_FIELDS*sizeof(char*))
#define SIZEOF_SHORT_BLOCK (sizeof(BLOCK) - IROPT_BLOCK_FIELDS*sizeof(char*))

#define DEINDEX_BLOCK(index) ((BLOCK*)( (int)(index) == NULL_INDEX ? NULL : \
	(((int)(index) - hdr.block_offset) + (char*)entry_tab)))

#define DEINDEX_TRIPLE(index) ((TRIPLE*)( (int)(index) == NULL_INDEX ? NULL : \
	(((((int)(index) - hdr.triple_offset)/SIZEOF_SHORT_TRIPLE)*sizeof(TRIPLE))\
		+ (char*) triple_tab)))

#define DEINDEX_LEAF(index) ((LEAF*)( ( (int)(index) == NULL_INDEX ? NULL : \
	((((int)(index) - hdr.leaf_offset)/SIZEOF_SHORT_LEAF)*sizeof(LEAF))\
		+ (char*) leaf_tab)) )

#define DEINDEX_NODE(index) (((int)(index) < hdr.block_offset ?\
	(IR_NODE*) DEINDEX_TRIPLE(index) :\
	(IR_NODE*) DEINDEX_LEAF(index)))

#define DEINDEX_LIST(index) ((LIST*)((int)(index) == NULL_INDEX ? NULL : \
	(((int)(index) - hdr.list_offset) + (char*) list_tab)))

#define DEINDEX_STRING(index) ((char*)((int)(index) == NULL_INDEX ? NULL : \
	(((int)(index) - hdr.string_offset) + (char*) string_tab)))

#define DEINDEX_SEG(index) ((SEGMENT*)((int)(index) == NULL_INDEX ? NULL : \
	(((int)(index) - hdr.seg_offset) + (char*) seg_tab)))

/* a list datap item could be pointing at anything ...*/

static char *
deindex_ldata(index)
	long index;
{
	if( index == NULL_INDEX) {
		return NULL;
	} else if(index < hdr.block_offset ) {
		return ((char*) DEINDEX_TRIPLE(index));
	} else if(index < hdr.leaf_offset ) {
		return ((char*) DEINDEX_BLOCK(index));
	} else if(index < hdr.seg_offset ) {
		return ((char*) DEINDEX_LEAF(index));
	} else if(index < hdr.string_offset ) {
		return ((char*) DEINDEX_SEG(index));
	} else if(index < hdr.list_offset ) {
		return ((char*) DEINDEX_STRING(index));
	} else if(index < hdr.proc_size ) {
		return ((char*) DEINDEX_LIST(index));
	} else {
		quit("deindex_ldata: index out of range");
	}
	/*NOTREACHED*/
}

BOOLEAN
read_ir(irfd)
	int irfd;
{
	unsigned nbytes;	/* size of IR structures from file, in bytes */
	char *irfile_space;	/* storage for IR structures */
	int proc_index;		/* file offset of procedure name */

	/* newer-than-expected IR files are an error; old ones are ok, at
	   least in 4.1. */
	if (hdr.major_vers!=IR_MAJOR_VERS || hdr.minor_vers > IR_MINOR_VERS) {
		quita("iropt: IR version = %d.%d, expected %d.%d\n",
		    hdr.major_vers, hdr.minor_vers,
		    IR_MAJOR_VERS, IR_MINOR_VERS);
		/*NOTREACHED*/
	}
	ntriples = (hdr.block_offset - hdr.triple_offset)/ SIZEOF_SHORT_TRIPLE;
	nblocks = (hdr.leaf_offset - hdr.block_offset)/ SIZEOF_SHORT_BLOCK;
	nleaves = (hdr.seg_offset - hdr.leaf_offset)/ SIZEOF_SHORT_LEAF;
	nseg = (hdr.string_offset - hdr.seg_offset)/ sizeof(SEGMENT);
	strtabsize = (hdr.list_offset - hdr.string_offset)/sizeof(char);
	nlists = (hdr.proc_size - hdr.list_offset)/ sizeof(LIST);
	npointers = hdr.nptrs;
	nelvars = hdr.nels;
	unknown_control_flow = IR_FALSE;
	regs_inconsistent = IR_FALSE;
	proc_index = (int) hdr.procname;
	hdr.procname = NULL;

	nbytes  = ntriples*sizeof(TRIPLE);
	nbytes += nblocks*sizeof(BLOCK);
	nbytes += nleaves*sizeof(LEAF);
	nbytes += nseg*sizeof(SEGMENT);
	nbytes += strtabsize;
	nbytes += nlists*sizeof(LIST);
	irfile_space = heap_setup(nbytes);
	/*
	 * triples
	 */
	if(ntriples) {
		triple_tab = (TRIPLE*) irfile_space;
		irfile_space += ntriples*sizeof(TRIPLE);
		triple_top = &triple_tab[ntriples];
	} else {
		triple_top = triple_tab = (TRIPLE*) NULL;
	}
	/*
	 * blocks
	 */
	if(nblocks) {
		entry_tab = (BLOCK*) irfile_space;
		irfile_space += nblocks*sizeof(BLOCK);
		entry_top = &entry_tab[nblocks];
	} else {
		entry_top = entry_tab = (BLOCK*) NULL;
	}
	/*
	 * leaves
	 */
	if(nleaves) {
		leaf_tab = (LEAF*) irfile_space;
		irfile_space += nleaves*sizeof(LEAF);
	} else {
		leaf_top = leaf_tab = (LEAF*) NULL;
	}
	/*
	 * segments
	 */
	if(nseg) {
		seg_tab = (SEGMENT*) irfile_space;
		irfile_space += nseg*sizeof(SEGMENT);
	} else {
		seg_tab = (SEGMENT*) NULL;
	}
	/*
	 * strings
	 */
	if(strtabsize) {
		string_tab = (char*) irfile_space;
		irfile_space += strtabsize;
		string_top = &string_tab[strtabsize];
	} else {
		string_top = string_tab = (char*) NULL;
	}
	/*
	 * lists
	 */
	if(nlists) {
		list_tab = (LIST*) irfile_space;
		irfile_space += nlists*sizeof(LIST);
		list_top = &list_tab[nlists];
	} else {
		list_top = list_tab = LNULL;
	}
	hdr.procname = DEINDEX_STRING(proc_index);
	hdr.source_file = DEINDEX_STRING(hdr.source_file);

	proc_init();

	if(ntriples) launder_triples(irfd);
	if(nblocks) launder_entries(irfd);
	if(nleaves) launder_leaves(irfd);
	if(nseg) launder_segs(irfd);
	launder_strings(irfd);
	if(nlists) launder_lists(irfd);

	if(nleaves == 0 || ntriples ==0 || nblocks == 0 || ext_entries == LNULL)
	{
		return IR_FALSE ;
	}
	ir_setup();
	return IR_TRUE ;
}

static void
launder_strings(irfd)
{
	if(strtabsize) {
		if( read(irfd, string_tab, strtabsize ) != strtabsize ) {
			perror("");
			quit("read_ir: read error");
		}
	}
	/* allocate an intial string_buffer for the strings read in */
	first_string_buff=string_buff=(STRING_BUFF*) ckalloc(1,sizeof(STRING_BUFF));
	string_buff->next = (STRING_BUFF*) NULL;
	string_buff->data = string_tab;
	string_buff->max = string_buff->top = string_top;
}

static void
launder_segs(irfd)
{
	register SEGMENT *seg;
	int seg_tab_size;

	seg_tab_size = nseg*sizeof(SEGMENT);
	if( read(irfd, (char*)seg_tab, seg_tab_size) != seg_tab_size ) {
		perror("");
		quit("read_ir: read error");
	}
	for(seg=seg_tab; seg != NULL; seg = seg->next_seg) {
		seg->name = DEINDEX_STRING(seg->name);
		seg->leaves = LNULL;
		seg->next_seg = DEINDEX_SEG(seg->next_seg);
		last_seg = seg;
	}
}

static void
launder_lists(irfd)
{
	register LIST *p;
	int list_tab_size;

	list_tab_size = nlists*sizeof(LIST);
	if( read(irfd, (char*)list_tab, list_tab_size) != list_tab_size ) {
		perror("");
		quit("read_ir: read error");
	}
	for(p=list_tab; p < list_top; p ++) {
		p->next = DEINDEX_LIST(p->next);
		p->datap = (LDATA *) deindex_ldata((long)p->datap);
	}
}

static void
launder_leaves(irfd)
{ 
	register LEAF *p;
	register int i;
	char *short_leaf_tab;
	char *next_short_leaf;
	unsigned short_leaf_tab_size;

	short_leaf_tab_size = nleaves * SIZEOF_SHORT_LEAF;
	short_leaf_tab = heap_tmpalloc(short_leaf_tab_size);
	if( read(irfd, short_leaf_tab, (int)short_leaf_tab_size)
		!= short_leaf_tab_size) {
		perror("");
		quit("read_ir: read error");
	}
	leaf_top = (LEAF*) NULL;
	next_short_leaf = short_leaf_tab;
	for(i=0,p=leaf_tab; i< nleaves; i++,p++) {
		bcopy(next_short_leaf, (char*)p, SIZEOF_SHORT_LEAF);
		next_short_leaf += SIZEOF_SHORT_LEAF;
		if (i != p->leafno ) {
			quit("launder_leaves: leaves out of order in ir file");
			/*FIXME debugging test */
		}
		p->overlap = DEINDEX_LIST(p->overlap);
		p->neighbors = DEINDEX_LIST(p->neighbors);
		p->pass1_id = DEINDEX_STRING(p->pass1_id);
		if(leaf_top) {
			leaf_top->next_leaf = p;
		}
		leaf_top = p;
		p->next_leaf = (LEAF*) NULL;
		p->visited = IR_FALSE;
		p->addressed_leaf = DEINDEX_LEAF(p->addressed_leaf);
		if( p->class == VAR_LEAF || p->class == ADDR_CONST_LEAF ) {
			p->val.addr.seg = DEINDEX_SEG(p->val.addr.seg );
		} else {
			if(p->val.const.isbinary == IR_TRUE) {
				p->val.const.c.bytep[0]=DEINDEX_STRING(p->val.const.c.bytep[0]);
				if( ISCOMPLEX(p->type.tword)) {
					p->val.const.c.bytep[1] = 
						DEINDEX_STRING(p->val.const.c.bytep[1]);
				}
			} else if(ISCHAR(p->type.tword) || ISPTRFTN(p->type.tword) )  {
				p->val.const.c.cp = DEINDEX_STRING( p->val.const.c.cp );
			} else if( ISREAL(p->type.tword) ) {
				p->val.const.c.fp[0] = DEINDEX_STRING( p->val.const.c.fp[0] );
			} else if( ISCOMPLEX(p->type.tword)) {
				p->val.const.c.fp[0] = DEINDEX_STRING( p->val.const.c.fp[0] );
				p->val.const.c.fp[1] = DEINDEX_STRING( p->val.const.c.fp[1] );
			}
		}
	}
	heap_tmpfree(short_leaf_tab);
}
	
static void
launder_entries(irfd)
{
	register BLOCK *bp;
	register LIST *lp;
	char *short_block_tab;
	char *next_short_block;
	unsigned short_block_tab_size;

	short_block_tab_size = nblocks * SIZEOF_SHORT_BLOCK;
	short_block_tab = heap_tmpalloc(short_block_tab_size);
	if( read(irfd,short_block_tab, (int)short_block_tab_size)
		!= short_block_tab_size) {
		perror("");
		quit("read_ir: read error");
	}
	ext_entries = LNULL;
	next_short_block = short_block_tab;
	for(bp=entry_tab; bp < entry_top; bp ++) {
		bcopy(next_short_block, (char*)bp, SIZEOF_SHORT_BLOCK);
		next_short_block += SIZEOF_SHORT_BLOCK;
		bp->entryname = DEINDEX_STRING(bp->entryname);
		bp->last_triple = DEINDEX_TRIPLE(bp->last_triple);
		bp->pred = DEINDEX_LIST(bp->pred);
		bp->succ = DEINDEX_LIST(bp->succ);
		bp->loop_weight = 1;
		if(bp->is_ext_entry == IR_TRUE) {
			lp=NEWLIST(proc_lq);
			(BLOCK*) lp->datap = bp;
			LAPPEND(ext_entries,lp);
		} 
	}
	heap_tmpfree(short_block_tab);
}

static void
make_null_entry()
{
	TYPE type;
	TRIPLE *def;
	TRIPLE *refs;
	BLOCK *entry;
	LIST *lp;

	type.tword = UNDEF;
	type.size =0;
	type.align = 0;
	refs = TNULL;
	last_block = (BLOCK*) NULL;

	entry_block = new_block();
	def = append_triple(TNULL,
		IR_LABELDEF, (IR_NODE*)ileaf(0), (IR_NODE*)NULL, type);
	entry_block->last_triple = def;
	add_labelref((IR_NODE*)def);
	LFOR(lp,ext_entries) {
		entry = (BLOCK*) lp->datap;
		refs = append_triple(refs,
			IR_LABELREF, (IR_NODE*)ileaf(entry->labelno),
			(IR_NODE*)ileaf(0), type);
		add_labelref((IR_NODE*)refs);
	}
	entry_block->last_triple = append_triple(entry_block->last_triple,
		IR_SWITCH, (IR_NODE*)ileaf(0), (IR_NODE*)refs, type);
}

static void
launder_triples(irfd)
{
	register TRIPLE *p;
	char *short_triple_tab;
	char *next_short_triple;
	unsigned short_triple_tab_size;

	short_triple_tab_size = ntriples * SIZEOF_SHORT_TRIPLE;
	short_triple_tab = heap_tmpalloc(short_triple_tab_size);
	if(read(irfd,short_triple_tab, (int)short_triple_tab_size) 
		!= short_triple_tab_size  ) {
		perror("");
		quit("read_ir: read error");
	}
	next_short_triple = short_triple_tab;
	for(p=triple_tab; p < triple_top; p ++) {
		bcopy(next_short_triple, (char*)p, SIZEOF_SHORT_TRIPLE);
		next_short_triple += SIZEOF_SHORT_TRIPLE;
		p->left = DEINDEX_NODE(p->left);
		p->right = DEINDEX_NODE(p->right);
		p->tnext = DEINDEX_TRIPLE(p->tnext);
		p->tprev = DEINDEX_TRIPLE(p->tprev);
		p->can_access = DEINDEX_LIST(p->can_access);
		p->visited = IR_FALSE;
	}
	heap_tmpfree(short_triple_tab);
}

/*
 * additional house keeping that can be done only after other parts
 * of the ir file are available
 */
static void
ir_setup()
{
	register TRIPLE *p, *tlp;
	register LIST *lp;
	register LEAF *leafp;
	register SEGMENT *seg;
	register int index;
	register BLOCK *bp;

	/*
	 * set up leaf hash tab
	 */
	for(leafp=leaf_tab; leafp; leafp=leafp->next_leaf) {
		/*
		 * create hash table entry for leafp
		 */
		if( leafp->class == VAR_LEAF
		    || leafp->class == ADDR_CONST_LEAF ) {
			index=hash_leaf(leafp->class,
				(LEAF_VALUE*)&leafp->val.addr);
			seg = leafp->val.addr.seg;
			if(leafp->class == VAR_LEAF) {
				lp = NEWLIST(proc_lq);
				(LEAF*) lp->datap = leafp;
				if (seg == &seg_tab[ARG_SEGNO])
					sort_append(seg,lp);
				else
					LAPPEND(seg->leaves,lp);
			}
		} else { /* a CONST LEAF */
			if(leafp->type.tword == LABELNO) {
				add_labelref((IR_NODE*)leafp);
			}
			index=hash_leaf(CONST_LEAF,
				(LEAF_VALUE*)&leafp->val.const);
		}
		lp = NEWLIST(proc_lq);
		(LEAF*) lp->datap = leafp;
		LAPPEND(leaf_hash_tab[index],lp);
	}
	/* set up label ref/def structures for cf */
	for(p=triple_tab; p < triple_top; p ++) {

            /* determine if we will try to optimize complex operations */
            if (ISCOMPLEX(p->type.tword)) {
                there_are_complex_operations();
            }

		switch (p->op) {
		case IR_FCALL:
		case IR_SCALL:
			if (p->left->operand.tag == ISLEAF) {
				LEAF *leafp = (LEAF*)p->left;
				if (leafp->unknown_control_flow)
					unknown_control_flow = IR_TRUE;
				if (leafp->makes_regs_inconsistent)
					regs_inconsistent = IR_TRUE;
			}
			break;
		case IR_GOTO:	 
		case IR_SWITCH:
		case IR_CBRANCH:
		case IR_REPEAT:	 
			TFOR(tlp,(TRIPLE*)p->right) {
				add_labelref((IR_NODE*)tlp);
			}
			break;
		case IR_INDIRGOTO:	 
			TFOR(tlp,(TRIPLE*)p->right) {
				add_labelref((IR_NODE*)tlp);
			}
			if(indirgoto_targets == LNULL) {
				TFOR(tlp,(TRIPLE*)p->right) {
					(void)insert_list(&indirgoto_targets,
						(LDATA*)tlp, proc_lq);
				}
			}
			break;
		case IR_LABELDEF:
			add_labelref((IR_NODE*)p); 
			break;
		}
	}
	/*
	 * ensure blocks are in correct order
	 */
	if( ext_entries->next == ext_entries ) {
		entry_block = (BLOCK*) ext_entries->datap;
	} else {
		multientries = IR_TRUE;
		make_null_entry();
	}
	last_block = entry_block;
	entry_block->blockno = 0; /* relied on by various df routines */
	nblocks  = 1;
	for(bp=entry_tab; bp < entry_top; bp ++) {
		if(bp==entry_block) {
			continue;
		} else {
			bp->prev = last_block;
			last_block->next = bp;
			last_block = bp;
			bp->blockno = nblocks++;
		}
	}
	last_block->next = (BLOCK*) NULL;
	connect_labelrefs();
}
 
static void
sort_append(seg, item)
	SEGMENT *seg;
	LIST *item;
{
	register LIST *before, *after, *tmp1;
	BOOLEAN first = IR_TRUE;
 
	before = after = seg->leaves;
	while(after != LNULL) {
		if (((LEAF*)(item->datap))->val.addr.offset >
		    ((LEAF*)(after->datap))->val.addr.offset){
			first = IR_FALSE;
			before = after;
			after = after->next;
			if (after == seg->leaves) 
				goto l;
			else
				continue;
		}
	l:
		tmp1 = item->next;
		item->next = before->next;
		before->next = tmp1;
		if (first == IR_TRUE)
			seg->leaves = item;
		return;
	}
	seg->leaves = item;
}
