#ifndef lint
static char sccs_id[] = "@(#)opt_regsets.c	1.1	94/10/31";
#endif
/*
 * This file contains code which sets the register usage information
 * in each instruction node. This includes the REGS_READ, REGS_WRITTEN,
 * and REGS_LIVE information. Once this is set up, local transformations
 * on the program must maintain it.
 */

#include <stdio.h>
#include "structs.h"
#include "alloc.h"
#include "bitmacros.h"
#include "registers.h"
#include "optimize.h"
#include "opt_statistics.h"
#include "opt_utilities.h"
#include "opt_regsets.h"
#include "pcc_types.h"

/*****************************************************************************/
/*
 * def'ns and routines for manipulating register sets
 */

	/* the empty set */
register_set empty_regset =  { 0,0,0 };
	/* the set {%g0} */
register_set zero_regset = { (1<<RAW_REGNO(RN_G0)), 0, 0 };
	/* the set of out registers */
register_set out_regset   =  { 0x0000ff00, 0, 0 };
	/* the set of window registers */
register_set window_regset = { 0xffffff00, 0, 0 };
	/* the set of floating-point registers */
register_set float_regset =  { 0, 0xffffffff, 0 };
	/* the condition codes */
register_set cc_regset = { 0, 0, (1<<RAW_REGNO(RN_ICC))|(1<<RAW_REGNO(RN_FCC)) };
	/* register live upon returning */
register_set exit_regset;
	/* all things the compiler knows about */
register_set compiler_regset = { 0xfffffffe, 0xffffffff, 
		(1<<RAW_REGNO(RN_ICC))|(1<<RAW_REGNO(RN_FCC))|(1<<RAW_REGNO(RN_MEMORY)) };

/*
 *   Data structures and routines to handle "special (known)" subroutines.
 */

struct call_table_entry
{
      char *name;
      register_set writes;
};

/*
 *  This table NEEDS to be in synch with a similar table in the code_generator.
 *  Except for "special" case : .stret  which only marks extra "read" (not
 *  "wirtten") registers.
 */
 
struct call_table_entry call_table[] = {
               { ".mul" , {0, 0, 0} } ,
               { ".div" , {0, 0, 0} } ,
               { ".rem" , {0, 0, 0} } ,
               { ".umul", {0, 0, 0} } ,
               { ".udiv", {0, 0, 0} } ,
               { ".urem", {0, 0, 0} } ,
	       { ".stret1",{0, 0, 0} } ,
	       { ".stret2",{0, 0, 0} } ,
	       { ".stret4",{0, 0, 0} } ,
	       { ".stret8",{0, 0, 0} } ,
               { "ld_short", {0, 0, 0} } ,
               { "ld_ushort", {0, 0, 0} } ,
               { "ld_int", {0, 0, 0} } ,
               { "ld_float", {0, 0x00000001, 0} } ,       /* writes %f0 */
               { "ld_double", {0, 0x00000003, 0} } ,      /* writes %f0-%f1 */
               { "st_short", {0, 0, 0} } ,
               { "st_int", {0, 0, 0} } ,
               { "st_float", {0, 0x00000001, 0} } ,        /* writes %f0 */
               { "st_double", {0, 0x00000003, 0} } ,       /* writes %f0-%f1 */
               { "", {0, 0, 0} }
};
 
 /*
  * Look for known routines in the above table and set the written fp-registers if it
  * is in the table.
  */
Bool
special_call_regset(np) 
        register Node *np;
{
        struct call_table_entry *h;

        if (np != NULL 
	    && np->n_operands.op_symp != NULL
	    && np->n_operands.op_symp->s_symname != NULL
	    && np->n_operands.op_symp->s_symname[0] != '_'){
                for ( h = call_table;
		      (*h).name[0] != '\0';
		      h++){
			if ( !strcmp(np->n_operands.op_symp->s_symname, (*h).name) ){
				/* This looks for a .stret call.  This is very 
				 * specific so watch the table for additions.
				 */
				if ( (np->n_operands.op_symp->s_symname[0] == '.') &&
				     (np->n_operands.op_symp->s_symname[1] == 's') )
					regset_procedure_exit_regs_read(np);
				else REGS_WRITTEN(np) = regset_add(REGS_WRITTEN(np), (*h).writes);

				return TRUE;
			}
		}
        }
        return FALSE;
}

/*
 *  Utilities for register_set manipulations.
 */



/*
 * set addition:  a + b 
 */
register_set
regset_add( a, b )
	register_set a, b;
{
	a.r[0] |= b.r[0];
	a.r[1] |= b.r[1];
	a.r[2] |= b.r[2];
	return a;
}

/*
 * set subtractions: a - b
 */
