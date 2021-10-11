static char sccs_id[] = "@(#)emit.c	1.1\t10/31/94";
/*
 * This file contains actions associated with the parse rules in "parse.y".
 *
 * Also see "actions_*.c".
 */

#include <stdio.h>
#include "structs.h"
#include "sym.h"
#include "y.tab.h"
#include "bitmacros.h"
#include "errors.h"
#include "actions.h"
#include "globals.h"
#include "lex.h"	/* for curr_flp */
#include "build_instr.h"
#include "segments.h"
#include "alloc.h"
#include "node.h"
#include "emit.h"
#include "disassemble.h"
#include "stab.h"
#include "opcode_ptrs.h"
#include "registers.h"
#include "set_instruction.h"
#include "ident.h"
#include "optimize.h"
#include "opt_utilities.h"

static void emit_0();		/* forward routine declaration */
static void emit_1();		/* forward routine declaration */
static void emit_2a();		/* forward routine declaration */
static void emit_2b();		/* forward routine declaration */
static void emit_3ab();		/* forward routine declaration */
static void emit_3cd11();	/* forward routine declaration */
static void emit_3cd01();	/* forward routine declaration */
static void emit_3cd00();	/* forward routine declaration */
static void emit_3cd10();	/* forward routine declaration */
static void emit_3cd1X();	/* forward routine declaration */
static void emit_3e11();	/* forward routine declaration */
static void emit_3e11_CPOP();	/* forward routine declaration */
static void emit_3e10();	/* forward routine declaration */
static void emit_3e01();	/* forward routine declaration */
static void emit_LABEL();	/* forward routine declaration */
static void emit_SET();		/* forward routine declaration */
static void emit_EQUATE();	/* forward routine declaration */
static void emit_MARKER();	/* forward routine declaration */
static void emit_PS();		/* forward routine declaration */
static void emit_CSWITCH();	/* forward routine declaration */
static void emit_FMOV2();  	/* forward routine declaration */
static void emit_LDST2();  	/* forward routine declaration */

void
emit_list_of_nodes(marker_np)
	register Node *marker_np;
{ 
	register Node *np;

	/* do nothing if list is non-existant */
	if (marker_np == NULL)	return;

        if (marker_np != stab_marker_np) do_stabs = FALSE;

	/* This works on the master (circular) list of doubly-linked nodes for
	 * a segment.  The beginning and end of the circular list is known by
	 * a marker node.
	 */
	emit_node(marker_np);
	for (np = marker_np->n_next;   np != marker_np;   np = np->n_next)
	{
		emit_node(np);
	}
}


static void
just_emit_a_node(np)
	register Node *np;
{
#ifdef DEBUG
	if (np == NULL)		internal_error("just_emit_a_node(): NULL np");
#endif

	switch ( np->n_opcp->o_binfmt )
	{
	case BF_IGNORE:		/* do nothing */	break;

	case BF_0:		emit_0(np);		break;
	case BF_1:		emit_1(np);		break;
	case BF_2a:		emit_2a(np);		break;
	case BF_2b:		emit_2b(np);		break;
	case BF_3ab:		emit_3ab(np);		break;
	case BF_3cd11:		emit_3cd11(np);		break;
        case BF_3cd01:          emit_3cd01(np);         break;
	case BF_3cd00:		emit_3cd00(np);		break;
	case BF_3cd10:		emit_3cd10(np);		break;
	case BF_3cd1X:		emit_3cd1X(np);		break;
	case BF_3e11:		emit_3e11(np);		break;
	case BF_3e11_CPOP:	emit_3e11_CPOP(np);	break;
	case BF_3e10:		emit_3e10(np);		break;
	case BF_3e01:		emit_3e01(np);		break;

	case BF_LABEL:		emit_LABEL(np);		break;
	case BF_SET:		emit_SET(np);		break;
	case BF_EQUATE:		emit_EQUATE(np);	break;
	case BF_MARKER:		emit_MARKER(np);	break;
	case BF_PS:		emit_PS(np);		break;
	case BF_CSWITCH:	emit_CSWITCH(np);	break;
	case BF_FMOV2:          emit_FMOV2(np);         break;
	case BF_LDST2:          emit_LDST2(np);         break;

#ifdef DEBUG
	default:
		internal_error("just_emit_a_node(): o_binfmt = %u",
				(unsigned int)(np->n_opcp->o_binfmt));
#endif
	}
}


