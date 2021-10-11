#ifndef lint
static  char sccsid[] = "@(#)iropt.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "iropt.h"
#include "recordrefs.h"
#include "debug.h"
#include "page.h"
#include "cf.h"
#include "setup.h"
#include "aliaser.h"
#include "implicit.h"
#include "var_df.h"
#include "make_expr.h"
#include "expr_df.h"
#include "loop.h"
#include "ln.h"
#include "li.h"
#include "iv.h"
#include "resequence.h"
#include "misc.h"
#include "cse.h"
#include "copy_ppg.h"
#include "reg.h"
#include "reg_alloc.h"
#include "auto_alloc.h"
#include "lu.h"
#include "tail_recursion.h"
#include "read_ir.h"
#include "ir_wf.h"
#include "complex_expansion.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "opdescr.h"

/* pointers to main IR structures */

HEADER	hdr;
BLOCK	*entry_block, *exit_block;
BLOCK	*first_block_dfo;
BLOCK	*last_block;
SEGMENT	*seg_tab, *last_seg;
LEAF	*leaf_tab, *leaf_top;
TRIPLE	*triple_tab, *triple_top;
LIST	*ext_entries;
LIST	*labelno_consts;
LIST	*indirgoto_targets;
LIST	*leaf_hash_tab[LEAF_HASH_SIZE];

/* IR structure counters */

int	nseg=0;
int	nblocks=0;
int	nleaves=0;
int	ntriples=0;
int	npointers=0;
int	nelvars=0;

/* other globals */

BOOLEAN debugflag[MAXDEBUGFLAG+1] = {IR_FALSE};
BOOLEAN optimflag[MAXOPTIMFLAG+1] = {IR_FALSE};
BOOLEAN use68881 = {IR_FALSE};
BOOLEAN use68020 = {IR_FALSE};
BOOLEAN usefpa = {IR_FALSE};
BOOLEAN picflag = {IR_FALSE};
BOOLEAN multientries = {IR_FALSE};
BOOLEAN unknown_control_flow = {IR_FALSE};
BOOLEAN regs_inconsistent = {IR_FALSE};
#if TARGET == I386
BOOLEAN weitek = {IR_FALSE};	/*KMC*/
#endif
int	optim_level = 0;
char	*ir_file; 
char	*heap_start;

char *optim_nametab[] = {
	"iv",
	"loop",
	"cse",
	"copy",
	"reg",
	"unroll",
	"tail",
	NULL
};

char *debug_nametab[] = {
	"",
	"b_iv",
	"a_iv",
	"b_loop",
	"a_loop",
	"b_localcse",
	"b_globalcse",
	"a_globalcse",
	"b_localcopy",
	"b_globalcopy",
	"a_globalcopy",
	"b_reg",
	"a_reg",
	"b_unroll",
	"a_unroll",
	"alias",
	"page",
	"showraw",
	"showcooked",
	"showdf",
	"showlive",
	"showdensity",
	"b_tail",
	"a_tail",
	NULL
};

/* locals */
static BOOLEAN	save_optimflag[MAXOPTIMFLAG+1] = {IR_FALSE};
static char	*pcc_file;
static char	stdout_buffer[BUFSIZ];
static char	stderr_buffer[BUFSIZ];
static char	outfp_buffer[BUFSIZ];
static FILE	*outfp=NULL;

static char	*scan_args(/*argc,argv*/);
static void	iropt_init();
static void	ir_prewrite();