register_set
regset_sub( a, b )
	register_set a,b;
{
	a.r[0]  &= ~b.r[0];
	a.r[1]  &= ~b.r[1];
	a.r[2]  &= ~b.r[2];
	return a;
}

/*
 * set intersection: a * b
 */
register_set
regset_intersect( a, b )
	register_set a,b;
{
	a.r[0] &= b.r[0];
	a.r[1] &= b.r[1];
	a.r[2] &= b.r[2];
	return a;
}

/*
 * the incl function: a + {m}
 * This could easily be inlined one day.
 */
register_set
regset_incl( a, m )
	register_set a;
	register Regno m;
{
	if (m!=RN_G0)
		a.r[ m>>_RAW_REGNO_BITS_NEEDED] |= 1<<RAW_REGNO(m);
	return a;
}

/*
 * the excl function: a - {m}
 * This could easily be inlined one day.
 */
register_set
regset_excl( a, m )
	register_set a;
	register Regno m;
{
	a.r[ m>>_RAW_REGNO_BITS_NEEDED] &= ~(1<<RAW_REGNO(m));
	return a;
}


/*
 * set synthesis: {m}
 * same as regset_incl( empty_regset, m );
 */
register_set
regset_mkset( m )
	register Regno m;
{
	register_set a;
	a = empty_regset;
	if (m!=RN_G0)
		a.r[ m>>_RAW_REGNO_BITS_NEEDED] = 1<<RAW_REGNO(m);
	return a;
}

/*
 * set synthesis: {m, m+1, ..., m+count-1}
 * (iterative form of regset_mkset())
 */
register_set
regset_mkset_multiple( m, count )
	register Regno m;
	register int count;
{
	register_set a;
	a = empty_regset;

	for ( ;  count > 0;  --count, m++) {
		if (m!=RN_G0){
			a.r[ m>>_RAW_REGNO_BITS_NEEDED] |= 1<<RAW_REGNO(m);
		}
	}

	return a;
}

/*
 * set equality: a == b
 */
Bool
regset_equal( a, b )
	register_set a, b;
{
	if (a.r[0] != b.r[0]) return FALSE;
	if (a.r[1] != b.r[1]) return FALSE;
	if (a.r[2] != b.r[2]) return FALSE;
	return TRUE;
}

/*
 * set improper subsetting: a <= b
 */
Bool
regset_subset( a, b )
	register_set a,b;
{
	if ( (a.r[0]&b.r[0]) != a.r[0]) return FALSE;
	if ( (a.r[1]&b.r[1]) != a.r[1]) return FALSE;
	if ( (a.r[2]&b.r[2]) != a.r[2]) return FALSE;
	return TRUE;
}

/*
 * set membership: m in a or {m} <= a
 */
Bool
regset_in( a, m )
	register_set a;
	register Regno m;
{
	if (a.r[ m>>_RAW_REGNO_BITS_NEEDED] & 1<<RAW_REGNO(m)) return TRUE;
	return FALSE;
}

/*
 * compare set with empty set
 */
Bool
regset_empty( a )
	register_set a;
{
	if (a.r[0] != 0 ) return FALSE;
	if (a.r[1] != 0 ) return FALSE;
	if (a.r[2] != 0 ) return FALSE;
	return TRUE;
}

/*
 * choose an element from a set
 * the element chosen is bounded.
 */
int
regset_select( a, min )
	register_set a;
	register Regno min;
{
	register Regno r;
	register long word;
	int indx;
	
	r = RAW_REGNO( min );
	for (indx = min >> _RAW_REGNO_BITS_NEEDED; indx <= 2; indx+= 1, r = 0 ){
		word = a.r[indx];
		if ( word != 0 ){
			for ( r; r< 32; r+=1){
				if ( word & ( 1<<r))
					return (indx<<_RAW_REGNO_BITS_NEEDED)+r;
			}
		}
	}
	return -1;
}
	
/*****************************************************************************/
/*
 * we found a .proc node.
 * set the exit_regset global set from the return-type information.
 */

static void
set_exit_regset( t )
	PCC_TWORD t;
{
	/* include register(s) return value is/are in */
	switch(t){
	case PCC_VOID:
		exit_regset = empty_regset;
		break;
	case PCC_FLOAT:
		exit_regset = regset_mkset(RN_FP(0));
		break;
	case PCC_DOUBLE:
		exit_regset = regset_mkset_multiple(RN_FP(0),2);
		break;
	case PCC_UNDEF:
		/*
		 * Type unknown, assume worst case.
		 * This occurs with multiple entry points
		 * having different return value types.
		 */
		exit_regset = regset_mkset_multiple(RN_FP(0),2);
		exit_regset = regset_incl( exit_regset, RN_I0 );
		break;
	default:
		exit_regset = regset_mkset(RN_I0);
		break;
	}
	/* and add the frame pointer, stack pointer,  and return pc */
	exit_regset = regset_incl( regset_incl( exit_regset, RN_FPTR ), RN_RETPTR);
	exit_regset = regset_incl( exit_regset, RN_SP );
	exit_regset = regset_incl( exit_regset, RN_FSR );
	exit_regset = regset_incl( exit_regset, RN_CSR );
}

