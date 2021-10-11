#ifndef lint
static	char sccsid[] = "@(#)debug.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

#include <stdio.h>
#include "iropt.h"
#include "recordrefs.h"
#include "make_expr.h"
#include "page.h"
#include "misc.h"
#include "setup.h"
#include "reg.h"
#include "loop.h"
#include "live.h"
#include "bit_util.h"
#include "debug.h"

/* from libc: used to measure the heap, NOT TO EXTEND IT. */
extern char *sbrk();

static char * seg_content_names[] = {
	"REG_SEG", "STG_SEG", "TEXT_SEG"
};

static char *base_type_names[] = {
	"undef",
	"farg",
	"char",
	"short",
	"int",
	"long",
	"float",
	"double",
	"strty",
	"unionty",
	"enumty",
	"moety",
	"uchar",
	"ushort",
	"unsigned",
	"ulong",
	"void",
	"bool",
	"extendedf",
	"complex",
	"dcomplex",
	"string",
	"labelno",
	"bitfield"
};

/* used in mem_stats */
static struct {
	char *name;
	int  count;
} var_ref_stats[] = {
	"VAR_DEF",	  0,
	"VAR_AVAIL_DEF",  0,
	"VAR_USE1",	  0,
	"VAR_USE2",	  0, 
	"VAR_EXP_USE1",	  0,
	"VAR_EXP_USE2",	  0,
	NULL
};

/* used in mem_stats */
static struct {
	char *name;
	int  count;
} expr_ref_stats[] = {
	"EXPR_GEN",	    0,
	"EXPR_AVAIL_GEN",   0,
	"EXPR_USE1",	    0,
	"EXPR_USE2",	    0,
	"EXPR_EXP_USE1",    0,
	"EXPR_EXP_USE2",    0,
	"EXPR_KILL",	    0,
	NULL
};