int
main(argc,argv) 
	int argc;
	char *argv[];
{
	int irfd;

	iropt_init();
	if( (ir_file=scan_args(argc,argv)) != NULL ) {
		irfd = open(ir_file,O_RDONLY,0);
		if (irfd == -1) {
			perror(ir_file);
			quit("can't open ir file");
		}
		if(pcc_file == (char*) NULL) {
			pcc_file = "a.pcc";
		}
		outfp=fopen(pcc_file,"w");
		if(outfp == NULL) {
			perror("iropt:");
			quit("can't open pcc file");
		}
		setbuf(outfp,outfp_buffer);
		bcopy((char*)optimflag, (char*)save_optimflag, sizeof(optimflag));
		while( read(irfd, (char*)&hdr, sizeof(HEADER)) == sizeof(HEADER) ) {
			if(read_ir(irfd) == IR_TRUE) {
				bcopy((char*)save_optimflag, (char*)optimflag, sizeof(optimflag));
				if (unknown_control_flow || regs_inconsistent
				    || hdr.opt_level == 0) {
					/*
					 * some day, we may be able to do
					 * something better in the former
					 * case than in the latter ... in
					 * the interim, all bets are off
					 * for this procedure.
					 */
					int i;
					for(i=0;i< MAXOPTIMFLAG+1;i++) {
						optimflag[i] =  IR_FALSE;
					}
				}
				form_bb();
				if(SHOWRAW==IR_TRUE) {
					dump_segments();
					dump_blocks();
					dump_leaves();
					dump_triples();
				}
				if(optimizing() == IR_TRUE) {
					if(multientries == IR_FALSE){
						if(DO_TAIL_REC == IR_TRUE){
							if(debugflag[B_TAIL_DEBUG]==IR_TRUE)
								dump_cooked(B_TAIL_DEBUG);
							do_tail_recursion();
                                                        if(debugflag[A_TAIL_DEBUG]==IR_TRUE)
                                                                dump_cooked(A_TAIL_DEBUG);
						}
					}
                                        if (worth_optimizing_complex_operations()) {
                                            expand_complex_leaves();
                                            expand_complex_operations();
                                            resequence_triples();
                                        }
					find_dfo();
					find_aliases();
					insert_implicit();
					find_loops(); 
					make_loop_tree();
					if(DO_LOOP == IR_TRUE) {
						preloop_ntriples = ntriples;
						preloop_nleaves = nleaves;
						compute_df(IR_TRUE,IR_FALSE,IR_TRUE,IR_FALSE);
						delete_implicit();
						/* want 1 temp per expr so build expr_hash_tab */
						entable_exprs();
						free_depexpr_lists();
                                                if(debugflag[B_LOOP_DEBUG]==IR_TRUE)
                                                        dump_cooked(B_LOOP_DEBUG);
						iv_init_visited();
						scan_loop_tree(loop_tree_root,IR_FALSE,IR_FALSE);
						if(debugflag[A_LOOP_DEBUG]==IR_TRUE) 
							dump_cooked(A_LOOP_DEBUG);
						if(debugflag[PAGE_DEBUG]) {
							mem_stats(LOOP_PHASE);
						}
						free_exprs();
						empty_listq(df_lq);
					}
					if(DO_IV == IR_TRUE && nloops > 0) {
						if(iv_lq == NULL) iv_lq = new_listq();
						resequence_triples();
						preloop_ntriples = ntriples;
						preloop_nleaves = nleaves;
						compute_df(IR_TRUE,IR_FALSE,IR_TRUE,IR_FALSE);
						entable_exprs();
						if(debugflag[B_IV_DEBUG]==IR_TRUE) 
							dump_cooked(B_IV_DEBUG);
						iv_init_visited();
						scan_loop_tree(loop_tree_root,IR_TRUE,IR_FALSE);
						if(debugflag[A_IV_DEBUG]==IR_TRUE) 
							dump_cooked(A_IV_DEBUG);
						if(debugflag[PAGE_DEBUG]) {
							mem_stats(IV_PHASE);
						}
						empty_listq(df_lq);
						empty_listq(tmp_lq);
						empty_listq(iv_lq);
						free_depexpr_lists();
						free_exprs();
					}
					if(DO_CSE == IR_TRUE ) {
						resequence_triples();	
						cse_init();
						compute_df(IR_FALSE,IR_TRUE,IR_FALSE,IR_FALSE);
						if(debugflag[B_LOCALCSE_DEBUG]==IR_TRUE) 
							dump_cooked(B_LOCALCSE_DEBUG);
						do_local_cse();
						free_exprs();
						empty_listq(df_lq);
						compute_df(IR_TRUE,IR_TRUE,IR_TRUE,IR_FALSE);
						if(debugflag[B_GLOBALCSE_DEBUG]==IR_TRUE) 
							dump_cooked(B_GLOBALCSE_DEBUG);
						do_global_cse();
						if(debugflag[A_GLOBALCSE_DEBUG]==IR_TRUE) 
							dump_cooked(A_GLOBALCSE_DEBUG);
						if(debugflag[PAGE_DEBUG]) {
							mem_stats(CSE_PHASE);
						}
						free_exprs();
						empty_listq(df_lq);
					}
					if(DO_COPY_PPG == IR_TRUE) {
						resequence_triples();	
						compute_df(IR_TRUE,IR_FALSE,IR_FALSE,IR_FALSE);
						if(debugflag[B_LOCALPPG_DEBUG]==IR_TRUE) 
							dump_cooked(B_LOCALPPG_DEBUG);
						do_local_ppg();
						empty_listq(df_lq);
						compute_df(IR_TRUE,IR_FALSE,IR_TRUE,IR_FALSE);
						if(debugflag[B_GLOBALPPG_DEBUG]==IR_TRUE) 
							dump_cooked(B_GLOBALPPG_DEBUG);
						do_global_ppg();
						if(debugflag[A_GLOBALPPG_DEBUG]==IR_TRUE) 
							dump_cooked(A_GLOBALPPG_DEBUG);
						if(debugflag[PAGE_DEBUG]) {
							mem_stats(COPY_PPG_PHASE);
						}
						empty_listq(df_lq);
					}
					if(DO_REG_ALLOC == IR_TRUE ) {
						resequence_triples();	
						compute_df(IR_TRUE,IR_FALSE,IR_TRUE,IR_TRUE);
						if(debugflag[B_REGALLOC_DEBUG]==IR_TRUE) 
							dump_cooked(B_REGALLOC_DEBUG);
						do_register_alloc();
						if(debugflag[A_REGALLOC_DEBUG]==IR_TRUE) 
							dump_cooked(A_REGALLOC_DEBUG);
						if(debugflag[PAGE_DEBUG]) {
							mem_stats(REG_ALLOC_PHASE);
						}
						free_webs();
						empty_listq(df_lq);
						empty_listq(reg_lq);
					}
					if(DO_UNROLL == IR_TRUE && nloops > 0
					    && loop_unroll_enabled()) {
						if(iv_lq == NULL) iv_lq = new_listq();
						resequence_triples();	
						preloop_ntriples = ntriples;
						preloop_nleaves = nleaves;
						compute_df(IR_TRUE,IR_FALSE,IR_TRUE,IR_FALSE);
						if(debugflag[B_UNROLL_DEBUG] == IR_TRUE)
							dump_cooked(B_UNROLL_DEBUG);
						iv_init_visited();
						scan_loop_tree(loop_tree_root,IR_FALSE,IR_TRUE);
						if(debugflag[A_UNROLL_DEBUG] == IR_TRUE)
							dump_cooked(A_UNROLL_DEBUG);
						if(debugflag[PAGE_DEBUG]) {
							mem_stats(UNROLL_PHASE);
						}
						empty_listq(df_lq);
						empty_listq(loop_lq);
						empty_listq(iv_lq);
					}
				}
				ir_prewrite();
				do_auto_alloc();
				write_irfile(hdr.procno, hdr.procname, hdr.proc_type, 
					hdr.source_file, hdr.source_lineno, hdr.opt_level, outfp);
				empty_listq(proc_lq);
			}
			heap_reset();
		}
	}
	return(0);
}