/*****************************************************************************/

/*
 * leaf routine optimization changes the RESTORE instruction.
 * The exit_regset needs to be changed as a result.
 */
void
regset_make_leaf_exit()
{
	if (regset_in(exit_regset, RN_I0)) {
		/* %i0 is out. %o0 is in. */
		exit_regset = regset_excl( exit_regset, RN_I0 );
		exit_regset = regset_incl( exit_regset, RN_O0 );
	}
}

/*****************************************************************************/

/* Used on the last instruction of a routine (usually RESTORE, but RETL in a
 * Leaf routine).
 * We can pretend that last instruction "reads" %i6, %i7, and the return value,
 * which must be live upon return.
 * Also pretend it uses %sp, so that it will remain live throughout.
 */
void
regset_procedure_exit_regs_read(np)
	register Node * np;
{
	REGS_READ(np)    = regset_add( REGS_READ(np), exit_regset );
}

/*****************************************************************************/

/*
 * set the REGS_READ & REGS_WRITTEN information for a single node
 */

void
regset_setnode( np )
	register Node * np;
{
	register_set rused, rset;

	switch (OP_FUNC(np).f_group){

	case FG_LD:
		/*
		 * loads write the dest reg, 
		 * read memory & the registers used in addressing
		 */
		REGS_WRITTEN(np) =
			regset_mkset_multiple(
				np->n_operands.op_regs[O_RD],
				BYTES_TO_WORDS(np->n_opcp->o_func.f_rd_width));
		REGS_READ(np) = regset_mkset(RN_MEMORY);
		goto memref;
	case FG_ST:
		/*
		 * stores write memory
		 * read the "destination" register 
		 * & the registers used in addressing
		 */
		REGS_WRITTEN(np) = regset_mkset(RN_MEMORY);
		REGS_READ(np) =
			regset_mkset_multiple(
				np->n_operands.op_regs[O_RD],
				BYTES_TO_WORDS(np->n_opcp->o_func.f_rd_width));
		goto memref;
	case FG_LDST:
		/* ldst does everything load & store do */
		REGS_WRITTEN(np) = regset_incl( regset_mkset(RN_MEMORY),
						np->n_operands.op_regs[O_RD] );
		REGS_READ(np) = REGS_WRITTEN(np);
		goto memref;
	case FG_RET:
	case FG_JMP:
		/* jumps read the registers used in addressing */
		REGS_WRITTEN(np) = regset_mkset(np->n_operands.op_regs[O_RD]);
		REGS_READ(np) = empty_regset;
		if( NODE_IS_RET(np) ) {/* RETURN reads and writes MEMORY */
			REGS_READ(np) = regset_incl( REGS_READ(np), RN_MEMORY );
			REGS_WRITTEN(np) = regset_incl( REGS_WRITTEN(np), RN_MEMORY );
		}
	memref:
		/* decode addressing modes for memory reference instructions*/
		REGS_READ(np) = regset_incl( REGS_READ(np), np->n_operands.op_regs[O_RS1]);
		if (! np->n_operands.op_imm_used)
			REGS_READ(np) = regset_incl( REGS_READ(np),
						np->n_operands.op_regs[O_RS2]);
		break;

	case FG_ARITH:
		/*
		 * arithmetic instructions write the destination register.
		 * the read their operands
		 * they may set the condition code.
		 * some also write the Y register!
		 * save and restore write clobber all the window registers!
		 */
		REGS_WRITTEN(np) = regset_mkset(np->n_operands.op_regs[O_RD]);
		REGS_READ(np) = regset_mkset(np->n_operands.op_regs[O_RS1]);
		if (! np->n_operands.op_imm_used)
			REGS_READ(np) = regset_incl( REGS_READ(np),
						np->n_operands.op_regs[O_RS2]);
		if (OP_FUNC(np).f_setscc)
			REGS_WRITTEN(np) = regset_incl(REGS_WRITTEN(np),RN_ICC);
		if (OP_FUNC(np).f_br_cc) /* this is an {add,sub}x[cc] */
			REGS_READ(np) = regset_incl(REGS_READ(np),RN_ICC);		
		/* account for special cases */
		switch (OP_FUNC(np).f_subgroup){
		case FSG_MULS:
			/* multiply-step uses the Y regiser */
			REGS_WRITTEN(np) = regset_incl(REGS_WRITTEN(np), RN_Y);
			REGS_READ(np) = regset_incl(REGS_READ(np), RN_Y);
			break;
		case FSG_SAVE:
			/* we can pretend that save reads & clobbers the whole
			 * window.
			 */
			REGS_WRITTEN(np) =
				regset_add( REGS_WRITTEN(np), window_regset);
			REGS_READ(np)    =
				regset_add( REGS_READ(np),    window_regset);
			break;
		case FSG_REST:
			/* We can pretend that restore clobbers the whole
			 * window, except the "exit_regset",
			 * which must be live upon return.
			 */
			REGS_WRITTEN(np) =
				regset_sub( window_regset, exit_regset );
			regset_procedure_exit_regs_read(np);
			break;
		}
		break;

	case FG_SETHI:
	case FG_SET:
		/* sethi writes into its destination register */
		REGS_WRITTEN(np) = regset_mkset(np->n_operands.op_regs[O_RD]);
		REGS_READ(np) = empty_regset;
		break;

	case FG_BR:
	case FG_TRAP:
		/* branches and traps may read the condition codes */
		REGS_WRITTEN(np) = empty_regset;
		/*
		 * branch/traps look at icc or fcc or no condition at all
		 */
		switch (OP_FUNC(np).f_br_cc){
		case CC_NONE:
			REGS_READ(np) = empty_regset; break;
		case CC_I:
			REGS_READ(np) = regset_mkset( RN_ICC ); break;
		case CC_F:
			REGS_READ(np) = regset_mkset( RN_FCC ); break;
		}
		break;
        case FG_JMPL:
        case FG_CALL:
		if (NODE_IS_PIC_CALL(np)) {
			/*
			 * no effect except o7 := pc
			 */
			REGS_READ(np) = empty_regset;
			REGS_WRITTEN(np) = regset_mkset( RN_GP_OUT(7) );
			break;
		}
                if (np->n_operands.op_call_oreg_count_used){
                        REGS_READ(np) = regset_mkset_multiple( RN_O0,
                                        np->n_operands.op_call_oreg_count);
                } else { 
                        /*
                         * an unknown call can read all
                         * the out registers, except %o7 
                         */
                        REGS_READ(np) = regset_excl( out_regset, RN_RETPTR );
                }
                /* CALL reads MEMORY */
                REGS_READ(np) = regset_incl( REGS_READ(np), RN_MEMORY );
                REGS_READ(np) = regset_incl( REGS_READ(np), RN_SP   );
                /* It writes all the out_register except the stack pointer. */
                REGS_WRITTEN(np) = regset_excl( out_regset, RN_SP);
                /* WRITEs FP, globals, miscs registers and MEMORY ... */
                REGS_WRITTEN(np) = regset_add( REGS_WRITTEN(np),
                                   regset_mkset_multiple( RN_G0+1, 7));  
                REGS_WRITTEN(np) = regset_add( REGS_WRITTEN(np),
			           regset_mkset_multiple(RN_Y, _NB_ALL_RES - 64));
		if (OP_FUNC(np).f_group == FG_JMPL) {
			REGS_READ(np) = regset_add( REGS_READ(np),
			       regset_mkset(np->n_operands.op_regs[O_RS1]));
			REGS_READ(np) = regset_add( REGS_READ(np),
			       regset_mkset(np->n_operands.op_regs[O_RS2]));
			REGS_WRITTEN(np) = regset_add( REGS_WRITTEN(np),
			       regset_mkset(np->n_operands.op_regs[O_RD]));
		}
                /* ... but we have to keep live the %csr and %fsr */
                REGS_WRITTEN(np) = regset_excl( REGS_WRITTEN(np), RN_FSR);
                REGS_WRITTEN(np) = regset_excl( REGS_WRITTEN(np), RN_CSR);
                if (!special_call_regset(np))
                        REGS_WRITTEN(np) = regset_add( REGS_WRITTEN(np),
                                                       regset_mkset_multiple(RN_FP(0), 32));
                break; 
	case FG_RD:
		/* read into the register in the rd field */
		REGS_WRITTEN(np) = regset_mkset(np->n_operands.op_regs[O_RD]);
		switch (OP_FUNC(np).f_subgroup){
		case FSG_RDY:	REGS_READ(np) = regset_mkset( RN_Y ); break;
		case FSG_RDPSR:	REGS_READ(np) = regset_mkset( RN_PSR ); break;
		case FSG_RDWIM:	REGS_READ(np) = regset_mkset( RN_WIM ); break;
		case FSG_RDTBR:	REGS_READ(np) = regset_mkset( RN_TBR ); break;
		}
		break;
	case FG_WR:
		/* written from the expression as per arithmetics */
		REGS_READ(np) = regset_mkset(np->n_operands.op_regs[O_RS1]);
		if (! np->n_operands.op_imm_used)
			REGS_READ(np) = regset_incl( REGS_READ(np), np->n_operands.op_regs[O_RS2]);
		switch (OP_FUNC(np).f_subgroup){
		case FSG_WRY:	REGS_WRITTEN(np) = regset_mkset( RN_Y ); break;
		case FSG_WRPSR:	REGS_WRITTEN(np) = regset_mkset( RN_PSR );break;
		case FSG_WRWIM:	REGS_WRITTEN(np) = regset_mkset( RN_WIM );break;
		case FSG_WRTBR:	REGS_WRITTEN(np) = regset_mkset( RN_TBR );break;
		}
		break;
		
	case FG_FLOAT2:
		REGS_READ(np) =
			regset_mkset_multiple(
				np->n_operands.op_regs[O_RS2],
				BYTES_TO_WORDS(np->n_opcp->o_func.f_rs2_width));
		switch (OP_FUNC(np).f_subgroup){
		case FSG_FCMP:
			REGS_READ(np) =
				regset_add( REGS_READ(np),
					    regset_mkset_multiple(
						np->n_operands.op_regs[O_RS1],
						BYTES_TO_WORDS(np->n_opcp->o_func.f_rs1_width)));
			REGS_WRITTEN(np) = regset_mkset( RN_FCC );
			break;
		default:
			REGS_WRITTEN(np) =
				regset_mkset_multiple(
					np->n_operands.op_regs[O_RD],
					BYTES_TO_WORDS(np->n_opcp->o_func.f_rd_width));
			break;
		}
		break;
	case FG_FLOAT3:
		REGS_READ(np) =
			regset_mkset_multiple(
				np->n_operands.op_regs[O_RS1],
				BYTES_TO_WORDS(np->n_opcp->o_func.f_rs1_width));
		REGS_READ(np) =
			regset_add(REGS_READ(np),
			    regset_mkset_multiple(
			       np->n_operands.op_regs[O_RS2],
			       BYTES_TO_WORDS(np->n_opcp->o_func.f_rs2_width)));
		REGS_WRITTEN(np) =
			regset_mkset_multiple(
				np->n_operands.op_regs[O_RD],
				BYTES_TO_WORDS(np->n_opcp->o_func.f_rd_width));
		break;
	case FG_COMMENT:
	case FG_UNIMP:
	case FG_NOP:
	case FG_LABEL:
	case FG_EQUATE:
	case FG_PS: /* should I ever see one of these? */
		/* nop, pseudos and others reference no resources */
		REGS_READ(np) = empty_regset;
		REGS_WRITTEN(np) = empty_regset;
		if (OP_FUNC(np).f_subgroup == FSG_PROC)
			set_exit_regset( np->n_operands.op_addend );
		break;
	default:
		break;  /* Nothing to do here */
	}
}