void
mem_stats(phase)
	int phase;
{
	static char fmt1[] = "%-16s	%8d (%8d bytes)\n";
	static char fmt2[] = "%-16s	%8d (%8d bytes) (%d implicit)\n";
	static char fmt3[] = "%-16s		 (%8d bytes)\n";
	static char fmt4[] = "%-16s	%8s %8s %8s\n";
	static char fmt5[] = "%-16s	%8d %8d %8d\n";
	EXPR_REF *rp;		/* pointers for traversing IR data structures */
	SEGMENT *segp;
	LEAF *leafp;
	BLOCK *bp;
	TRIPLE *tp, *tp2;
	LIST *lp, **lpp;
	EXPR *ep, **epp;
	VAR_REF *vrp;
	int i;
	int nedges;		/* counters for edges, blocks, triples, ... */
	int nblocks;
	int ntriples;
	int nimplicits;
	int nvarrefs;
	int navailvardefs;
	int var_df_sets;
	int nexprs;
	int expr_df_sets;
	int nexprrefs;
	int navailexprdefs;
	int nleaves;
	int nlists;
	int nsegments;
	int heap_est;
	int heap_used;

	/*
	 * count var_refs
	 */
	for( i = 0; var_ref_stats[i].name != NULL; i++) {
		var_ref_stats[i].count = 0;
	}
	nvarrefs = 0;
	for(vrp = var_ref_head; vrp; vrp = vrp->next) {
		nvarrefs++;
		var_ref_stats[(int)vrp->reftype].count++;
	}
	navailvardefs =  var_ref_stats[(int)VAR_AVAIL_DEF].count;
	navailvardefs = roundup((unsigned)navailvardefs, (unsigned)BPW);
	/*
	 * count expr_refs
	 */
	for( i = 0; expr_ref_stats[i].name != NULL; i++) {
		expr_ref_stats[i].count = 0;
	}
	nlists=0;
	nexprs = 0;
	nexprrefs = 0;
	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		for( ep = *epp; ep; ep = ep->next_expr ) {
			nexprs++;
			LFOR(lp, ep->depends_on) { nlists++; }
			for(rp = ep->references; rp ; rp = rp->next_eref) {
				nexprrefs++;
				expr_ref_stats[(int)rp->reftype].count++;
			}
		}
	}
	/*
	 * count lists, segments, leaves, blocks, triples, implicits
	 */
	nsegments=0;
	nleaves=0;
	nblocks=0;
	ntriples=0;
	nimplicits=0;
	nedges=0;
	LFOR(lp, edges) {
		nlists++;
		nedges++;
	}
	for(lpp = leaf_hash_tab; lpp < &leaf_hash_tab[LEAF_HASH_SIZE]; lpp++) {
		LFOR(lp, *lpp) { nlists++; }
	}
	for(segp=seg_tab; segp != NULL; segp = segp->next_seg) {
		nsegments++;
		LFOR(lp, segp->leaves) { nlists++; }
	}
	for(leafp = leaf_tab; leafp; leafp = leafp->next_leaf ) {
		nleaves++;
		LFOR(lp, leafp->overlap) { nlists++; }
		LFOR(lp, leafp->neighbors) { nlists++; }
		LFOR(lp, leafp->dependent_exprs) { nlists++; }
		LFOR(lp, leafp->kill_copy) { nlists++; }
	}
	for(bp=entry_block; bp; bp = bp->next) {
		nblocks++;
		LFOR(lp, bp->pred) { nlists++; }
		LFOR(lp, bp->succ) { nlists++; }
		LFOR(lp, bp->loops) { nlists++; }
		TFOR(tp, bp->last_triple) {
			ntriples++;
			if (tp->op == IR_IMPLICITUSE || tp->op == IR_IMPLICITDEF) {
				nimplicits++;
			}
			if( ISOP(tp->op, NTUPLE_OP)) {
				ntriples++;
				TFOR( tp2, (TRIPLE*) tp->right ) {
					LFOR(lp,tp2->can_access) { nlists++; }
					LFOR(lp,tp2->canreach) { nlists++; }
					LFOR(lp,tp2->reachdef1) { nlists++; }
					LFOR(lp,tp2->reachdef2) { nlists++; }
					LFOR(lp,tp2->implicit_use) { nlists++; }
					LFOR(lp,tp2->implicit_def) { nlists++; }
				}
			}
			LFOR(lp,tp->can_access) { nlists++; }
			LFOR(lp,tp->canreach) { nlists++; }
			LFOR(lp,tp->reachdef1) { nlists++; }
			LFOR(lp,tp->reachdef2) { nlists++; }
			LFOR(lp,tp->implicit_use) { nlists++; }
			LFOR(lp,tp->implicit_def) { nlists++; }
		}
	}
	for (tp=free_triple_lifo; tp; tp = (TRIPLE*)tp->left) {
		ntriples++;
	}
	for (bp=free_block_lifo; bp; bp = bp->next) {
		nblocks++;
	}
	/*
	 * compute estimate of total heap structures
	 */
	heap_est  = nblocks*sizeof(BLOCK);
	heap_est += nedges*sizeof(EDGE);
	heap_est += nlists*sizeof(LIST);
	heap_est += nsegments*sizeof(SEGMENT);
	heap_est += nleaves*sizeof(LEAF);
	heap_est += ntriples*sizeof(TRIPLE);
	heap_est += nexprs*sizeof(EXPR);
	/*
	 * Compute totals of data flow bit vectors.  Note that
	 * this figure overlaps with the list totals somewhat,
	 * since storage for {gen,kill} is freed before
	 * expanding the du/ud chains into lists.
	 */
	var_df_sets = ((nblocks*navailvardefs)/SZCHAR)*3; /*in,gen,kill*/
	heap_est += var_df_sets;
	if (nexprrefs) {
		/* available expressions: in,gen,kill,exprdef_mask */
		expr_df_sets = ((nblocks*nexprs)/SZCHAR)*4; 
		/* reaching defs: in, gen, kill, all_exprdef_mask */
		navailexprdefs = expr_ref_stats[(int)EXPR_AVAIL_GEN].count;
		navailexprdefs = roundup((unsigned)navailexprdefs, (unsigned)BPW);
		expr_df_sets += ((nblocks*navailexprdefs)/SZCHAR)*4;
	} else {
		expr_df_sets = 0;
	}
	heap_est += expr_df_sets;
	/*
	 * display var_ref stats
	 */
	printf("\n\n------------------ %s -------------------\n",
		optim_nametab[phase]);
	if (nvarrefs) {
		printf("\nvar_refs:\n");
		for( i = 0; var_ref_stats[i].name != NULL; i++) {
			if (var_ref_stats[i].count) {
				printf(fmt1, var_ref_stats[i].name,
				    var_ref_stats[i].count,
				    var_ref_stats[i].count*sizeof(VAR_REF));
			}
		}
		heap_est += nvarrefs*sizeof(VAR_REF);
		printf(fmt1, "total:", nvarrefs, nvarrefs*sizeof(VAR_REF));
	}
	/*
	 * display expr_ref stats
	 */
	if (nexprrefs) {
		printf("\nexpr_refs:\n");
		for( i = 0; expr_ref_stats[i].name != NULL; i++) {
			if (expr_ref_stats[i].count) {
				printf(fmt1, expr_ref_stats[i].name,
				    expr_ref_stats[i].count,
				    expr_ref_stats[i].count*sizeof(EXPR_REF));
			}
		}
		heap_est += nexprrefs*sizeof(EXPR_REF);
		printf(fmt1, "total:", nexprrefs, nexprrefs*sizeof(EXPR_REF));
	}
	/*
	 * display other heap structure stats
	 */
	printf("\nheap structure counts:\n");
	if (nblocks)
	    printf(fmt1, "blocks:", nblocks, nblocks*sizeof(BLOCK));
	if (nedges)
	    printf(fmt1, "edges:", nedges, nedges*sizeof(EDGE));
	if (nlists)
	    printf(fmt1, "lists:", nlists, nlists*sizeof(LIST));
	if (nsegments)
	    printf(fmt1, "segments:", nsegments, nsegments*sizeof(SEGMENT));
	if (nleaves)
	    printf(fmt1, "leaves:", nleaves, nleaves*sizeof(LEAF));
	if (nvarrefs)
	    printf(fmt1, "var_refs:", nvarrefs, nvarrefs*sizeof(VAR_REF));
	if (nexprrefs)
	    printf(fmt1, "expr_refs:",nexprrefs, nexprrefs*sizeof(EXPR_REF));
	if (nexprs)
	    printf(fmt1, "exprs:", nexprs, nexprs*sizeof(EXPR));
	if (ntriples)
	    printf(fmt2, "triples:", ntriples, ntriples*sizeof(TRIPLE),
		nimplicits);
	if (var_df_sets)
	    printf(fmt3, "var df sets:", var_df_sets);
	if (expr_df_sets)
	    printf(fmt3, "expr df sets:", expr_df_sets);
	/*
	 * if applicable, display reg_alloc stats
	 */
	if (phase == REG_ALLOC_PHASE) {
		int interference;	/* size of interference graph */
		int nreachdefs;		/* size of copy of reachdefs.out */
		int nlivedefs;		/* size of livedefs.{def,use,in,out} */
		int nsorts;		/* sloppiness from reg.c */
		nsorts = n_dsort + n_asort + n_fsort;
		interference = (nwebs*nwebs)/SZCHAR;
		nreachdefs = (nblocks*navailvardefs)/SZCHAR;
		nlivedefs = nreachdefs*3;	/* use[], def[], out[] */
		printf(fmt1, "webs:", nwebs, nwebs*sizeof(WEB));
		printf(fmt1, "sorts:", nsorts, nsorts*sizeof(SORT));
		printf(fmt1, "defs=>webs map:",nvardefs, nvardefs*sizeof(WEB*));
		printf(fmt3, "interference:", interference);
		printf(fmt3, "reachdefs sets:", nreachdefs);
		printf(fmt3, "livedefs sets:", nlivedefs);
		heap_est  += nwebs*sizeof(WEB);
		heap_est  += nsorts*sizeof(SORT);
		heap_est  += interference;
		heap_est  += nreachdefs;
		heap_est  += nlivedefs;
	}
	/*
	 * display bottom-line estimates and actual usage
	 */
	printf("\n");
	printf(fmt4, "total memory use:", "estimate", "actual", "missing");
	printf(fmt4, "-----------------", "--------", "------", "-------");
	heap_used = sbrk(0) - heap_start;
	printf(fmt5, "heap:", heap_est, heap_used, heap_used-heap_est);
	pagestats();
	printf("\f");
}