void
emit_node(np)
	register Node *np;
{
	/* Set the input line# of the node, in case there are any errors
	 * during code emission.
	 */
	input_line_no = np->n_lineno;

	/* Emit the code associated with this node. */
	just_emit_a_node(np);

	/* The disassembly must be done AFTER the node "emission", since
	 * the "emission" code may fill in information needed for disassembly.
	 * Example: EQUATE.
	 */
	if (do_disassembly)	disassemble_node(np);
}


/* emit_relocations() emits the relocations specified in the node, if any,
 * for the word JUST emitted (i.e. at ".-4").
 */
static void
emit_relocations(np)
	register Node *np;
{
	if (np->n_operands.op_reltype != REL_NONE)
	{
		relocate(np->n_operands.op_reltype,
			 np->n_operands.op_symp,
			 get_dot_seg(),
			 get_dot()-4,
			 np->n_operands.op_addend,
			 input_filename,
			 np->n_lineno);
	}
}


static void
emit_0(np)
	register Node *np;
{
	/* An instruction which should be output with its bits as-is.
	 * Note that the register which is in np->n_operands.op_regs[O_RS1]
	 * for both RET and RETL is ALREADY INCLUDED in o_instr...
	 */
	emit_instr( np->n_opcp->o_instr );
}


static void
emit_1(np)			/* CALL */
	register Node *np;
{
	emit_instr( np->n_opcp->o_instr | MAKEINSTR_1(0/*op*/, 0/*disp30*/) );
	emit_relocations(np);
}


static void
emit_2a(np)			/* SETHI */
	register Node *np;
{
	emit_instr( np->n_opcp->o_instr |
		    MAKEINSTR_2A(
			0,	/* op */
			RAW_REGNO(np->n_operands.op_regs[O_RD]),
			0,	/* op2 */
			0	/* disp22 */ )
		  );
	emit_relocations(np);
}


static void
emit_2b(np)			/* BICC, BFCC */
	register Node *np;
{
	emit_instr( np->n_opcp->o_instr |
		    MAKEINSTR_2B(
			0,	/* op   */
			0,	/* a    */
			0,	/* cond */
			0,	/* op2  */
			0	/* disp22 */ )
		  );
	emit_relocations(np);
}


static void
emit_3ab(np)		/* for the TICC instructions */
	register Node *np;
{
	if ( np->n_operands.op_imm_used )
	{
		/* format 3ab[i=1] */

		emit_instr( np->n_opcp->o_instr |
			    MAKEINSTR_3IMM(
				0,	/* op */
				0,	/* ign+cond */
				0,	/* op3 */
				RAW_REGNO(np->n_operands.op_regs[O_RS1]),
				0	/* simm13 */ )
			  );
	}
	else
	{
		/* format 3ab[i=0] */

		emit_instr( np->n_opcp->o_instr |
			    MAKEINSTR_3REG(
				0,	/* op */
				0,	/* ign+cond */
				0,	/* op3 */
				RAW_REGNO(np->n_operands.op_regs[O_RS1]),
				0,	/* <ignored(asi)> */
				RAW_REGNO(np->n_operands.op_regs[O_RS2]) )
			  );
	}

	emit_relocations(np);
}

static void
emit_3cd01(np)          /* for the LD{FSR,CSR}/ST{FSR,FQ,CSR,CQ} instructions */
        register Node *np;
{
        if ( np->n_operands.op_imm_used )
        {
                /* format 3cd11[i=1] */
 
                emit_instr( np->n_opcp->o_instr |
                            MAKEINSTR_3IMM(
                                0,      /* op */
                                0,	/* "rd" is 0 */
                                0,      /* op3 */
                                RAW_REGNO(np->n_operands.op_regs[O_RS1]),
                                0       /* simm13 */ )
                          );
        }
        else
        {
                /* format 3cd11[i=0] */

                emit_instr( np->n_opcp->o_instr |
                            MAKEINSTR_3REG(
                                0,      /* op */
                                0,      /* "rd" is 0 */
                                0,      /* op3 */
                                RAW_REGNO(np->n_operands.op_regs[O_RS1]),
                                /* (op_asi will be 0 if op_asi_used == FALSE) */
                                np->n_operands.op_asi,  /* asi */
                                RAW_REGNO(np->n_operands.op_regs[O_RS2]) )
                          );
        }

        emit_relocations(np);
}