static char *
scan_args(argc,argv)
	int argc;
	register char *argv[];
{
	register char c;
	register char *s;
	register int i;
	char option_name[BUFSIZ];

	--argc;
	++argv;
	optimflag[0] = debugflag[DDEBUG] = IR_FALSE;
	pcc_file = (char*) NULL;
	while(argc > 0 &&  *argv[0] =='-' ) {
		switch(*(argv[0]+1)) {
		case 'o':
			--argc;
			++argv;
			if(argc == 0) {
				quit("scanargs: no argument given for -o");
			}
			pcc_file = *argv;
			goto contin;
		}
		for(s=argv[0]+1;*s; ++s) {
			switch(c = *s) {
			case 'P':
			case 'O':
				optim_level = (c=='P' ? PARTIAL_OPT_LEVEL : MAX_OPT_LEVEL);
				for(i=0;i< MAXOPTIMFLAG+1;i++) {
					optimflag[i] =  IR_TRUE;
				}
				/*
				**	test whether individual optimizations should be 
				**	suppressed
				*/
				while(c == 'O' || c == 'P' || c == ',') {
					i = 0;
					c = *++s;
					if (isdigit(c)) {
						/* -O<level>: set optimization level */
						while (isdigit(c)) {
							i = i*10 + (c - '0');
							c = *++s;
						}
						if (i > MAX_OPT_LEVEL) {
							i = MAX_OPT_LEVEL;
						}
						optim_level = i;
						if(optim_level < PARTIAL_OPT_LEVEL)
							for(i=0;i< MAXOPTIMFLAG+1;i++) {
								optimflag[i] =  IR_FALSE;
							}
					} else {
						/* -O<string>: set individual optim_flag */
						while (isalnum(c) || c == '_') {
							option_name[i++] = c;
							c = *++s;
						}
						option_name[i] = '\0';
						for (i = 0; optim_nametab[i] != NULL; i++) {
							if (strcmp(option_name, optim_nametab[i]) == 0) {
								optimflag[i] = IR_FALSE;
								break; /* for */
							}
						}
					}
				}
				goto contin;
	
			case 'd':
				debugflag[DDEBUG]=IR_TRUE;
				c = *s;
				while(c == 'd' || c == ',') {
					i = 0;
					c = *++s;
					while (isalnum(c) || c == '_') {
						option_name[i++] = c;
						c = *++s;
					}
					option_name[i] = '\0';
					for (i = 0; debug_nametab[i] != NULL; i++) {
						if (strcmp(option_name, debug_nametab[i]) == 0) {
							debugflag[i] = IR_TRUE;
							break; /* for */
						}
					}
				}
				goto contin;
	
			case 'F':
				/* skyflag = IR_TRUE; */
				break;

			case 'm':
				use68881 = IR_TRUE;
				break;
			
			case 'f':
				usefpa = IR_TRUE;
				break;
				
			case 'c':
				use68020 = IR_TRUE;
				break;
				
			case 'k':
				picflag = IR_TRUE;
				break;

#if TARGET == I386
			case 'w':	/*weitek option ..KMC*/
				if (*(s+1) == 'c' && *(s+2) == '\0'){
				    weitek = IR_TRUE;
				    s++;
				} else
				    quita("invalid flag %s",s);
				break;
#endif

			case 'l':
				i = 0;
				c = *++s;
				while (isdigit(c)) {
					i = i*10 + (c - '0');
					c = *++s;
				}
				set_unroll_option(i);
				goto contin;

			default:
				quita("invalid flag %c",*s);
				/*NOTREACHED*/
			}
		}
	contin: 
		--argc;
		++argv;
	}
	if(argc < 1) {
		quit(" no ir file specified ");
		/*NOTREACHED*/
	}
	return(argv[0]);
}