/*****************************************************************************/
/*****************************************************************************/

/*
 * Register set calculus:
 *
 * 1) The "liveset" of an instruction denotes the set of resources which,
 *    at a point before the execution of the given instruction, will be
 *    read before they are next modified.
 * 2) The liveset of a plain-old non-branching instruction is computed
 *    by subtracting from the liveset of its successor the set of resources
 *    written by the given instruction, then adding the set of resources
 *    read by the instruction.
 * 3) The liveset of a label (or other pseudo-op) is the same as the
 *    liveset of its succeeding instruction. This is a consequence of the
 *    passivity of such entities.
 * 4) For a 0-delay branch instruction, the liveset 
 *    is the sum of the livesets of it and all its possible successors.
 *    a) In the case of an unconditional branch, this is only its target.
 *    b) In the case of a conditional branch, this includes both its target
 *	 and its sequencial successor.
 *    c) In the case of a multi-way computed branch, it is the sum of the 
 *	 livesets of all its possible destinations.
 *    d) In the case of a computed GOTO, it is all the resources, since 
 *	 we don't know where it will land.
 * 5) For a 1-delay branch instruction, the liveset is the sum of the
 *    resources read by the branch instruction and the liveset of the
 *    instruction in its delay slot. However, since THAT instruction may have
 *    many possible successors, its liveset must be computed similarly to
 *    the 0-delay branch in item (4) above.
 */