void
print_var_ref(rp)
	register VAR_REF *rp;
{
	if (rp == (VAR_REF*)NULL) {
		return;
	}
	printf("[%d]\t%s", rp->refno, var_ref_stats[ORD(rp->reftype)].name);
	if(!ISUSE(rp->reftype)) printf(" defno %d ",rp->defno);
	printf(" L[%d]",rp->leafp->leafno);
}

void
dump_var_refs()
{
	register VAR_REF *rp;

	printf("\nVAR_REFS :\n");
	for(rp = var_ref_head; rp; rp = rp->next) {
		print_var_ref(rp);
		printf(" at B[%d] T[%d]\n",
			rp->site.bp->blockno,rp->site.tp->tripleno);
	}
}

void
dump_expr_refs()
{
	EXPR_REF *rp;
	EXPR *ep, **epp;
	struct entry {
		EXPR_REF *rp;
		int exprno;
	} *ordered_refs, *entryp;

	if(nexprrefs ==  0)
		return;
	printf("\nEXPR_REFS :\n");
	ordered_refs = (struct entry *)
	    heap_tmpalloc((unsigned)(nexprrefs+1) * sizeof(struct entry));
	for(epp = expr_hash_tab; epp < &expr_hash_tab[EXPR_HASH_SIZE]; epp++) {
		for( ep = *epp; ep; ep = ep->next_expr ) {
			for(rp = ep->references; rp ; rp = rp->next_eref) {
# ifndef DEBUG
				/* refno field not compiled unless debugging */
				if(rp->reftype == EXPR_KILL) continue;
# endif
				ordered_refs[rp->refno].rp = rp;
				ordered_refs[rp->refno].exprno = ep->exprno;
			}
		}
	}
	ordered_refs[nexprrefs].rp = (EXPR_REF*) NULL;
	for(entryp = ordered_refs; entryp->rp ; entryp++ ) {
		rp = entryp->rp;
		printf("[%d]\t%s of V[%d] at B[%d] T[%d]",
			rp->refno, expr_ref_stats[ORD(rp->reftype)].name,
			entryp->exprno,
			rp->site.bp->blockno, rp->site.tp->tripleno);
		if( rp->reftype == EXPR_AVAIL_GEN)
			printf(" defno %d",rp->defno);
		printf("\n");
	}
	heap_tmpfree((char*)ordered_refs);
}