static void
emit_3cd11(np)		/* for the LD/ST instructions */
	register Node *np;
{
	if ( np->n_operands.op_imm_used )
	{
		/* format 3cd11[i=1] */

		emit_instr( np->n_opcp->o_instr |
			    MAKEINSTR_3IMM(
				0,	/* op */
				RAW_REGNO(np->n_operands.op_regs[O_RD]),
				0,	/* op3 */
				RAW_REGNO(np->n_operands.op_regs[O_RS1]),
				0	/* simm13 */ )
			  );
	}
	else
	{
		/* format 3cd11[i=0] */

		emit_instr( np->n_opcp->o_instr |
			    MAKEINSTR_3REG(
				0,	/* op */
				RAW_REGNO(np->n_operands.op_regs[O_RD]),
				0,	/* op3 */
				RAW_REGNO(np->n_operands.op_regs[O_RS1]),
				/* (op_asi will be 0 if op_asi_used == FALSE) */
				np->n_operands.op_asi,	/* asi */
				RAW_REGNO(np->n_operands.op_regs[O_RS2]) )
			  );
	}

	emit_relocations(np);
}


static void
emit_3cd00(np)		/* for the WR instruction */
	register Node *np;
{
	if ( np->n_operands.op_imm_used )
	{
		/* format 3cd00[i=1] */

		emit_instr( np->n_opcp->o_instr |
			    MAKEINSTR_3IMM(
				0,	/* op */
				0,	/* <ignored(rd)> */
				0,	/* op3 */
				RAW_REGNO(np->n_operands.op_regs[O_RS1]),
				0	/* simm13 */ )
			  );
	}
	else
	{
		/* format 3cd00[i=0] */

		emit_instr( np->n_opcp->o_instr |
			    MAKEINSTR_3REG(
				0,	/* op */
				0,	/* <ignored(rd)> */
				0,	/* op3 */
				RAW_REGNO(np->n_operands.op_regs[O_RS1]),
				0,	/* <ignored(asi/opf)> */
				RAW_REGNO(np->n_operands.op_regs[O_RS2]) )
			  );
	}

	emit_relocations(np);
}


static void
emit_3cd10(np)		/* for the remaining format-3 non-FPop instructions */
	register Node *np;
{
	if ( np->n_operands.op_imm_used )
	{
		/* format 3cd10[i=1] */

		emit_instr( np->n_opcp->o_instr |
			    MAKEINSTR_3IMM(
				0,	/* op */
				RAW_REGNO(np->n_operands.op_regs[O_RD]),
				0,	/* op3 */
				RAW_REGNO(np->n_operands.op_regs[O_RS1]),
				0	/* simm13 */ )
			  );
	}
	else
	{
		/* format 3cd10[i=0] */

		emit_instr( np->n_opcp->o_instr |
			    MAKEINSTR_3REG(
				0,	/* op */
				RAW_REGNO(np->n_operands.op_regs[O_RD]),
				0,	/* op3 */
				RAW_REGNO(np->n_operands.op_regs[O_RS1]),
				0,	/* <ignored(asi/opf)> */
				RAW_REGNO(np->n_operands.op_regs[O_RS2]) )
			  );
	}

	emit_relocations(np);
}