/*****************************************************************************/

/*
 * routines and data structures for maintaining a stack of Node *'s
 */

#define NNODES 1999 /* or any other arbitrary constant */
static Node **nodestack=NULL;
static int    nodesp = 0;
static int    nnodes = 0;
static int    prev_nnodes = 0;

Node *
popnode()
{
	Node *np;
	if (nodesp <= 0 ) return (Node *)0;
	np = nodestack[--nodesp];
	np->n_mark = 0;
	return np;
}

void
pushnode( np )
	Node * np;
{
	int new_nnodes;

	if (np->n_mark) return; /* already stacked */
	if (nodesp >= nnodes){
		/* see if this is the first time */
		if ( nodestack == NULL ){
			new_nnodes = NNODES;
			nnodes = NNODES;
			nodestack = (Node **)as_malloc( NNODES*sizeof(Node *));
		} else {
			/* use fibonacci reallocation */
			new_nnodes = nnodes + prev_nnodes;
			nodestack = (Node **)as_realloc(nodestack, new_nnodes*sizeof(Node *));
		}
		prev_nnodes = nnodes;
		nnodes = new_nnodes;
	}
	np->n_mark = 1;
	nodestack[nodesp++] = np;
}

/*
 * routines to do commonly-used liveset computations.
 * they return the new value of the liveset of the target node
 */