void
dump_byte_tab(header,tab,nrows,ncols)
	char *header, *tab;
	int nrows, ncols;
{
	int row, col, val;
	char *index = tab;

	printf("%s\n   ",header);
	for(col=0;col<ncols;col++) {
		printf("%2d ",col);
	}
	(void)putchar('\n');
	for(row=0;row<nrows;row++) {
		printf("%2d ",row);
		for(col=0;col<ncols;col++) {
			if(val = *(index++)) {
				printf("%2d ",val);
			} else {
				printf("   ");
			}
		}
		(void)putchar('\n');
	}
}

void
print_type(type)
	TYPE type;
{
	TWORD t;

	t=type.tword;
	for(;; t = DECREF(t) ){
			if( ISPTR(t) ) printf( " PTR" );
			else if( ISFTN(t) ) printf( " FTN" );
			else if( ISARY(t) ) printf( " ARY" );
			else {
				printf( " %s ", base_type_names[t]);
				return;
			}
	}
}

void
print_address(ap)
	ADDRESS *ap;
{
	SEGMENT *sp;

	sp=ap->seg;
	printf("%s",sp->name);
	printf("(%d,%d) ", sp->base,ap->offset);
	if(sp->descr.class == DATA_SEG) printf(" label %d",ap->labelno);
}