static void
emit_3cd1X(np)		/* for UNIMP, RD instructions */
	register Node *np;
{
	if ( np->n_operands.op_imm_used )
	{
		/* format 3cd1X[imm]: UNIMP instruction */

		emit_instr( np->n_opcp->o_instr |
			    MAKEINSTR_3(
				0,	/* op */
				RAW_REGNO(np->n_operands.op_regs[O_RD]),
				0,	/* op3 */
				0,	/* <ignored(rs1)> */
				0,	/* <ignored(imm)> */
				0	/* <ignored(low13)*/ )
			  );
	}
	else
	{
		/* format 3cd1X[non-imm]: RD instruction */

		emit_instr( np->n_opcp->o_instr |
			    MAKEINSTR_3REG(
				0,	/* op */
				RAW_REGNO(np->n_operands.op_regs[O_RD]),
				0,	/* op3 */
				0,	/* <ignored(rs1)> */
				0,	/* <ignored(asi/opf)> */
				0	/* <ignored(rs2)> */ )
			  );
	}

	emit_relocations(np);
}


static void
emit_3e11(np)		/* 3-operand floating-point instructions */
	register Node *np;
{
	emit_instr( np->n_opcp->o_instr |
		    MAKEINSTR_3REG(
			0,	/* op */
			RAW_REGNO(np->n_operands.op_regs[O_RD]),
			0,	/* op3 */
			RAW_REGNO(np->n_operands.op_regs[O_RS1]),
			0,	/* opf */
			RAW_REGNO(np->n_operands.op_regs[O_RS2]) )
		  );

#ifdef DEBUG
	if (np->n_operands.op_reltype != REL_NONE)
	{
		internal_error("emit_3e11(): op_reltype = %u",
				(unsigned int)(np->n_operands.op_reltype) );
	}
#endif
	/* There is nothing to relocate on this one,
	 *	so don't bother calling the routine:
	 * emit_relocations(np);
	 */
}


static void
emit_3e11_CPOP(np)		/* 3-operand coprocessor instructions */
	register Node *np;
{
	emit_instr( np->n_opcp->o_instr |
		    MAKEINSTR_3REG(
			0,	/* op */
			RAW_REGNO(np->n_operands.op_regs[O_RD]),
			0,	/* op3 */
			RAW_REGNO(np->n_operands.op_regs[O_RS1]),
			np->n_operands.op_cp_opcode,
			RAW_REGNO(np->n_operands.op_regs[O_RS2]) )
		  );

#ifdef DEBUG
	if (np->n_operands.op_reltype != REL_NONE)
	{
		internal_error("emit_3e11(): op_reltype = %u",
				(unsigned int)(np->n_operands.op_reltype) );
	}
#endif
	/* There is nothing to relocate on this one,
	 *	so don't bother calling the routine:
	 * emit_relocations(np);
	 */
}


static void
emit_3e10(np)		/* most 2-operand floating-point instructions */
	register Node *np;
{
	emit_instr( np->n_opcp->o_instr |
		    MAKEINSTR_3REG(
			0,	/* op */
			RAW_REGNO(np->n_operands.op_regs[O_RD]),
			0,	/* op3 */
			0,	/* <ignored(rs1)> */
			0,	/* opf */
			RAW_REGNO(np->n_operands.op_regs[O_RS2]) )
		  );

#ifdef DEBUG
	if (np->n_operands.op_reltype != REL_NONE)
	{
		internal_error("emit_3e10(): op_reltype = %u",
				(unsigned int)(np->n_operands.op_reltype) );
	}
#endif
	/* There is nothing to relocate on this one,
	 *	so don't bother calling the routine:
	 * emit_relocations(np);
	 */
}


static void
emit_3e01(np)		/* (2-operand) floating-pt Compare instructions */
	register Node *np;
{
	emit_instr( np->n_opcp->o_instr |
		    MAKEINSTR_3REG(
			0,	/* op */
			0,	/* <ignored(rd)> */
			0,	/* op3 */
			RAW_REGNO(np->n_operands.op_regs[O_RS1]),
			0,	/* opf */
			RAW_REGNO(np->n_operands.op_regs[O_RS2]) )
		  );

	/* There is nothing to relocate on this one,
	 *	so don't bother calling the routine:
	 * emit_relocations(np);
	 */
#ifdef DEBUG
	if (np->n_operands.op_reltype != REL_NONE)
	{
		internal_error("emit_3e01(): op_reltype = %u",
				(unsigned int)(np->n_operands.op_reltype) );
	}
#endif
}