/* regular, boring case */
static register_set
compute_normal( np, succ )
	register Node * np;
	register_set succ;
{
	REGS_LIVE(np)= succ = 
		regset_add(
		    regset_sub(succ, REGS_WRITTEN(np)),
		    REGS_READ(np));
	return succ;
}

/* target node has a delay slot */
/* both of them need to be recomputed */
static register_set
compute_delay( np )
	register Node * np;
{
	Node * delay;
	register_set lp;

	delay = np->n_next; /* had better be pretty simple */
	/* first pick up target */
	/* (this may have been computed incrementally) */
	lp =  REGS_LIVE(np);
	/* then delay, which happens BEFORE the branch does */
	lp = compute_normal( delay, lp );
	/*
	 * except that, if the branch reads the condition codes, it does
	 * so BEFORE the delay slot instruction!!
	 */
	if (regset_in( REGS_READ(np), RN_ICC))
		lp = regset_incl( lp, RN_ICC);
	if (regset_in( REGS_READ(np), RN_FCC))
		lp = regset_incl( lp, RN_FCC);
	REGS_LIVE(np) = lp;
	return lp;
}

static register_set
compute_branch( np )
	register Node *np;
{
	Node *target = np->n_operands.op_symp->s_def_node;

	REGS_LIVE(np) = REGS_LIVE(target);
	if (BRANCH_IS_CONDITIONAL(np))
		REGS_LIVE(np) = regset_add( REGS_LIVE(np), REGS_LIVE(np->n_next->n_next));
	return compute_delay(np);
}


static register_set
compute_switch( np )
	register Node *np;
{
	Node *target ; /* = np->n_operands.op_symp->s_def_node; */
	register Node *csp;
	register_set r;

	/* back up to first CSWITCH of the lot */
	while (OP_FUNC(np->n_prev).f_group == FG_CSWITCH )
		np= np->n_prev;
	csp = np;

	/* now add em up */
	r = empty_regset;
	while (OP_FUNC(np).f_group == FG_CSWITCH ){
		if (np->n_operands.op_symp != NULL) {
			target = np->n_operands.op_symp->s_def_node;
			if (OP_FUNC(target).f_group == FG_SYM_REF)
				r = regset_add( r, REGS_LIVE(target->n_operands.op_symp->s_def_node) );
			else
				r = regset_add( r, REGS_LIVE(target) );
		}
		np = np->n_next;
	}
	/* finally, move that value around */
	np = csp;
	while (OP_FUNC(np).f_group == FG_CSWITCH ){
		REGS_LIVE(np) = r;
		np = np->n_next;
	}
	return r;
}
/*****************************************************************************/

/*
 * Propegate the live-register information through the list.
 *
 * Starting with the final, exit, node, we recede (thats precede backwards)
 * through the list as follows: if we encounter ...
 * 1) a label: use for its liveset that of its successor.
 *    put on the stack all the nodes that reference that label.
 * 2) an unconditional branch: (1 delay slot): the liveset of its
 *    successor, delay-slot, instruction must be recomputed using the
 *    successor of the branch, rather than the static successor of the
 *    instruction. The liveset of the branch itself is then the same as
 *    that of its successor.
 * 3) a conditional branch: (1 delay slot): again, all the action occurs
 *    in the delay slot. This time there are 2 possible successors to
 *    take into account.
 * 4) a multi-way branch: this is just like a conditional branch, but has
 *    n possible successors.
 * 5) a pseudo-op: if it doesn't generate any code, then its liveset is
 *    the same as its successor: information, as control,  just flows through.
 *    If it generates code, then it has {} for a liveset. Control does not
 *    flow through.
 * 6) a sequencial instruction: liveset is computed using this instruction
 *    and its successor.
 * 7) when we've made an initial pass over the program, pop a (branch)
 *    node off the stack and do some more computing.
 * 8) except for the initial pass, whenever we recompute a liveset, and its
 *    values is unchanged, we can stop computing this pass, since nothing
 *    more will change. In this case we pop another node off the stack
 *    and continue.
 * 9) the liveset for branches and their delay instructions will be
 *    computed incrementally, as we discover new successors to them.
 *    The alternative is to find all the successor nodes each time we
 *    want to compute the liveset. This tactic was used in the 68000
 *    c2 with apparent success, and will be our fall back if the above
 *    doesn't work.
 */