void
print_const_leaf(lp)
	LEAF *lp;
{ 
	if(ISPTRFTN(lp->type.tword)) {
		printf("%s",lp->val.const.c.cp);
	} else switch(BTYPE(lp->type.tword)) {
		case UCHAR:
		case CHAR:
		case STRING:
			printf("%s",lp->val.const.c.cp);
			break;
		case LABELNO:
		case SHORT:
		case USHORT:
		case INT:
		case UNSIGNED:
		case LONG:
		case ULONG:
			printf("%d",lp->val.const.c.i);
			break;
		case FLOAT:
			if(lp->val.const.isbinary == IR_TRUE) {
				printf("0x%X", l_align(lp->val.const.c.bytep[0]));
			} else {
				printf("%s",lp->val.const.c.fp[0]);
			}
			break;
		case DOUBLE:
			if(lp->val.const.isbinary == IR_TRUE) {
				printf("(0x%X,0x%X)", 
				 l_align(lp->val.const.c.bytep[0]),
				 l_align(lp->val.const.c.bytep[0]+4));
			} else {
				printf("%s",lp->val.const.c.fp[0]);
			}
			break;
		case COMPLEX:
			if(lp->val.const.isbinary == IR_TRUE) {
				printf("(0x%X,0x%X)", 
				 l_align(lp->val.const.c.bytep[0]),
				 l_align(lp->val.const.c.bytep[1]));
			} else {
				printf("(%s,%s)",
					lp->val.const.c.fp[0],lp->val.const.c.fp[1]);
			}
			break;
		case DCOMPLEX:
			if(lp->val.const.isbinary == IR_TRUE) {
				printf("(0x%X,0x%X , 0x%X,0x%X)", 
				 l_align(lp->val.const.c.bytep[0]),
				 l_align(lp->val.const.c.bytep[0]+4),
				 l_align(lp->val.const.c.bytep[1]),
				 l_align(lp->val.const.c.bytep[1]+4));
			} else {
				printf("(%s,%s)",
					lp->val.const.c.fp[0],lp->val.const.c.fp[1]);
			}
			break;
		case BOOL:
			printf("%s",(lp->val.const.c.i ? "true" : "false" ));
			break;
		default:
			printf("unknown leaf constant type");
			break;
	}
}

void
print_leaf(lp)
	LEAF *lp;
{
	register LIST *listp;
	VAR_REF *vrp;

	printf("[%d] %s size %d align %d ", lp->leafno,
		(lp->pass1_id ? lp->pass1_id : "\"\""),
		lp->type.size, lp->type.align );
	print_type(lp->type);
	switch(lp->class) {
	case ADDR_CONST_LEAF:
		printf("ADDR_CONST ");
		print_address(&lp->val.addr);
		break;
	case VAR_LEAF:
		printf("VAR ");
		print_address(&lp->val.addr);
		break;
	case CONST_LEAF:
		printf(" CONST ");
		print_const_leaf(lp);
		break;
	default:
		printf(" unknown leaf class");
		break;
	}
	if(lp->references) {
		printf(" references: ");
		for(vrp = lp->references; vrp ; vrp = vrp->next_vref) {
			printf("%d ", vrp->refno);
		}
	}
	if(lp->dependent_exprs) {
		printf(" dependent_exprs: ");
		LFOR(listp,lp->dependent_exprs->next) {
			printf("E[%d] ", LCAST(listp,EXPR)->exprno);
		}
	}
	if(lp->overlap) {
		printf(" overlap: ");
		LFOR(listp,lp->overlap) {
			printf("L[%d] ", LCAST(listp,LEAF)->leafno);
		}
	}
	if(lp->neighbors) {
		printf(" neighbors: ");
		LFOR(listp,lp->neighbors) {
			printf("L[%d] ", LCAST(listp,LEAF)->leafno);
		}
	}
	printf("\n");
}