static void
emit_LABEL(np)
	register Node *np;
{
	/* a label node */

	/* define the symbol */
	define_symbol(np->n_operands.op_symp, get_dot_seg(), get_dot());
}


static void
emit_SET(np)
	register Node *np;
{
	/* a "set" node */

	Node temp_node;

	/*Make a copy of the node, so we don't mess up the one we were passed.*/
	bcopy(np, &temp_node, sizeof(Node));
	temp_node.n_next = temp_node.n_prev = NULL; /* no pointers! */

	switch ( minimal_set_instruction(np->n_operands.op_symp,
					 np->n_operands.op_addend) )
	{
	case SET_JUST_SETHI:
		/* The source operand is an absolute value,
		 * and the 10 low-order bits are zero,
		 * so can use a single instruction:
		 *      SETHI  %hi(VALUE),RD
		 */
		temp_node.n_opcp = opcode_ptr[OP_SETHI];
		temp_node.n_operands.op_reltype     = REL_HI22;
		just_emit_a_node(&temp_node);
		break;

	case SET_JUST_MOV:
		/* The source operand is an absolute value, within the signed
		 * 13-bit Immediate range, so can use a single instruction:
		 *	MOV const13,RS2
		 * (actually "or  %0,const13,RD")
		 */
		temp_node.n_opcp = opcode_ptr[OP_OR];
		temp_node.n_operands.op_reltype     = REL_13;
		temp_node.n_operands.op_regs[O_RS1] = RN_GP(0);
		just_emit_a_node(&temp_node);
		break;

	case SET_SETHI_AND_OR:
		/* Turn SET into 2 instructions: a SETHI followed by a OR.
		 * np->n_operands.op_symp, np->n_operands.op_regs[O_RD], and
		 * n_operands.op_addend are already present in the SET
		 * pseudo-instruction, for both of the real instructions to use.
		 */

		/* First, modify our given node for the SETHI and emit it. */
		temp_node.n_opcp = opcode_ptr[OP_SETHI];
		temp_node.n_operands.op_reltype     = REL_HI22;
		just_emit_a_node(&temp_node);

		/* Then, do likewise for the appropriate OR instruction. */
		temp_node.n_opcp = opcode_ptr[OP_OR];
		temp_node.n_operands.op_reltype     = REL_LO10;
		temp_node.n_operands.op_regs[O_RS1] =
			temp_node.n_operands.op_regs[O_RD];
		just_emit_a_node(&temp_node);
		break;
	
	default:
		internal_error("emit_SET(): SET_?");
		/*NOTREACHED*/
	}
}

static void 
emit_FMOV2(np)
	register Node *np;
{
	Node temp_node;
	int i, width;

	/*Make a copy of the node, so we don't mess up the one we were passed.*/
	bcopy(np, &temp_node, sizeof(Node));
	temp_node.n_next = temp_node.n_prev = NULL; /* no pointers! */
	temp_node.n_opcp = opcode_ptr[OP_FMOVS];  /* Change it to an fmovs */
	width = BYTES_TO_WORDS(np->n_opcp->o_func.f_rd_width);
	if ((np->n_operands.op_regs[O_RS2] < np->n_operands.op_regs[O_RD]) &&
	    ((np->n_operands.op_regs[O_RS2] + width) > np->n_operands.op_regs[O_RD])){
		temp_node.n_operands.op_regs[O_RS2] += (width - 1);
		temp_node.n_operands.op_regs[O_RD] += (width - 1);
		just_emit_a_node(&temp_node);             /* Emit the damm thing */
		for (i = width; i > 1; i--){
			temp_node.n_operands.op_regs[O_RS2]--;    /* Change the registers */
			temp_node.n_operands.op_regs[O_RD]--;
			just_emit_a_node(&temp_node);     /* Emit additional instructions */
		}
	} else{
		just_emit_a_node(&temp_node);             /* Emit the damm thing */
		for (i = 1; i < width; i++){
			temp_node.n_operands.op_regs[O_RS2]++;    /* Change the registers */
			temp_node.n_operands.op_regs[O_RD]++;
			just_emit_a_node(&temp_node);     /* Emit additional instructions */
		}
	}
}
    