void
regset_propagate( marker )
	Node *marker;
{
	register Node *np, *user, *delay;
	Bool first = TRUE;
	register_set ls, old;

	/* start by putting the exit node on the stack */
	pushnode( marker->n_prev );

	/* while there are still nodes on the stack ... */
	while ((np = popnode()) != (Node *)0){
		ls = empty_regset;
		for(;np != marker; np = np->n_prev){
			switch (OP_FUNC(np).f_group){
			/*
			 * normally, each case here will end with a "continue".
			 * a break will break out of the for-loop.
			 */
			/* normal, boring instructions */
			case FG_LD:
			case FG_ST:
			case FG_LDST:
			case FG_ARITH:
			case FG_WR:
			case FG_SETHI:
			case FG_RD:
			case FG_FLOAT2:
			case FG_FLOAT3:
			case FG_NOP:
			case FG_UNIMP:
			case FG_TRAP:
			/* flow-through and generative pseudos */
			case FG_SET:
				old = REGS_LIVE(np);
				ls = compute_normal( np, ls );
				if (!first && regset_equal( old, ls ))
					break;
				continue;
			/* label */
			case FG_LABEL:
				REGS_LIVE(np)= ls;
				for (user = np->n_operands.op_symp->s_ref_list;
				     user;
				     user = user->n_symref_next){
					switch (OP_FUNC(user).f_group ){
					case FG_BR:
						old = REGS_LIVE(user);
						if ( first || !regset_equal(compute_branch(user), old))
							pushnode(user);
						break;
					/*  This case is really a switch node
					 * that has an expression instead of a
					 * label (for PIC).
					 */	
					case FG_SYM_REF:
						if (user->n_prev != NULL){
							old = REGS_LIVE(user->n_prev);
							if ( first || !regset_equal(compute_switch(user->n_prev), old))
						    		pushnode(user->n_prev);
						}
						break;
					case FG_CSWITCH:
						old = REGS_LIVE(user);
						if ( first || !regset_equal(compute_switch(user), old))
							pushnode(user);
						break;
					}
				}
				continue;
			/* other pseudos */
			case FG_EQUATE:
			case FG_PS:
			case FG_COMMENT:
				if (NODE_GENERATES_CODE(np)){
					/* opaque pseudos */
					ls = empty_regset;
					REGS_LIVE(np) = empty_regset;
				} else {
					/* flow-through pseudos */
					REGS_LIVE(np) = ls;
				}
				continue;
			/* call and jmpl have delay slot */
			case FG_JMPL:
			case FG_CALL:
				(void)compute_normal(np, REGS_LIVE(np->n_next->n_next));
				ls = compute_delay(np);
				continue;
			/* branches of various sorts */
			case FG_BR:
				/*
				 * a branch can have one successor if 
				 * unconditional, or two if conditional.
				 */

				ls = compute_branch(np);
				continue;
			case FG_CSWITCH:
				/*
				 * a switch is represented as a whole bunch of
				 * cswitch nodes. collectively, they will have
				 * many successors, none of which are "fall through"
				 */
				ls = compute_switch(np);
				/* push this mask all the way back to the JMP */
				while (OP_FUNC(np).f_group != FG_JMP){
					np = np->n_prev;
				}
				REGS_LIVE(np) = ls;
				ls = compute_delay(np);
				continue;
			case FG_JMP:
				/*
				 * this jmp is not part of a recognizable switch.
				 * since we don't know where it could go, we
				 * must assume the worst.
				 */
				 REGS_LIVE(np) = compiler_regset;
				ls = compute_delay(np);
				continue;
			case FG_RET:
				/* be careful -- there may be no 2nd successor here! */
				/* assume that the destination is beyond the
				   scope of this run */
				REGS_LIVE(np) = REGS_READ(np);
				ls = compute_delay(np);
				continue;
			default:
				internal_error("opt_regsets:regset_propatate(): unexpected f_group=%u",OP_FUNC(np).f_group);

			}
			break;
		}
		first = FALSE;
	}
}

/*
 * we changed or deleted a node. starting at np, recompute backwards
 * until things stop changing. this is potentially ruinous, but is not
 * often so in practice.
 */