/*
 * print an IR_NODE operand, in somewhat less detail than
 * a full table dump, but in somewhat more detail than
 * "L[100]" or "T[989]"
 */
void
print_operand(np)
	IR_NODE *np;
{
	LEAF *leafp;

	if (np == NULL) {
		return;
	}
	printf(" (");
	switch(np->operand.tag) {
	case ISLEAF:
		switch(np->leaf.class) {
		case CONST_LEAF:
			printf("CONST #");
			print_const_leaf((LEAF*)np);
			printf(" ");
			break;
		case ADDR_CONST_LEAF:
			printf("ADDR_CONST ");
			leafp = np->leaf.addressed_leaf;
			if (leafp != NULL && leafp->pass1_id != NULL) {
				printf("&%s ", leafp->pass1_id);
			}
			break;
		case VAR_LEAF:
			printf("VAR ");
			if (np->leaf.pass1_id != NULL) {
				printf("%s ", np->leaf.pass1_id);
			}
			break;
		}
		printf("L[%d]", np->leaf.leafno);
		break;
	case ISTRIPLE:
		printf("T[%d]", np->triple.tripleno);
		break;
	case ISBLOCK:
		printf("B[%d]", np->block.blockno);
		break;
	}
	printf(") ");
}

void
print_triple(tp, indent)
	register TRIPLE *tp;
	int indent;
{
	register LIST *lp;
	register TRIPLE *tlp;
	VAR_REF *vrp;
	int i;

	for(i=0;i<indent;i++){
		printf("\t");
	}

	printf("[%d] ", tp->tripleno );

	if( ISOP(tp->op,VALUE_OP)) {
		print_type(tp->type);
	} else {
		printf("\t");
	}
	printf("%s ",op_descr[ORD(tp->op)].name);
	if(tp->op == IR_PASS) {
		printf(" %s\n",tp->left->leaf.val.const.c.cp);
	} else if(tp->op == IR_GOTO) {
		if(tp->right->triple.left) {
			printf("\n");
			print_triple((TRIPLE*)tp->right,indent+2);
		} else {
			printf("exit\n");
		}
	} else {
		if(tp->left)  {
			print_operand(tp->left);
		}
		if(tp->right) switch(tp->right->operand.tag) {
			case ISLEAF:
				print_operand(tp->right);
				break;
			case ISTRIPLE:
				if(ISOP(tp->op,BRANCH_OP)) {
					printf(" labels:");
					TFOR(tlp, (TRIPLE*) tp->right){
						printf(" B[%d] if ",((BLOCK*)tlp->left)->blockno);
						printf("L[%d]", ((LEAF*)tlp->right)->leafno);
					}
				} else if(tp->op == IR_SCALL || tp->op == IR_FCALL) {
					if(tp->right) {
						printf("args :\n");
						TFOR(tlp,tp->right->triple.tnext) {
							print_triple(tlp,indent+2);
						}
					}
				} else {
					print_operand(tp->right);
				}
				break;
	
			default:
				printf("print_triple: bad right operand");
				break;
		}
		if(tp->reachdef1) {
			printf("\n		reachdef1: ");
			LFOR(lp,tp->reachdef1->next) {
				if(tp->left->operand.tag == ISTRIPLE) {
					printf("T[%d] ", ( LCAST(lp,EXPR_REF)->site.tp->tripleno));
				} else if(tp->left->operand.tag == ISLEAF ){
					printf("T[%d] ", ( LCAST(lp,VAR_REF)->site.tp->tripleno));
				} else {
					printf("bad left operand");
				}
			}
		}
		if(tp->reachdef2) {
			printf("\n		reachdef2: ");
			LFOR(lp,tp->reachdef2->next){
				if(tp->right->operand.tag == ISTRIPLE) {
					printf("T[%d] ", ( LCAST(lp,EXPR_REF)->site.tp->tripleno));
				} else if(tp->right->operand.tag == ISLEAF ){
					printf("T[%d] ", ( LCAST(lp,VAR_REF)->site.tp->tripleno));
				} else {
					printf("bad right operand");
				}
			}
		}
		if( tp->can_access ) {
			printf("\n		can_access: ");
			LFOR(lp,tp->can_access){
					printf("L[%d] ", LCAST(lp,LEAF)->leafno);
			}
		}
		if(tp->canreach) {
			printf("\n		canreach: ");
			LFOR(lp,tp->canreach->next){
				if(ISOP(tp->op,VALUE_OP)){
					printf("T[%d] ", LCAST(lp,EXPR_REF)->site.tp->tripleno);
				} else {
					printf("T[%d] ", LCAST(lp,VAR_REF)->site.tp->tripleno);
				}
			}
		}
		if( tp->var_refs ) {
			printf("\n		var refs: ");
			i = 0;
			for(vrp=tp->var_refs;vrp && vrp->site.tp == tp;vrp= vrp->next){
				if (i++ > 0) {
					printf("\n			");
				}
				print_var_ref(vrp);
			}
		}
		lvpr(tp);
		printf("\n");
	}
}