static void 
emit_LDST2(np)
	register Node *np;
{
	Node temp_node1, temp_node2;

	/* Make one copy of the node. */
	bcopy(np, &temp_node1, sizeof(Node));
	temp_node1.n_next = temp_node1.n_prev = NULL; /* no pointers! */
	switch (OP_FUNC(np).f_group){
	      case FG_LD:
		if (*np->n_opcp->o_name == '*') 
			temp_node1.n_opcp = opcode_ptr[OP_LDF];
	        else 
			temp_node1.n_opcp = opc_lookup("ld");
		break;

	      case FG_ST:
		if (*np->n_opcp->o_name == '*') 
			temp_node1.n_opcp = opcode_ptr[OP_STF];
		else
			temp_node1.n_opcp = opcode_ptr[OP_ST];
		break;
	}
	/* Make another copy of the node. */
	bcopy(&temp_node1, &temp_node2, sizeof(Node));
	temp_node2.n_next = temp_node2.n_prev = NULL; /* no pointers! */

	temp_node2.n_operands.op_regs[O_RD]++;     /* Change the register */
	if (temp_node2.n_operands.op_imm_used)     /* Change the address  */
		temp_node2.n_operands.op_addend += 4;
	else {
		temp_node2.n_operands.op_addend = 4;
		temp_node2.n_operands.op_imm_used = TRUE;
		temp_node2.n_operands.op_reltype = REL_13;
	}
	if (np->n_operands.op_regs[O_RS1] != np->n_operands.op_regs[O_RD]){
		just_emit_a_node(&temp_node1);   /* Emit the damm thing */
		just_emit_a_node(&temp_node2);   /* Emit the second part */
	} else {  /* do the reverse for things like "ld [%o0],%o0" */
		just_emit_a_node(&temp_node2);   /* Emit the second part */
		just_emit_a_node(&temp_node1);   /* Emit the damm thing */
	}
}	




static void
emit_EQUATE(np)
	register Node *np;
{
	/* an EQUATE (=) node */

	if ( BIT_IS_OFF(np->n_operands.op_symp->s_attr, SA_DEFINED) )
	{
		/* The symbol is not yet defined; define it if we can.
		 * All EQUATE'd symbols except those equated to expressions
		 * involving relocatable values should have been defined during
		 * the input phase.
		 */

		if ( evaluate_symbol_expression( np->n_operands.op_symp,
						 get_dot_seg(), get_dot() )
		   )
		{
			/* Succeeded in evaluating the expression;
			 * finish defining the symbol.
			 */
			define_symbol( np->n_operands.op_symp,
				       np->n_operands.op_symp->s_segment,
				       np->n_operands.op_symp->s_value );
		}
		else
		{
			/* Failed at the expression evaluation (and got an
			 * error msg from evaluate_symbol_expression()).
			 * At least define the "Equate" symbol (give it a
			 * value of 0), so it doesn't come up as Undefined.
			 */
			define_symbol( np->n_operands.op_symp, ASSEG_ABS,
					(long)0 );
		}
	}
}


static void
emit_MARKER(np)
	register Node *np;
{
	change_named_segment(np->n_string);
}


static long int
verify_byteval(val)
	register long int val;
{
	if ( (val < -0x80) || (val > 0xff) )
		   /*-128*/	    /*255*/
	{
		error(ERR_VAL_NOFIT, input_filename, input_line_no, 8);
		val = 0;

	}
	return val;
}


static long int
verify_halfval(val)
	register long int val;
{
	if ( (val < -0x00008000) || (val > 0x0000ffff) )
		     /*-32768*/             /*65535*/
	{
		error(ERR_VAL_NOFIT, input_filename, input_line_no, 16);
		val = 0;
	}
	return val;
}


static void
emit_word_node(np)
	register Node *np;
{
	/* emit a word value */
	if (np->n_operands.op_symp == NULL)
	{
		emit_word( (U32BITS)(np->n_operands.op_addend) );
	}
	else
	{
		emit_word( (U32BITS)(0) );
		relocate(np->n_operands.op_reltype,
			 np->n_operands.op_symp,
			 get_dot_seg(), get_dot()-4,
			 np->n_operands.op_addend,
			 input_filename, np->n_lineno);
	}
}