BOOLEAN
optimizing()
{
	int i;

	for (i = 0; i < MAXOPTIMFLAG+1; i++){
		if (optimflag[i] == IR_TRUE)
			return IR_TRUE;
	}
	return IR_FALSE;
}

static void
iropt_init()
{
	/* make sure the stdio buffers don't interfere with heap management */
	extern char *sbrk();
	setbuf(stdout,stdout_buffer);
	setbuf(stderr,stderr_buffer);
	heap_start= sbrk(0);
}

void
proc_init()
{
	register LIST **lpp, **lptop;
	register EXPR **epp, **eptop;

	(void)fflush(stdout);
	lptop = &leaf_hash_tab[LEAF_HASH_SIZE];
	for(lpp=leaf_hash_tab; lpp < lptop;) *lpp++ = LNULL;

	eptop = &expr_hash_tab[EXPR_HASH_SIZE];
	for(epp=expr_hash_tab; epp < eptop;) *epp++ = (EXPR *)NULL;

	entry_block = exit_block = (BLOCK*) NULL;
	loop_tree_root = ( LOOP_TREE*) NULL;
	init_cf();

	if(proc_lq == NULL) proc_lq = new_listq();
	if(df_lq == NULL) df_lq = new_listq();
	if(tmp_lq == NULL) tmp_lq = new_listq();
	/*
	 * set default reg "masks", since we might run
	 * without doing register allocation
	 */
#	if TARGET == MC68000
	    regmask = SET_REGMASK(0, 0, MAX_FREG, MAX_AREG, MAX_DREG);
#	endif
#	if TARGET == SPARC
	    regmask = AVAIL_INT_REG;
	    fregmask = AVAIL_F_REG;
#	endif
#	if TARGET == I386
	    regmask = AVAIL_REG;
#	endif
}

/*
 * before rewriting an ir file  make sure labelefs point to leaves so
 * cg doesn't have to distinguish optimized from unoptimzed input; also
 * zero out some fields that will not be used by cg
 */
static void
ir_prewrite()
{
        register BLOCK *bp, *block;
	register TRIPLE *tp, *labelref;
	IR_OP op;

	cleanup_cf();
	for(bp=entry_block; bp; bp=bp->next) {
		TFOR(tp, bp->last_triple) {
			op = tp->op;
			if(ISOP(op,NTUPLE_OP)) {
				TFOR(labelref, (TRIPLE*)tp->right) {
                                        if( labelref->op == IR_LABELREF ) {
                                                if((block=(BLOCK*)labelref->left) && block->tag ==ISBLOCK){
                                                        labelref->left = (IR_NODE*) ileaf(block->labelno);
						} else {
							labelref->left = (IR_NODE*) ileaf(-1);
						}
					}
				}
                        } else if(op == IR_LABELDEF) {
                                if((block=(BLOCK*)tp->left) && block->tag ==ISBLOCK) {
                                        tp->left = (IR_NODE*) ileaf(block->labelno);
				} else {
					tp = free_triple(tp, bp);
				}
			} else if (op == IR_IMPLICITUSE
			    || op == IR_ENTRYDEF
			    || op == IR_EXITUSE
			    || op == IR_IMPLICITDEF) {
				/* these are of no use to us in cg */
				tp = free_triple(tp, bp);
			}
			tp->can_access = LNULL;
		}
		bp->pred = bp->succ = LNULL;
	}
}