void
print_block(p, detailed)
	register BLOCK *p;
	BOOLEAN detailed;
{
	register LIST *lp;
	register TRIPLE *tlp;
	int i=0;

	printf("[%d] %s label %d loop_weight %u next %d",p->blockno,
				(p->entryname ? p->entryname : ""), p->labelno, 
				p->loop_weight, (p->next ? p->next->blockno : -1 ));

	printf(" pred: ");
	if(p->pred) LFOR(lp,p->pred->next) {
		printf("%d ",  ((BLOCK *)lp->datap)->blockno );
	}
	printf("succ: ");
	if(p->succ) LFOR(lp,p->succ->next) {
		printf("%d ",  ((BLOCK *)lp->datap)->blockno );
	}
	printf("\ntriples:");
	if(p->last_triple) TFOR(tlp,p->last_triple->tnext) {
		if(detailed == IR_TRUE) {
			print_triple(tlp, 1);
		} else {
			if( (++i)%35 == 0) printf("\n");
			printf("%d ",   tlp->tripleno);
		}
	}
	show_live_after_block(p);
	printf("\n");
}

void
print_segment(sp)
	register SEGMENT *sp;
{
	printf("%s",sp->name);
	printf(" length %d",sp->len);
	printf(" %s ", seg_content_names[(int)sp->descr.content]);
	printf(" %s ",(sp->descr.external == EXTSTG_SEG) ? "EXTSTG_SEG" :"LCLSTG_SEG");
	printf("\n");
}

void
dump_segments()
{
    register SEGMENT *sp;

    printf("\nSEGMENTS\n");
    for(sp=seg_tab; sp != NULL; sp = sp->next_seg) {
	print_segment(sp);
    }
}

void
dump_leaves() 
{
	LEAF *leafp;

	printf("\nLEAVES\n");
	for(leafp = leaf_tab; leafp; leafp = leafp->next_leaf ) {
		print_leaf(leafp);
	}
}

void
dump_triples() 
{
	register TRIPLE *tp;

	printf("\nTRIPLES\n");
	for(tp=triple_tab;tp<triple_top; tp++) {
		print_triple(tp,0);
	}
}

void
dump_blocks() 
{
	register BLOCK *bp;

	printf("\nBLOCKS\n");
	for(bp=entry_block;bp;bp=bp->next) {
		printf("(0x%x): ", bp);
		print_block(bp, IR_FALSE);
		printf(" next=(0x%x) prev=(0x%x)\n", bp->next, bp->prev);
	}
}

/*
 * dbx printing functions
 */

/*LINTLIBRARY*/

void
show_list(start)
	LIST *start;
{
	LIST *l;

	LFOR(l, start) {
		(void)printf("(0x%08x): 0x%08x\n", (int)l, (int)l->datap);
	}
}

/*
 * apply function to each element of a list
 */
void
mapc(list, fun)
	LIST *list;
	int  (*fun)();
{
	register LIST *lp;

	if (fun != NULL) {
		LFOR(lp, list) {
			(*fun)(lp->datap);
		}
	}
}
	
/*
 * print a triple
 */