static void
emit_PS_OPS(np)
	register Node *np;
{
	switch (np->n_opcp->o_func.f_subgroup)
	{
	case FSG_ALIGN:
		/* align on given boundary*/
		align(np->n_operands.op_addend, FALSE);
		break;

	case FSG_SKIP:	/* ".skip" pseudo-op */
		/* We already verifed in eval_pseudo() that there is just one
		 * list element.
		 */
		emit_skipbytes( get_dot_seg(),
				(U32BITS)(np->n_operands.op_addend) );
		break;

	case FSG_PROC:
	case FSG_NOALIAS:
		/* ignore this node now; it was used only in optimizing pass.*/
		break;

	case FSG_BYTE:
		/* emit a byte value */
		if (np->n_operands.op_symp == NULL)
		{
			emit_byte( (U8BITS)
				   (verify_byteval(np->n_operands.op_addend)) );
		}
		else
		{
			emit_byte( (U8BITS)(0) );
			relocate(np->n_operands.op_reltype,
				 np->n_operands.op_symp,
				 get_dot_seg(), get_dot()-1,
				 np->n_operands.op_addend,
				 input_filename, np->n_lineno);
		}
		break;

	case FSG_HALF:
		/* emit a halfword value */
		if (np->n_operands.op_symp == NULL)
		{
			emit_halfword( (U16BITS)
				   (verify_halfval(np->n_operands.op_addend)) );
		}
		else
		{
			emit_halfword( (U16BITS)(0) );
			relocate(np->n_operands.op_reltype,
				 np->n_operands.op_symp,
				 get_dot_seg(), get_dot()-2,
				 np->n_operands.op_addend,
				 input_filename, np->n_lineno);
		}
		break;

	case FSG_WORD:
		/* emit a word value */
		emit_word_node(np);
		break;

#ifdef DEBUG
	default:
		internal_error("emit_PS_OPS(): bad f_subgroup=%u",
				(unsigned int)(np->n_opcp->o_func.f_subgroup));
#endif
	}
}


static void
emit_PS_NONE(np)
	register Node *np;
{
	switch (np->n_opcp->o_func.f_subgroup)
	{
	case FSG_EMPTY:
	case FSG_ALIAS:
		break;
#ifdef DEBUG
	default:
		internal_error("emit_PS_NONE(): bad f_subgroup=%u",
				(unsigned int)(np->n_opcp->o_func.f_subgroup));
#endif
	}
}


static void
emit_PS_STRG(np)
	register Node *np;
{
	switch (np->n_opcp->o_func.f_subgroup)
	{
	case FSG_SEG:		/* ".seg" pseudo-op */
		change_named_segment(np->n_string);
		break;	
	case FSG_OPTIM:		/* ".optim" pseudo-op */
		/* ignore it, at this stage. */
		break;	
        case FSG_IDENT:         /* ".ident" pseudo-op */
                queue_ident( np->n_string );
                break;
#ifdef DEBUG
	default:
		internal_error("emit_PS_STRG(): bad f_subgroup=%u",
				(unsigned int)(np->n_opcp->o_func.f_subgroup));
#endif
	}
}


static void
emit_PS_FLP(np)
	register Node *np;
{
	switch (np->n_opcp->o_func.f_subgroup)
	{
	case FSG_BOF:	/* beginning-of-input-file node */
		curr_flp	  = np->n_flp;
		input_filename    = curr_flp->fl_fnamep;
		break;
#ifdef DEBUG
	default:
		internal_error("emit_PS_FLP(): bad f_subgroup=%u",
				(unsigned int)(np->n_opcp->o_func.f_subgroup));
#endif
	}
}