void
regset_recompute( np )
	register Node *np;
{
	register Node  *user, *delay;
	register_set ls, old;

	/* start by putting the designated node on the stack */
	pushnode( np );

	/* while there are still nodes on the stack ... */
	while ((np = popnode()) != (Node *)0){
		ls = REGS_LIVE(np->n_next);
		for(;; np = np->n_prev){
			switch (OP_FUNC(np).f_group){
			/*
			 * normally, each case here will end with a "continue"
			 * a break will break out of the for-loop
			 */
			case FG_MARKER:
				break;
			/* normal, boring instructions */
			case FG_LD:
			case FG_ST:
			case FG_LDST:
			case FG_ARITH:
			case FG_WR:
			case FG_SETHI:
			case FG_RD:
			case FG_FLOAT2:
			case FG_FLOAT3:
			case FG_NOP:
			case FG_UNIMP:
			case FG_TRAP:
			/* flow-through and generative pseudos */
			case FG_SET:
				old = REGS_LIVE(np);
				ls = compute_normal( np, ls );
				if (regset_equal( old, ls ))
					break;
				continue;
			/* label */
			case FG_LABEL:
				REGS_LIVE(np)= ls;
				for (user=np->n_operands.op_symp->s_ref_list;
				     user;
				     user = user->n_symref_next){
					switch (OP_FUNC(user).f_group ){
					case FG_BR:
						old = REGS_LIVE(user);
						if ( !regset_equal(compute_branch(user), old))
							pushnode(user);
						break;
					case FG_CSWITCH:
						old = REGS_LIVE(user);
						if ( !regset_equal(compute_switch(user), old))
							pushnode(user);
						break;
					}
				}
				continue;
			/* other pseudos */
			case FG_EQUATE:
			case FG_PS:
			case FG_COMMENT:
				if (NODE_GENERATES_CODE(np)){
					/* opaque pseudos */
					break;
				} else {
					/* flow-through pseudos */
					REGS_LIVE(np) = ls;
					continue;
				}
			/* call and jmpl have delay slot */
			case FG_JMPL:
			case FG_CALL:
				(void)compute_normal(np, REGS_LIVE(np->n_next->n_next));
				ls = compute_delay(np);
				continue;
			/* branches of various sorts */
			case FG_BR:
				/*
				 * a branch can have one successor if 
				 * unconditional, or two if conditional.
				 */

				ls = compute_branch(np);
				continue;
			case FG_CSWITCH:
				/*
				 * a switch is represented as a whole bunch of
				 * cswitch nodes. collectively, they will have
				 * many successors, none of which are "fall through"
				 */
				ls = compute_switch(np);
				/* push this mask all the way back to the JMP */
				while (OP_FUNC(np).f_group != FG_JMP){
					np = np->n_prev;
				}
				REGS_LIVE(np) = ls;
				ls = compute_delay(np);
				continue;
			case FG_JMP:
				/*
				 * this jmp is not part of a recognizable switch.
				 * since we don't know where it could go, we
				 * must assume the worst.
				 */
				 REGS_LIVE(np) = compiler_regset;
				ls = compute_delay(np);
				continue;
			case FG_RET:
				/* be careful -- there may be no 2nd successor here! */
				/* assume that the destination is beyond the
				   scope of this run */
				REGS_LIVE(np) = REGS_READ(np);
				ls = compute_delay(np);
				continue;
			default:
				internal_error("opt_regsets:regset_recompute(): unexpected f_group=%u",OP_FUNC(np).f_group);
			}
			break;
		}
	}
}
/*****************************************************************************/

register_set
following_liveset( p )
	register Node *p;
{
	register Node *pp = p->n_prev;
	register_set r;
	if (NODE_HAS_DELAY_SLOT( pp)){
		switch (OP_FUNC(pp).f_group){
		case FG_BR:
			r = REGS_LIVE(pp->n_operands.op_symp->s_def_node);
			if ( BRANCH_IS_CONDITIONAL(pp) &&  !BRANCH_IS_ANNULLED(pp)){
				r = regset_add( r, REGS_LIVE(p->n_next));
			}
			return r;
		case FG_JMPL:
		case FG_CALL:
			return regset_add(
				regset_sub(REGS_LIVE(p->n_next), REGS_WRITTEN(pp)),
		    		REGS_READ(pp));
		case FG_JMP:
			if (OP_FUNC(p->n_next->n_next).f_group == FG_CSWITCH){
				return REGS_LIVE(p->n_next->n_next);
			} else {
				return REGS_LIVE(pp);
			}
		case FG_RET:
			return REGS_READ(pp);
		default:
			internal_error("opt_regsets:following_liveset(): unexpected f_group=%u",OP_FUNC(pp).f_group);
			/*NOTREACHED*/
		}
	} else {
		return REGS_LIVE(p->n_next);
	}
}