void
tpr(tp)
	TRIPLE *tp;
{
	printf("(0x%x) T", tp);
	print_triple(tp, 0);
}

/*
 * print a leaf
 */
void
lpr(leafp)
	LEAF *leafp;
{
	printf("(0x%x) L", leafp);
	print_leaf(leafp);
}

/*
 * print a block
 */
void
bpr(blockp)
	BLOCK *blockp;
{
	printf("(0x%x) B", blockp);
	print_block(blockp, IR_TRUE);
}

/*
 * print a node (one of the above)
 */
void
npr(np)
	IR_NODE *np;
{
	if (np != NULL) {
		switch(np->operand.tag) {
		case ISLEAF:
			lpr((LEAF*)np);
			break;
		case ISTRIPLE:
			tpr((TRIPLE*)np);
			break;
		case ISBLOCK:
			bpr((BLOCK*)np);
			break;
		}
	}
}

/*
 * print a triple, by number
 */
void
tnpr(n)
{
	register TRIPLE *tp;
	register BLOCK *bp;

	for(bp=entry_block; bp; bp = bp->next) {
		TFOR(tp, bp->last_triple) {
			if (tp->tripleno == n) {
				tpr(tp);
				return;
			}
		}
	}
	printf("can't find triple T[%d]\n", n);
}

/*
 * print a leaf node, by number
 */
void
lnpr(n)
{
	register LEAF *leafp;

	for(leafp = leaf_tab; leafp; leafp = leafp->next_leaf ) {
		if (leafp->leafno == n) {
			lpr(leafp);
			return;
		}
	}
	printf("can't find leaf node L[%d]\n", n);
}


/*
 * print a block, by number
 */
void
bnpr(n)
{
	register BLOCK *bp;

	for(bp=entry_block; bp; bp=bp->next) {
		if (bp->blockno == n) {
			bpr(bp);
			return;
		}
	}
	printf("can't find block B[%d]\n", n);
}

/*
 * show live defs after triple T
 */
void
lvpr(T)
	TRIPLE *T;
{
	BLOCK *bp;
	TRIPLE *tp;
	SITE site;

	for(bp=entry_block; bp; bp=bp->next) {
		TFOR(tp, bp->last_triple->tnext) {
			if (tp == T) {
				site.bp = bp;
				site.tp = tp;
				show_live_after_site(site);
				return;
			}
		}
	}
}

/*
 * show live defs after triple with triple number Tn
 */
void
lvnpr(Tn)
{
	BLOCK *bp;
	TRIPLE *tp;
	SITE site;

	for(bp=entry_block; bp; bp=bp->next) {
		TFOR(tp, bp->last_triple->tnext) {
			if (tp->tripleno == Tn) {
				site.bp = bp;
				site.tp = tp;
				show_live_after_site(site);
				return;
			}
		}
	}
	printf("can't find triple T[%d]\n", Tn);
}

int
nitems(listp)
	LIST *listp;
{
	LIST *lp;
	int n = 0;
	LFOR(lp, listp) { n++; }
	return n;
}

void
dump_set(set)
	SET_PTR set;
{
	int nrows;
	int i;

	if (set == NULL) {
		return;
	}
	nrows = NROWS(set);
	for (i = 0; i < nrows; i++) {
		dump_row(set, i);
	}
}

void
dump_row(set, i)
	SET_PTR set;
	int i;
{
	SET row;
	int nwords;
	int j,n;

	if (set == NULL) {
		return;
	}
	nwords = SET_SIZE(set);
	row = ROW_ADDR(set, i);
	n = (nwords/4)*4;
	for (j = 0; j < n; j += 4) {
		printf("(%5d,%5d): 0x%08x 0x%08x 0x%08x 0x%08x\n",
		    i, j, row[j], row[j+1], row[j+2], row[j+3]);
	}
	if (n < nwords) {
		printf("(%5d,%5d):", i, n);
		for( j = n; j < nwords; j++ ) {
			printf(" 0x%08x", row[j]);
		}
		if (nwords >= 4)
			printf("\n");
	}
	printf("\n");
}