static void
emit_PS_VLHP(np)
	register Node *np;
{
	register struct opcode	      *opcp;
	register struct val_list_head *vlhp;
	register struct value	      *vp;
	register int		       len;
	U32BITS fval[4];	/* scratch area in which to build f.p. values */

	opcp = np->n_opcp;
	vlhp = np->n_vlhp;

	switch (opcp->o_func.f_subgroup)
	{
	case FSG_ASCII:
	case FSG_ASCIZ:
		for (vp = vlhp->vlh_head;   vp != NULL;   vp = vp->v_next)
		{
			/* Emit an ASCII string */

			/* We already verifed in eval_pseudo() that all list
			 * elements are quoted strings.
			 */
			register char *cp;

			for (cp = vp->v_strptr, len = vp->v_strlen;
			     len > 0;  len--,cp++)
			{
				emit_byte( (U8BITS)(*cp) );
			}

			if (opcp->o_func.f_subgroup == FSG_ASCIZ)
			{
				emit_byte('\0');
			}
		}
		break;

	case FSG_SGL:
	case FSG_DBL:
	case FSG_QUAD:
		len = (int)(opcp->o_igroup); /* #words to emit, per list item */
#ifdef USE_REAL_DBL_ALIGNMENT
		/* get on word or doubleword boundary*/
		align((len==1)?4:8, TRUE);
#else /*USE_REAL_DBL_ALIGNMENT*/
		align(4, TRUE); /* only give WORD alignment */
#endif /*USE_REAL_DBL_ALIGNMENT*/
		for (vp = vlhp->vlh_head;   vp != NULL;   vp = vp->v_next)
		{
			/* emit a floating-point value */
			register int i;
			convert_to_fp(vp->v_strptr, len, &fval[0]);
			for (i = 0;   i < len;    i++)	emit_word(fval[i]);
		}
		break;
	case FSG_STAB:	/* stabd, stabn, stabs */
		/* We already verifed in eval_pseudo() that the proper
		 * number of list elements (3, 4, or 5) are present.
		 */
		{
			register int i;
			struct value *(vps[5]);

			/* Put the value pointers into an array, for easy
			 * passing to emit_stab().
			 */
			for (vp = vlhp->vlh_head, i = 0;    vp != NULL;
			     vp = vp->v_next, i++)
			{
				vps[i] = vp;
			}
			/* Zero the unused pointers (out of the 5). */
			while (i < 5)	vps[i++] = NULL;
	
			/* stabd have special treatment */
			if ( (opcp->o_igroup == 3) && !do_stabs )
                        {
                                Node *new_stabd_np;
                                new_stabd_np = new_node(np->n_lineno, opc_lookup(".stabd"));
                                copy_node(np,new_stabd_np,OPT_NONE);
                                new_stabd_np->n_vlhp->vlh_head->v_next->v_value = get_dot();
                                add_stab_node_to_list(new_stabd_np);
                                /* Leave it as a nop so it goes away soon
                                 * and it will make dealing with n_vlhp easy.
                                 */
                                np->n_opcp = opcode_ptr[OP_NOP];
			}
			else emit_stab(opcp->o_igroup/*(the arg count)*/, vps);
		}
		break;

	case FSG_COMMON:	/* ".common" pseudo-op */
		/* nothing to emit here; just ignore these pseudos */
		break;
#ifdef DEBUG
	default:
		internal_error("emit_PS_VLHP(): bad f_subgroup=%u",
				(unsigned int)(opcp->o_func.f_subgroup));
#endif
	}
}


static void
emit_PS(np)
	register Node *np;
{
	switch (np->n_opcp->o_func.f_node_union)
	{
	case NU_NONE:	emit_PS_NONE(np);	break;
	case NU_FLP:	emit_PS_FLP(np);	break;
	case NU_VLHP:	emit_PS_VLHP(np);	break;
	case NU_OPS:	emit_PS_OPS(np);	break;
	case NU_STRG:	emit_PS_STRG(np);	break;
#ifdef DEBUG
	default:
		internal_error("emit_PS(): f_node_union == %u?",
			(unsigned int)(np->n_opcp->o_func.f_node_union) );
#endif
	}
}


static void
emit_CSWITCH(np)
	register Node *np;
{
#ifdef DEBUG
	if(!NODE_IS_CSWITCH(np)) internal_error("emit_CSWITCH(): not CSWITCH?");
#endif /*DEBUG*/

	emit_word_node(np);
}
