static char sccs_id[] = "@(#)actions_instr.c	1.1\t10/31/94";
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
#include "instructions.h"
#include "globals.h"
#include "lex.h"
#include "build_instr.h"
#include "segments.h"
#include "alloc.h"
#include "node.h"
#include "scratchvals.h"
#include "obj.h"
#include "stab.h"
#include "opcode_ptrs.h"
#include "registers.h"
#include "set_instruction.h"

static Node * form_wr_node();	/* forward routine declaration */
static Node * form_rd_node();	/* forward routine declaration */
static Node * form_fp_node();	/* forward routine declaration */
static Node * form_cp_node();	/* forward routine declaration */
static Node * form_1opsE();	/* forward routine declaration */
static Node * form_arithRCR();	/* forward routine declaration */
static Node * form_sfa_ldst();	/* forward routine declaration */
static void   fill_in_call_argcount_field();	/* forward routine decl'n */


Node *
eval_opc0ops(opcp)
	register struct opcode *opcp;
{
	register Node *np;

	np = new_node(input_line_no, opcp);

	if (np->n_opcp->o_func.f_group == FG_RET)
	{
		np->n_operands.op_regs[O_RD] = RN_GP(0);

		switch ( np->n_opcp->o_func.f_subgroup )
		{
		case FSG_RET:
			np->n_operands.op_regs[O_RS1] = RN_GP_IN(7); /* %i7 */
			break;

		case FSG_RETL:
			np->n_operands.op_regs[O_RS1] = RN_GP_OUT(7); /* %o7 */
			break;
		}
	}

	return  np;
}


#ifdef CHIPFABS
static void
append_a_NOP(np)
	register Node *np;
{
	register Node *nop_np;
#  ifdef DEBUG
	if(debug)fprintf(stderr,"[NOP brute-force-appended @line# %d]\n",
			np->n_lineno);
#  endif

	/* create a node with a "nop" instruction */
	nop_np = new_node(np->n_lineno, opcode_ptr[OP_NOP]);
	nop_np->n_internal = TRUE;  /* this is an internally-generated node */

	/* ...and link the two nodes together. */
	np->n_next = nop_np;
}
#endif /* CHIPFABS*/


#ifndef ALWAYS_BUILD_NODE_LIST
static Node *
prepend_a_NOP(np)
	register Node *np;
{
	register Node *nop_np;
#  ifdef DEBUG
	if(debug)fprintf(stderr,"[NOP brute-force-inserted @line# %d]\n",
			np->n_lineno);
#  endif

	/* create a node with a "nop" instruction */
	nop_np = new_node(np->n_lineno, opcode_ptr[OP_NOP]);
	nop_np->n_internal = TRUE;  /* this is an internally-generated node */

	/* ...and link the two nodes together. */
	nop_np->n_next = np;

	return nop_np;
}
#endif /*ALWAYS_BUILD_NODE_LIST*/


#ifdef CHIPFABS
static void
append_an_FMOV(np, regno)	/* insert a FMOV from an FPreg to itself */
	register Node *np;
	register Regno regno;
{
	register Node *fmov_np;
#  ifdef DEBUG
	if(debug)fprintf(stderr,"[FMOV brute-force-inserted @line# %d]\n",
			np->n_lineno);
#  endif

	/* create a node with an "fmovs" instruction */
	fmov_np = new_node(np->n_lineno, opcode_ptr[OP_FMOVS]);
	fmov_np->n_operands.op_regs[O_RS2] = regno;
	fmov_np->n_operands.op_regs[O_RD]  = regno;
	fmov_np->n_internal = TRUE;  /* this is an internally-generated node */

	/* ...and link the two nodes together. */
	np->n_next = fmov_np;
}
#endif /*CHIPFABS*/


static Regno
check_double_regno(regno)
	register Regno regno;
{
	if( TST_BIT(regno, 01) )
	{
		/* EVEN-numbered register is required for doubleword
		 * register Load and Store instructions.
		 */
		error(ERR_ODDREG, input_filename, input_line_no);
		CLR_BIT(regno, 01);	/* force it even */
	}

	return regno;
}


/* used for LD's, ST's, LDSTUB's, & SWAP's, of all sizes */
Node *
eval_ldst(opcp, operp, regno)
	register struct opcode   *opcp;	/* opcode ptr		*/
	register struct operands *operp;/* operands ptr		*/
	register Regno		  regno;/* RD specified in instr */
{
	register Node *np;
	register Regno checked_regno;

	switch (opcp->o_func.f_rd_width)
	{
	case 1:	/* byte */
#ifdef CHIPFABS
		if ( (iu_fabno == 1) &&
		     (opcp->o_func.f_group == FG_LDST) &&
		     (opcp->o_func.f_subgroup == FSG_LSB) )
		{
			/* "ldstub" instruction not implemented on 1st-fab IU */
			error(ERR_UNK_OPC, input_filename, input_line_no,
				opcp->o_name);
		}
#endif /*CHIPFABS*/
		/* byte loads and stores may only be done on GP regs */
		checked_regno = chk_regno(regno, RM_GP);
		break;

	case 2:	/* halfword */
		/* halfword loads and stores may only be done on GP regs */
		checked_regno = chk_regno(regno, RM_GP);
		break;

	case 4:	/* word */
		if ( IS_A_GP_REG(regno) )
		{
			checked_regno = regno;
		}
		else if ( IS_AN_FP_REG(regno) )
		{
		    checked_regno = RN_FP(RAW_REGNO(regno));

		    if (opcp->o_func.f_ldst_alt)
		    {
#ifdef CHIPFABS	/* Alt-space FP ld/st's are only supported in fab#1 IU chip */
			if (iu_fabno == 1)
			{
			    switch (opcp->o_func.f_group)
			    {
			    case FG_LD:	opcp = opcode_ptr[OP_LDFA];	break;
			    case FG_ST:	opcp = opcode_ptr[OP_STFA];	break;
			    }
			}
			else
			{
			    error(ERR_INV_REG, input_filename, input_line_no);
			    checked_regno = ((Regno)0);
			}
#else /*CHIPFABS*/
			error(ERR_INV_REG, input_filename, input_line_no);
			checked_regno = ((Regno)0);
#endif /*CHIPFABS*/
		    }
		    else
		    {
			switch (opcp->o_func.f_group)
			{
			case FG_LD:	opcp = opcode_ptr[OP_LDF];	break;
			case FG_ST:	opcp = opcode_ptr[OP_STF];	break;
			}
		    }
		}
		else if ( IS_FSR_REG(regno) )
		{
		    checked_regno = regno;

		    if (opcp->o_func.f_ldst_alt)
		    {
#ifdef CHIPFABS	/* Alt-space FP ld/st's are only supported in fab#1 IU chip */
			if (iu_fabno == 1)
			{
			    switch (opcp->o_func.f_group)
			    {
			    case FG_LD:	opcp = opcode_ptr[OP_LDFSRA];	break;
			    case FG_ST:	opcp = opcode_ptr[OP_STFSRA];	break;
			    }
			}
			else
			{
			    error(ERR_INV_REG, input_filename, input_line_no);
			    checked_regno = ((Regno)0);
			}
#else /*CHIPFABS*/
			error(ERR_INV_REG, input_filename, input_line_no);
			checked_regno = ((Regno)0);
#endif /*CHIPFABS*/
		    }
		    else
		    {
			switch (opcp->o_func.f_group)
			{
			case FG_LD:	opcp = opcode_ptr[OP_LDFSR];	break;
			case FG_ST:	opcp = opcode_ptr[OP_STFSR];	break;
			}
		    }
		}
#ifdef CHIPFABS	/* coprocessor instr's are not supported in fab#1 IU chip */
		else if ( (iu_fabno != 1) &&
			  IS_A_CP_REG(regno) && (!opcp->o_func.f_ldst_alt) )
#else /*CHIPFABS*/
		else if ( IS_A_CP_REG(regno) && (!opcp->o_func.f_ldst_alt) )
#endif /*CHIPFABS*/
		{
			switch (opcp->o_func.f_group)
			{
			    case FG_LD:	opcp = opcode_ptr[OP_LDC];	break;
			    case FG_ST:	opcp = opcode_ptr[OP_STC];	break;
			}

			checked_regno = regno;
		}
#ifdef CHIPFABS	/* coprocessor instr's are not supported in fab#1 IU chip */
		else if ( (iu_fabno != 1) &&
			  IS_CSR_REG(regno) && (!opcp->o_func.f_ldst_alt) )
#else /*CHIPFABS*/
		else if ( IS_CSR_REG(regno) && (!opcp->o_func.f_ldst_alt) )
#endif /*CHIPFABS*/
		{
			switch (opcp->o_func.f_group)
			{
			    case FG_LD:	opcp = opcode_ptr[OP_LDCSR];	break;
			    case FG_ST:	opcp = opcode_ptr[OP_STCSR];	break;
			}

			checked_regno = regno;
		}
		else
		{
			error(ERR_INV_REG, input_filename, input_line_no);
			checked_regno = ((Regno)0);
		}


#ifdef CHIPFABS
		if ( (opcp->o_func.f_group == FG_LDST) &&
		     (((iu_fabno == 1) && (opcp->o_func.f_subgroup == FSG_SWAP))
			||
		      ((iu_fabno != 1) && (opcp->o_func.f_subgroup == FSG_LSW))
		     )
		   )
		{
			/* This is "swap" instruction, which is not implemented
			 * on the 1st-fab IU, or the "ldst" (word) instruction,
			 * which is ONLY implemented on the 1st-fab IU.
			 */
			error(ERR_UNK_OPC, input_filename, input_line_no,
				opcp->o_name);
		}
#endif /*CHIPFABS*/
		break;

	case 8:	/* doubleword */
		/* Doubleword loads and stores may be done from FP and GP regs*/
#ifdef CHIPFABS	/* doubleword LDs and STs are not supported in fab#1 IU chip */
		if ( (iu_fabno != 1) && IS_A_GP_REG(regno) )
#else /*CHIPFABS*/
		if (IS_A_GP_REG(regno))
#endif /*CHIPFABS*/
		{
			if  ((opcp == opc_lookup("ld2")) || (opcp == opc_lookup("st2"))){
				if ( (regno % 8) == 7 )
					error(ERR_BRY_DBL_REG, input_filename,
					      input_line_no);
				checked_regno = regno;
			}
			else   /* This is a real ldd/std so we have to check it */
				checked_regno = check_double_regno(regno);
		}
		else if (IS_AN_FP_REG(regno))
		{
			if (opcp == opc_lookup("ld2")) opcp = opc_lookup("*ld2");
			else if (opcp == opc_lookup("st2")) opcp = opc_lookup("*st2");
			else 
				switch (opcp->o_func.f_group)
				{
				      case FG_LD:	opcp = opcode_ptr[OP_LDDF];	break;
				      case FG_ST:	opcp = opcode_ptr[OP_STDF];	break;
				}
			
#ifdef CHIPFABS
			if (iu_fabno == 1)
			{
				checked_regno = check_double_regno(regno);
			}
			else	checked_regno = regno;
#else /*CHIPFABS*/
			checked_regno = regno;
#endif /*CHIPFABS*/
		}
#ifdef CHIPFABS	/* coprocessor instr's are not supported in fab#1 IU chip */
		else if ( (iu_fabno != 1) && IS_A_CP_REG(regno) )
#else /*CHIPFABS*/
		else if ( IS_A_CP_REG(regno) )
#endif /*CHIPFABS*/
		{
			switch (opcp->o_func.f_group)
			{
			case FG_LD:	opcp = opcode_ptr[OP_LDDC];	break;
			case FG_ST:	opcp = opcode_ptr[OP_STDC];	break;
			}

#ifdef CHIPFABS
			if (iu_fabno == 1)
			{
				checked_regno = check_double_regno(regno);
			}
			else	checked_regno = regno;
#else /*CHIPFABS*/
			checked_regno = regno;
#endif /*CHIPFABS*/
		}
		else if ( IS_FQ_REG(regno) && (opcp->o_func.f_group == FG_ST) )
		{
			/* the FQ "reg" may only have doubleword ST's done */
			opcp = opcode_ptr[OP_STDFQ];
			checked_regno = regno;
		}
#ifdef CHIPFABS	/* coprocessor instr's are not supported in fab#1 IU chip */
		else if ( (iu_fabno != 1) &&
		     IS_CQ_REG(regno) && (opcp->o_func.f_group == FG_ST) )
#else /*CHIPFABS*/
		else if ( IS_CQ_REG(regno) && (opcp->o_func.f_group == FG_ST) )
#endif /*CHIPFABS*/
		{
			/* the CQ "reg" may only have doubleword ST's done */
			opcp = opcode_ptr[OP_STDCQ];
			checked_regno = regno;
		}
		else
		{
			error(ERR_INV_REG, input_filename, input_line_no);
			checked_regno = ((Regno)0);
		}

		break;

	default:
		internal_error("eval_ld(): bad f_rd_width");
	}

	np = new_node(input_line_no, opcp);
	np->n_operands = *operp;	/* get RS1, RS2, etc */
	free_operands(operp);

	np->n_operands.op_regs[O_RD] = checked_regno;

	/* ...and in case there was a symbol */
	node_references_label(np, np->n_operands.op_symp);

#ifdef CHIPFABS
	/* For IU chip fab#1, insert a NOP after every LD to a GP reg.
	 * This is normally done more optimally in perform_fab_workarounds()
	 * in a later pass.
	 * But if optimization is NOT turned on, do it here instead.
	 */
	if ( (iu_fabno == 1) && USE_BRUTE_FORCE_FAB_WORKAROUNDS &&
	     (opcp->o_func.f_group == FG_LD) && IS_A_GP_REG(regno)
	   )
	{
		append_a_NOP(np);
	}
#endif /*CHIPFABS*/

	return np;
}


static void
chk_sfa_relocs(vp)
	register struct value *vp;
{
	if ( vp->v_relmode != VRM_NONE )
	{
		/* illegal to have %hi or %lo here! */
		error(ERR_PCT_HILO, input_filename, input_line_no);
		vp->v_relmode = VRM_NONE;
		vp->v_symp    = NULL;
		vp->v_value   = 0;
	}
}


Node *
eval_sfa_ldst(opcp, vp, regno)	/* used for LD's & ST's */
	struct opcode *opcp;
	struct value *vp;
	Regno regno;
{
	register Node *np;

	np = form_sfa_ldst(opcp, vp, regno);

	/* In any case, the scratch value structure has served its purpose,
	 * and is no longer needed.
	 */
	free_scratch_value(vp);

	return np;
}


static Node *
form_sfa_ldst(opcp, vp, regno)	/* used for LD's & ST's */
	struct opcode *opcp;
	struct value *vp;
	Regno regno;
{
	register struct operands *operp;

	chk_sfa_relocs(vp);

	operp = get_operands13(vp);
	operp->op_regs[O_RS1] = 0;	/* will be filled in by linker */
	operp->op_reltype = REL_SFA;	/*...the reloc'n as we really want it*/

	/* do basic instruction */
	return eval_ldst(opcp, operp, regno);
}


Node *
eval_ldsta(opcp, operp, regno, asi)	/* used for LDA's, LDSTA's, & STA's */
	struct opcode *opcp;
	register struct operands *operp;
	Regno regno;
	unsigned int asi;
{
	operp->op_asi_used = TRUE;
	operp->op_asi      = asi;
	return eval_ldst(opcp, operp, regno);
}


Node *
eval_arithNOP(opcp)		/* save & restore, with no operands */
	register struct opcode *opcp;
{
	register Node *np;
	
	if (opcp->o_igroup & IG_SVR)	/* SAVE and RESTORE */
	{
		np = eval_arithRRR(opcp, RN_GP(0), RN_GP(0), RN_GP(0));
	}
	else
	{
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}

	return np;
}

Node *
eval_arithRR(opcp, regno1, regno2)
	register struct opcode *opcp;
	register Regno regno1, regno2;
{
	register Node *np;

	if (opcp->o_igroup & IG_WR)
	{
		np = form_wr_node(regno1, regno2, FALSE, RN_GP(0), NULL);
	}
	else
	{
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}
	
	return np;
}


Node *
eval_arithRRR(opcp, rs1, rs2, rd)
	register struct opcode *opcp;
	register Regno rs1, rs2, rd;
{
	register Node *np;

	rs2 = chk_regno(rs2, RM_GP);

	if (opcp->o_igroup & IG_WR)
	{
		np = form_wr_node(rs1, rd, FALSE, rs2, NULL);
	}
	else	/* a real ARITH instruction */
	{
		rs1 = chk_regno(rs1, RM_GP);
		rd =  chk_regno(rd,  RM_GP);

		np = new_node(input_line_no, opcp);
		np->n_operands.op_regs[O_RS1] = rs1;
		np->n_operands.op_regs[O_RS2] = rs2;
		np->n_operands.op_regs[O_RD]  = rd;
	}
	
	return np;
}


Node *
eval_arithCRR(opcp, vp, regno1, destregno)
	register struct opcode *opcp;
	register struct value *vp;
	register Regno regno1;
	register Regno destregno;
{
	register Node *np;

	if (opcp->o_func.f_ar_commutative)
	{
		/* source operands of this instruction are commutative */
		np = form_arithRCR(opcp, regno1, vp, destregno);
	}
	else
	{
		/* operands of this ARITH instructions aren't commutative */
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}

	/* In any case, the scratch value structure has served its purpose,
	 * and is no longer needed.
	 */
	free_scratch_value(vp);

	return np;
}


Node *
eval_arithRCR(opcp, regno1, vp, dregno)
	register struct opcode *opcp;
	register Regno regno1;
	register struct value *vp;
	register Regno dregno;
{
	register Node *np;

	np = form_arithRCR(opcp, regno1, vp, dregno);

	/* In any case, the scratch value structure has served its purpose,
	 * and is no longer needed.
	 */
	free_scratch_value(vp);

	return np;
}


static Node *
form_arithRCR(opcp, regno1, vp, dregno)
	register struct opcode *opcp;
	register Regno regno1;
	register struct value *vp;
	register Regno dregno;
{
	register Node *np;

	if ((vp->v_relmode == VRM_NONE) || (vp->v_relmode == VRM_LO10))
	{
		if (opcp->o_igroup & IG_WR)
		{
			np = form_wr_node(regno1, dregno, TRUE, RN_NONE, vp);
		}
		else	/* a real ARITH instruction */
		{
			regno1 = chk_regno(regno1, RM_GP);
			dregno = chk_regno(dregno, RM_GP);

			np = new_node(input_line_no, opcp);
			np->n_operands.op_regs[O_RS1]= chk_regno(regno1, RM_GP);
			np->n_operands.op_regs[O_RD] = chk_regno(dregno, RM_GP);
			np->n_operands.op_imm_used   = TRUE;
			np->n_operands.op_symp       = vp->v_symp;
			np->n_operands.op_addend     = vp->v_value;
			if ( vp->v_relmode == VRM_LO10 )
			{
				np->n_operands.op_reltype = REL_LO10;
			}
			else	np->n_operands.op_reltype = REL_13;

			node_references_label(np, np->n_operands.op_symp);
		}
	}
	else
	{
		error(ERR_PCT_HILO, input_filename, input_line_no);
		np = NULL;
	}
	
	return np;
}


Node *
eval_3ops(opcp, rs1, rs2, rd)
	register struct opcode *opcp;
	register Regno rs1, rs2, rd;
{
#ifdef CHIPFABS
	register Node *np;
#endif /*CHIPFABS*/
#ifdef DEBUG
	if ((opcp->o_igroup&IG_FP)==0) internal_error("eval_3ops(): not IG_FP");
#endif

	/* 3-register-operand Floating-point instructions */

	rs1 = chk_regno(rs1, RM_FP);
	rs2 = chk_regno(rs2, RM_FP);
	rd  = chk_regno(rd,  RM_FP);

#ifdef CHIPFABS
	np = form_fp_node(opcp, rs1, rs2, rd);

	if ( np != NULL )
	{
		switch (fpu_fabno)
		{
		case 1:
			/* For FPU chip fab#1, insert an FMOV after every 1164
			 * or 1165 F.P. operation.  Every 3-operand F.P.
			 * instruction falls into this class.
			 */
			append_an_FMOV(np, rd);
			break;
		case 2:
			/* For FPU chip fab #2, insert an FMOV after each
			 * double-precision 1164 instruction (i.e. FMULD), only.
			 * This works around a bug in the FPU chaining logic.
			 */
			if ( USE_BRUTE_FORCE_FAB_WORKAROUNDS &&
			     OP_FUNC(np).f_fp_1164 &&
			     (OP_FUNC(np).f_rd_width == 8)
			   )
			{
				append_an_FMOV(np, rd);
			}
			break;
		default:
			/* Do nothing. */
			break;
		}
	}
	return  np;
#else /*CHIPFABS*/
	return  form_fp_node(opcp, rs1, rs2, rd);
#endif /*CHIPFABS*/
}


Node *
eval_4ops(opcp, cpop, rs1, rs2, rd)
	register struct opcode *opcp;
	register struct value *cpop;
	register Regno rs1, rs2, rd;
{
#ifdef DEBUG
	if ((opcp->o_igroup&IG_CP)==0) internal_error("eval_4ops(): not IG_CP");
#endif

	if (!VALUE_IS_ABSOLUTE(cpop)) warning();
        if ( (cpop->v_value < 0) || (cpop->v_value > (0x01ff)) )
                           /* 0 */                  /* 511 */
        {
                error(ERR_VAL_9, input_filename, input_line_no);
        }
	/* 3-register-operand coprocessor instructions */

	rs1 = chk_regno(rs1, RM_CP);
	rs2 = chk_regno(rs2, RM_CP);
	rd  = chk_regno(rd,  RM_CP);

	return form_cp_node(opcp, cpop->v_value, rs1, rs2, rd);

}


static struct opcode *
get_incdec_opcp(opcp)
	register struct opcode *opcp;
{
	/* convert an inc/dec instruction to a "real" instruction */

	switch ( opcp->o_igroup & IG_SUBGRP_MASK )
	{
	case ISG_I:	opcp = opcode_ptr[OP_ADD];	break;
	case ISG_IC:	opcp = opcode_ptr[OP_ADDCC];	break;
	case ISG_D:	opcp = opcode_ptr[OP_SUB];	break;
	case ISG_DC:	opcp = opcode_ptr[OP_SUBCC];	break;
#ifdef DEBUG
	default:  internal_error("get_incdec_opcp(): o_igroup = 0x%02x", (int)(opcp->o_igroup & IG_SUBGRP_MASK));
#endif
	}

	return opcp;
}


static struct opcode *
get_bit_opcp(opcp)
	register struct opcode *opcp;
{
	/* convert a bit instruction to a "real" instruction */

	switch ( opcp->o_igroup & IG_SUBGRP_MASK )
	{
	case ISG_BT:	opcp = opcode_ptr[OP_ANDCC];	break;	/* btst */
	case ISG_BS:	opcp = opcode_ptr[OP_OR];	break;	/* bset */
	case ISG_BC:	opcp = opcode_ptr[OP_ANDN];	break;	/* bclr */
	case ISG_BG:	opcp = opcode_ptr[OP_XOR];	break;	/* btog */
#ifdef DEBUG
	default:  internal_error("get_bit_opcp(): o_igroup = 0x%02x", (int)(opcp->o_igroup & IG_SUBGRP_MASK));
#endif
	}

	return opcp;
}


Node *
eval_ER(opcp, vp, regno1)
	register struct opcode *opcp;
	register struct value *vp;
	register Regno regno1;
{
	register Node *np;

	if (opcp->o_igroup & IG_SETHI)
	{
		regno1 = chk_regno(regno1, RM_GP);

		np = new_node(input_line_no, opcp);
		np->n_operands.op_regs[O_RD] = regno1;
		np->n_operands.op_symp   = vp->v_symp;
		np->n_operands.op_addend = vp->v_value;
		/* op_imm_used is probably not referenced for SETHI, but
		 * "what the heck?" -- it DOES use an immediate value...
		 */
		np->n_operands.op_imm_used = TRUE;

		switch (vp->v_relmode)
		{
		case VRM_NONE:
			np->n_operands.op_reltype = REL_22;
			break;
		case VRM_HI22:
			np->n_operands.op_reltype = REL_HI22;
			break;
		case VRM_LO10:  /* error, for now */
			error(ERR_SETHI_LO, input_filename, input_line_no);
			free_node(np);	/* oops.. changed our minds */
			np = NULL;
			break;
#ifdef DEBUG
		default:  internal_error("eval_ER(): relmode = %d", (int)(vp->v_relmode));
#endif
		}

		/* ...and in case there was a symbol */
		if (np != NULL)
		{
			node_references_label(np, np->n_operands.op_symp);
		}
	}
	else if (opcp->o_igroup & IG_SET)
	{
		if (vp->v_relmode == VRM_NONE)
		{
			regno1 = chk_regno(regno1, RM_GP);

			switch( minimal_set_instruction(vp->v_symp,
							vp->v_value) )
			{
			case SET_JUST_SETHI:
				/* The source operand is an absolute value,
				 * and the 10 low-order bits are zero,
				 * so can use a single instruction:
				 *      SETHI  %hi(VALUE),regno1
				 */
				np = new_node(input_line_no,
					      opcode_ptr[OP_SETHI]);
				np->n_operands.op_reltype     = REL_HI22;
				break;

			case SET_JUST_MOV:
				/* The source operand is an absolute value,
				 * within the signed 13-bit Immediate range,
				 * so can use a single instruction:
				 *	MOV const13,regno1
				 * (actually "or  %0,const13,regno1")
				 */
				np = new_node(input_line_no, opcode_ptr[OP_OR]);
				np->n_operands.op_regs[O_RS1] = RN_GP(0);
				np->n_operands.op_reltype     = REL_13;
				break;

			case SET_SETHI_AND_OR:
				/* must expand "set" into 2 instructions,
				 * (a SETHI plus an OR);  leave it as a "set"
				 * node for now and do the expansion later,
				 * either during optimization or when it is
				 * emitted.
				 */
				np = new_node(input_line_no, opcp/*"set"*/);
				np->n_operands.op_reltype     = REL_NONE;
				/* REL_NONE will be changed later, when this
				 * is expanded into 2 instructions.
				 */
				break;
			
			default:
				internal_error("eval_ER(): SET_?");
				/*NOTREACHED*/
			}

			np->n_operands.op_regs[O_RD]  = regno1;
			np->n_operands.op_imm_used    = TRUE;
			np->n_operands.op_symp        = vp->v_symp;
			np->n_operands.op_addend      = vp->v_value;

			/* ...and in case there was a symbol */
			node_references_label(np, np->n_operands.op_symp);
		}
		else
		{
			error(ERR_PCT_HILO, input_filename, input_line_no);
			np = NULL;
		}
	}
	else if (opcp->o_igroup & IG_MOV)
	{
		if ((vp->v_relmode == VRM_NONE) || (vp->v_relmode == VRM_LO10))
		{	
			if ( IS_A_GP_REG(regno1) )
			{
				/* MOV to general-purpose register   */
				/* (actually "or  %0,const13,regno1")*/
				regno1 = chk_regno(regno1, RM_GP);

				np = new_node(input_line_no, opcode_ptr[OP_OR]);
				np->n_operands.op_regs[O_RS1] = RN_GP(0);
				np->n_operands.op_regs[O_RD]  = regno1;
				np->n_operands.op_imm_used    = TRUE;
				np->n_operands.op_symp        = vp->v_symp;
				np->n_operands.op_addend      = vp->v_value;
				if ( vp->v_relmode == VRM_LO10 )
				{
					np->n_operands.op_reltype = REL_LO10;
				}
				else /* (vp->v_relmode == VRM_NONE) */
				{
					np->n_operands.op_reltype = REL_13;
				}

				/* ...and in case there was a symbol */
				node_references_label(np,
						      np->n_operands.op_symp);
			}
			else
			{
				/* MOV to Y, PSR, WIM, or TBR:
				 * actually "wr %0,VAL,{y,psr,wim,tbr}" 
				 */
				np = form_wr_node(RN_GP(0), regno1, TRUE,
							RN_NONE, vp);
			}
		}
		else
		{
			error(ERR_PCT_HILO, input_filename, input_line_no);
			np = NULL;
		}
	}
	else if ( opcp->o_igroup & IG_ID/*inc/dec*/ )
	{
		/* INC   (actually   "add   regno1,<expr>,regno1")	*/
		/* INCCC (actually   "addcc regno1,<expr>,regno1")	*/
		/* DEC   (actually   "sub   regno1,<expr>,regno1")	*/
		/* DECCC (actually   "subcc regno1,<expr>,regno1")	*/
		np = form_arithRCR(get_incdec_opcp(opcp), regno1, vp, regno1);
	}
	else if (opcp->o_igroup & IG_BIT)
	{
		register Regno rd;

		if ( (opcp->o_igroup & IG_SUBGRP_MASK) == ISG_BT )
		{
			/* this is BTST pseudo-instruction;
			 *	there is no destination register.
			 */
			rd = RN_GP(0);
		}
		else rd = regno1;	/* dest reg same as source reg*/

		/* BTST  (actually   "andcc regno1,<expr>,%g0   ")	*/
		/* BSET  (actually   "or    regno1,<expr>,regno1")	*/
		/* BCLR  (actually   "andn  regno1,<expr>,regno1")	*/
		/* BTOG  (actually   "xor   regno1,<expr>,regno1")	*/
		np = form_arithRCR(get_bit_opcp(opcp), regno1, vp, rd);
	}
	else if (opcp->o_igroup & IG_CMP)
	{
		/* (actually this is "subcc  const13,regno1,%0", which isn't
		 *  even an instruction;  but we CAN do "subcc %0,regno1,%0"
		 *  iff the constant was 0)
		 */
		/* (note that vp->v_symp == NULL implies
		 *  that vp->v_relmode==VRM_NONE)
		 */
		if ( (vp->v_symp == NULL)  &&  (vp->v_value == 0) )
		{
			np = eval_arithRRR(opcode_ptr[OP_SUBCC], RN_GP(0),
						regno1, RN_GP(0));
		}
		else
		{
			error(ERR_CMP_EXP_REG, input_filename, input_line_no);
			np = NULL;
		}
	}
	else
	{
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}

	/* In any case, the scratch value structure has served its purpose,
	 * and is no longer needed.
	 */
	free_scratch_value(vp);

	return np;
}


Node *
eval_EE(opcp, v1p, v2p)
	register struct opcode *opcp;
	register struct value *v1p;
	register struct value *v2p;
{
	register Node *np;

	if ( opcp->o_igroup & IG_CALL )
	{
		/* Have a "call <routine>,<count-of-args-in-out-regs>", which
		 * is the same as "call <routine>" with one extra field filled
		 * in, in the operands structure.
		 */

		/* Form a regular CALL node, just as if we had
		 * "call <routine>".
		 */
		np = form_1opsE(opcp, v1p);

		/* Now, fill in the extra field. */
		fill_in_call_argcount_field(np, v2p);
	}
	else
	{
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}

	/* In any case, the scratch value structures have served their purpose,
	 * and are no longer needed.
	 */
	free_scratch_value(v1p);
	free_scratch_value(v2p);

	return np;
}


Node *
eval_2opsSfaR(opcp, vp, rd)
	register struct opcode *opcp;
	register struct value *vp;
	register Regno rd;
{
	register Node *np;

	/* only the "mov" mnenonic allows this syntax */

	if (opcp->o_igroup & IG_MOV)
	{
		chk_sfa_relocs(vp);	/* looks at vp->v_relmode... */

		if ( IS_A_GP_REG(rd) )
		{
			/* have a MOV to a general-purpose register
			 * (actually
			 *    "add  basereg(sym),offset(sym),rd")
			 */
			rd = chk_regno(rd, RM_GP);
			np = form_arithRCR(opcode_ptr[OP_ADD], RN_GP(0), vp,rd);

			/* now set the relocation type the way we REALLY want
			 * it, for Short-Form Addressing.
			 */
			np->n_operands.op_reltype = REL_SFA;

		}
		else
		{
			/* MOV to Y, PSR, WIM, or TBR:   ERROR */
			error(ERR_INV_REG,input_filename,input_line_no);
			np = NULL;
		}
	}
	else
	{
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}

	/* In any case, the scratch value structure has served its purpose,
	 * and is no longer needed.
	 */
	free_scratch_value(vp);

	return np;
}


static Node *
form_wr_node(rs1, destreg, imm, rs2, vp)
	register Regno rs1;	/* rs1 */
	register Regno destreg;	/* Y, PSR, WIM, TBR */
	Bool imm;		/* TRUE if immediate value used */
	Regno rs2;		/* if imm==TRUE: rs2 */
	struct value *vp;	/* if imm==FALSE: ptr to value for imm value */
{
	register struct opcode *opcp;	/* ptr to opcode info */
	register Node * np;

	switch ( destreg )
	{
	case RN_Y:	opcp = opcode_ptr[OP_WRY];	break;
	case RN_PSR:	opcp = opcode_ptr[OP_WRPSR];	break;
	case RN_WIM:	opcp = opcode_ptr[OP_WRWIM];	break;
	case RN_TBR:	opcp = opcode_ptr[OP_WRTBR];	break;
	default:
		/* invalid register specified */
		error(ERR_INV_REG, input_filename, input_line_no);
		return  (Node *)NULL;
	}

	rs1 = chk_regno(rs1, RM_GP);

	np = new_node(input_line_no, opcp);
	np->n_operands.op_regs[O_RS1] = rs1;

	if ( imm )
	{
		np->n_operands.op_imm_used    = TRUE;
		np->n_operands.op_symp        = vp->v_symp;
		np->n_operands.op_addend      = vp->v_value;

		node_references_label(np, np->n_operands.op_symp);

		switch ( vp->v_relmode )
		{
		case VRM_LO10:	np->n_operands.op_reltype = REL_LO10;	break;
		case VRM_NONE:	np->n_operands.op_reltype = REL_13;	break;
		default:
			error(ERR_PCT_HILO, input_filename, input_line_no);
			free_node(np);
			np = NULL;
			break;
		}
	}
	else	np->n_operands.op_regs[O_RS2] = rs2;

	return np;
}


static Node *
form_rd_node(srcreg, destreg)
	register Regno srcreg, destreg;
{
	register Node *np;
	register struct opcode *opcp;

	destreg = chk_regno(destreg, RM_GP);

	switch ( srcreg )
	{
	case RN_Y:	opcp = opcode_ptr[OP_RDY];	break;
	case RN_PSR:	opcp = opcode_ptr[OP_RDPSR];	break;
	case RN_WIM:	opcp = opcode_ptr[OP_RDWIM];	break;
	case RN_TBR:	opcp = opcode_ptr[OP_RDTBR];	break;
	default:
		/* invalid register specified */
		error(ERR_INV_REG, input_filename, input_line_no);
		return (Node *)NULL;
	}

	np = new_node(input_line_no, opcp);
	np->n_operands.op_regs[O_RD] = destreg;

	return np;
}


Node *
eval_RR(opcp, reg1, reg2)
	register struct opcode *opcp;
	register Regno reg1, reg2;
{
	register Node *np;

	if (opcp->o_igroup & IG_MOV)
	{
		if ( ! IS_A_GP_REG(reg1) )
		{
			np = form_rd_node(reg1, reg2);
		}
		else if ( ! IS_A_GP_REG(reg2) )
		{
			np = form_wr_node(reg1, reg2, FALSE, RN_GP(0), NULL);
		}
		else
		{
			/*   MOV GPREG1, GPREG2		*/
			/* (actually "or %0,reg1,reg2") */
			np = eval_arithRRR(opcode_ptr[OP_OR],
						RN_GP(0), reg1, reg2);
		}
	}
	else if (opcp->o_igroup & IG_CMP)
	{
		/* (actually "subcc reg1,reg2,%0") */
		np = eval_arithRRR(opcode_ptr[OP_SUBCC], reg1, reg2, RN_GP(0));
	}
	else if (opcp->o_igroup & IG_NN)	/* NOT and NEG */
	{
		reg1 = chk_regno(reg1, RM_GP);
		reg2 = chk_regno(reg2, RM_GP);

		/* determine NOT vs. NEG by looking at the instr. subgroup */
		if (opcp->o_igroup & ISG_NOT)
		{
			/* "not" instruction */
			/* (actually "xnor reg1,%0,reg2") */
			np = eval_arithRRR(opcode_ptr[OP_XNOR],
						reg1, RN_GP(0), reg2);
		}
		else
		{
			/* "neg" instruction */
			/* (actually "sub %0,reg1,reg2") */
			np = eval_arithRRR(opcode_ptr[OP_SUB],
						RN_GP(0), reg1, reg2);
		}
	}
	else if (opcp->o_igroup & IG_FP)
	{
		reg1 = chk_regno(reg1, RM_FP);	/* rs1	*/
		reg2 = chk_regno(reg2, RM_FP);	/* rs2	*/

		if (opcp->o_igroup & ISG_FCMP)
		{
			/* instruction is an FCMP. */

#ifdef CHIPFABS
			Node *fcmp_np, *first_np;

			first_np = fcmp_np =
				form_fp_node(opcp, reg1, reg2, RN_NONE);

			if ( (iu_fabno == 2)  && (fpu_fabno == 1) &&
			     USE_BRUTE_FORCE_FAB_WORKAROUNDS
			   )
			{
				fcmp_np = form_fp_node(opcp,reg1,reg2,RN_NONE);
				/* this is an internally-generated node */
				fcmp_np->n_internal = TRUE;

				/* link the two nodes together. */
				first_np->n_next = fcmp_np;
			}

			/* For IU chip fab#1, insert 2 NOPs after every FCMP.
			 * This is normally done more optimally in
			 * perform_fab_workarounds() in a later pass.  But if
			 * optimization is NOT turned on, do it here instead.
			 */
			if ((iu_fabno == 1) && USE_BRUTE_FORCE_FAB_WORKAROUNDS)
			{
				append_a_NOP(fcmp_np);
				append_a_NOP(fcmp_np->n_next);
			}
			np = first_np;
#else /*CHIPFABS*/
			np = form_fp_node(opcp, reg1, reg2, RN_NONE);
#endif /*CHIPFABS*/
		}
		else
		{
			np = form_fp_node(opcp, RN_NONE, reg1, reg2);
#ifdef CHIPFABS
			/* For FPU chip fab#1, insert an FMOV after every 1164
			 * or 1165 F.P. operation.
			 */
			if ( (np != NULL) && (fpu_fabno == 1) &&
			     ( np->n_opcp->o_func.f_fp_1164 ||
			       np->n_opcp->o_func.f_fp_1165 )
			   )
			{
				append_an_FMOV(np, reg2);
			}
#endif /*CHIPFABS*/
		}
	}	
	else if (opcp->o_igroup & IG_RD)
	{
		np = form_rd_node(reg1, reg2);
	}	
	else if (opcp->o_igroup & IG_BIT)
	{
		register Regno rd;

		reg1 = chk_regno(reg1, RM_GP);
		reg2 = chk_regno(reg2, RM_GP);

		if ( (opcp->o_igroup & IG_SUBGRP_MASK) == ISG_BT )
		{
			/* this is BTST pseudo-instruction;
			 *	there is no destination register.
			 */
			rd = RN_GP(0);
		}
		else rd = reg2;	/* dest reg same as source reg*/

		/* BTST  (actually   "andcc reg2,reg1,%g0 ")	*/
		/* BSET  (actually   "or    reg2,reg1,reg2")	*/
		/* BCLR  (actually   "andn  reg2,reg1,reg2")	*/
		/* BTOG  (actually   "xor   reg2,reg1,reg2")	*/
		opcp = get_bit_opcp(opcp);
		np = eval_arithRRR(opcp, reg2, reg1, rd);
	}
#ifdef CHIPFABS
	else if ( (opcp->o_igroup & IG_JMPL) && (iu_fabno != 1) )
#else /*CHIPFABS*/
	else if (opcp->o_igroup & IG_JMPL)
#endif /*CHIPFABS*/
	{
		np = new_node(input_line_no, (reg2 == RN_GP(0))
						? opcode_ptr[OP_JMP]
						: opcode_ptr[OP_JMPL] );
		np->n_operands.op_regs[O_RS1] = chk_regno(reg1, RM_GP);
		np->n_operands.op_regs[O_RS2] = RN_GP(0);
		np->n_operands.op_regs[O_RD]  = chk_regno(reg2, RM_GP);
	}
	else if (opcp->o_igroup & IG_NOALIAS)
	{
		np = new_node(input_line_no, opcp);
		np->n_operands.op_regs[O_RS1] = chk_regno(reg1, RM_GP);
		np->n_operands.op_regs[O_RS2] = chk_regno(reg2, RM_GP);
	}
	else
	{
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}

	return np;
}


Node *
eval_j_RRE(opcp, reg1, reg2, vp)
	register struct opcode *opcp;	/* opcode */
	register Regno reg1, reg2;	/* 1st & 2nd operands */
	register struct value *vp;	/* 3rd operand(call arg count)*/
{
	register Node *np;

#ifdef CHIPFABS
	if ( ((opcp->o_igroup & IG_JMPL) == 0) || (iu_fabno == 1) )
#else /*CHIPFABS*/
	if ( (opcp->o_igroup & IG_JMPL) == 0 )
#endif /*CHIPFABS*/
	{
		/* This is not a JMPL opcode mnemonic. */
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}
	else if ( (reg2 == RN_GP(0)) && (vp != NULL) )
	{
		/* We have the illegal combination of RD==0 and a third
		 * argment to the JMPL mnemonic.  This makes no sense, since
		 * if RD==0, we can't be using it as an indirect-CALL
		 * instruction (which is the only context in which the 3rd
		 * argument makes sense).
		 */
		error(ERR_JMP_ARGC, input_filename, input_line_no);
		np = NULL;
	}
	else
	{
		np = eval_RR(opcp, reg1, reg2);

		/* Plus, fill in the arg-count field, if present. */
		if (vp != NULL)	fill_in_call_argcount_field(np, vp);
	}


	free_scratch_value(vp);

	return np;
}


Node *
eval_2opsR(opcp, reg1)
	register struct opcode *opcp;
	register Regno reg1;
{
	register Node *np;

	if (opcp->o_igroup & IG_NN)	/* NOT and NEG */
	{
		reg1 = chk_regno(reg1, RM_GP);

		/* determine NOT vs. NEG by looking at the instr. subgroup */
		if (opcp->o_igroup & ISG_NOT)
		{
			/* "not" instruction */
			/* (actually "xnor reg1,%0,reg1") */
			np = eval_arithRRR(opcode_ptr[OP_XNOR],
						reg1, RN_GP(0), reg1);
		}
		else
		{
			/* "neg" instruction */
			/* (actually "sub %0,reg1,reg1") */
			np = eval_arithRRR(opcode_ptr[OP_SUB],
						RN_GP(0), reg1, reg1);
		}
	}
	else if ( opcp->o_igroup & IG_ID/*inc/dec*/ )
	{
		static struct value vconst1 = { 1L, NULL,0,VRM_NONE,NULL,NULL};
		reg1  = chk_regno(reg1, RM_GP);

		/* INC   (actually   "add   reg1,1,reg1")	*/
		/* INCCC (actually   "addcc reg1,1,reg1")	*/
		/* DEC   (actually   "sub   reg1,1,reg1")	*/
		/* DECCC (actually   "subcc reg1,1,reg1")	*/
		np = form_arithRCR(get_incdec_opcp(opcp), reg1, &vconst1, reg1);
	}
	else
	{
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}

	return np;
}


Node *
eval_2opsRE(opcp, reg1, vp)
	register struct opcode *opcp;
	register Regno reg1;
	register struct value *vp;
{
	register Node *np;

	if (opcp->o_igroup & IG_CMP)
	{
		/* (actually "subcc  reg1,const13,%0") */
		np = form_arithRCR(opcode_ptr[OP_SUBCC], reg1, vp, RN_GP(0));
	}
	else
	{
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}

	/* In any case, the scratch value structure has served its purpose,
	 * and is no longer needed.
	 */
	free_scratch_value(vp);

	return np;
}	


Node *
eval_1opsNOARG(opcp)
	register struct opcode *opcp;
{
	register Node *np;

	if (opcp->o_igroup & IG_UNIMP)
	{
		/* (actually "unimp  0") */

		register struct value *vp;
		
		/* This will get a Value which is the constant 0. */
		vp = get_scratch_value(NULL, 0);

		np = form_1opsE(opcp, vp);

		/* The scratch value structure has served its purpose,
		 * and is no longer needed.
		 */
		free_scratch_value(vp);
	}
	else
	{
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}

	return np;
}	


Node *
eval_1opsA(opcp, operp)
	register struct opcode   *opcp;
	register struct operands *operp;
{
	register Node *np;

	if (opcp->o_igroup & (IG_JRI|IG_TICC))
	{
		np = new_node(input_line_no, opcp);
		np->n_operands = *operp;

		/* ...and if there was a symbol */
		node_references_label(np, np->n_operands.op_symp);
	}
#ifdef CHIPFABS
	else if ( (opcp->o_igroup & IG_CALL) && (iu_fabno != 1) )
#else /*CHIPFABS*/
	else if (opcp->o_igroup & IG_CALL)
#endif /*CHIPFABS*/
	{
		/* CALL (actually "jmpl	   %reg+val,%15")	*/
		/*             or "jmpl	   %reg1+%reg2,%15")	*/
		np = new_node(input_line_no, opcode_ptr[OP_JMPL]);
		np->n_operands = *operp;
		np->n_operands.op_regs[O_RD] = RN_GP(15);

		/* ...and if there was a symbol */
		node_references_label(np, np->n_operands.op_symp);
	}
	else
	{
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}

	free_operands(operp);

	return np;
}


Node *
eval_call_AE(opcp, operp, vp)
	register struct opcode   *opcp;
	register struct operands *operp;
	register struct value *vp;
{
	register Node *np;

	if ( opcp->o_igroup & IG_CALL )
	{
		/* Have a "call <reg_addr>,<count-of-args-in-out-regs>", which
		 * is the same as "call <reg_addr>" with one extra field filled
		 * in, in the operands structure.
		 */

		/* Form a regular JMPL node, just as if we had
		 * "call <reg_addr>".
		 */
		np = eval_1opsA(opcp, operp);

		/* Now, fill in the extra field. */
		fill_in_call_argcount_field(np, vp);
	}
	else
	{
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}

	/* In any case, the scratch value structure has served its purpose,
	 * and is no longer needed.
	 */
	free_scratch_value(vp);

	return np;
}


static void
fill_in_call_argcount_field(np, vp)
	register Node *np;
	register struct value *vp;
{
	if ( (np != NULL) && (vp != NULL) )
	{
		/* Here, we both:
		 *	-- have a value to use (vp)
		 *      -- have a valid node (np) into which to put the value.
		 */
		chk_absolute(vp);
		if ( (vp->v_value < 0) || (vp->v_value > 6) )
		{
			error(ERR_OUT_REG_CNT, input_filename, input_line_no,
				vp->v_value);
			vp->v_value = 6;	/* the most conservative value*/
		}
		np->n_operands.op_call_oreg_count_used = TRUE;
		np->n_operands.op_call_oreg_count      = vp->v_value;
	}
}


Node *
eval_j_ARE(opcp, operp, regno, vp)
	register struct opcode   *opcp;		/* The opcode.		      */
	register struct operands *operp;	/* 1st operand (RS1&RS2/imm)  */
	register Regno  regno;			/* 2nd operand (RD)	      */
	register struct value *vp;		/* 3rd operand(call arg count)*/
{
	register Node *np;

#ifdef CHIPFABS
	if ( ((opcp->o_igroup & IG_JMPL) == 0) || (iu_fabno == 1) )
#else /*CHIPFABS*/
	if ( (opcp->o_igroup & IG_JMPL) == 0 )
#endif /*CHIPFABS*/
	{
		/* This is not a JMPL opcode mnemonic. */
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}
	else if ( (regno == RN_GP(0)) && (vp != NULL) )
	{
		/* We have the illegal combination of RD==0 and a third
		 * argment to the JMPL mnemonic.  This makes no sense, since
		 * if RD==0, we can't be using it as an indirect-CALL
		 * instruction (which is the only context in which the 3rd
		 * argument makes sense).
		 */
		error(ERR_JMP_ARGC, input_filename, input_line_no);
		np = NULL;
	}
	else
	{
		np = new_node(input_line_no, (regno == RN_GP(0))
						? opcode_ptr[OP_JMP]
						: opcode_ptr[OP_JMPL] );
		np->n_operands = *operp;
		np->n_operands.op_regs[O_RD] = chk_regno(regno, RM_GP);

		/* ...and in case there was a symbol */
		node_references_label(np, np->n_operands.op_symp);

		/* Plus, fill in the arg-count field, if present. */
		if (vp != NULL)	fill_in_call_argcount_field(np, vp);
	}

	free_operands(operp);
	free_scratch_value(vp);

	return np;
}


/* 1 operand: a Register. */
Node *
eval_1opsR(opcp, regno1)
	register struct opcode *opcp;
	register Regno regno1;
{
	register Node *np;

	if ( opcp->o_igroup & (IG_TST|IG_JRI|IG_TICC|IG_CLR|IG_CALL) )
	{
		regno1  = chk_regno(regno1, RM_GP);
	}

	if (opcp->o_igroup & IG_CLR)
	{
		switch ( opcp->o_igroup & IG_SUBGRP_MASK )
		{
		case ISG_CLR:
			/* OK;  opcode mnemonic was "clr" */
			/* CLR  (actually  "or    %0,%0,regno1")*/
			np = eval_arithRRR(opcode_ptr[OP_OR],
					RN_GP(0), RN_GP(0), regno1);
			break;

		default:
			/* "clrb" and "clrh" are invalid here */
			error(ERR_CLR, input_filename, input_line_no);
			np = NULL;
			break;
		}
	}
	else if (opcp->o_igroup & IG_TST)
	{
		/* TST  (actually  "orcc  %0,regno1,%0")	*/
		np = eval_arithRRR(opcode_ptr[OP_ORCC],
					RN_GP(0), regno1, RN_GP(0));
	}
#ifdef CHIPFABS
	else if ( (opcp->o_igroup & IG_CALL) && (iu_fabno != 1) )
#else /*CHIPFABS*/
	else if (opcp->o_igroup & IG_CALL)
#endif /*CHIPFABS*/
	{
		/* CALL regno1
		 *	(actually "jmpl	   regno1+%0,%15")
		 */
		np = new_node(input_line_no, opcode_ptr[OP_JMPL]);
		np->n_operands.op_regs[O_RS1] = regno1;
		np->n_operands.op_regs[O_RS2] = RN_GP(0);
		np->n_operands.op_regs[O_RD]  = RN_GP(15);
	}
	else if ( opcp->o_igroup & (IG_JRI|IG_TICC) )
	{
		/* JMP  (actually  "jmp    regno1+%0")	*/
		/* RETT (actually  "rett   regno1+%0")	*/
		/* IFLUSH(actually "iflush regno1+%0")	*/
		/* TICC (actually  "Ticc   regno1+%0")	*/
		np = new_node(input_line_no, opcp);
		np->n_operands.op_regs[O_RS1] = regno1;
		np->n_operands.op_regs[O_RS2] = RN_GP(0);
		if ( opcp->o_func.f_group == FG_JMP )
		{
			np->n_operands.op_regs[O_RD]  = RN_GP(0);
		}
	}
	else
	{
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}

	return np;
}


/* 2 operands: a Register and an Expression. */
Node *
eval_call_RE(opcp, regno, vp)
	register struct opcode *opcp;
	register Regno regno;
	register struct value *vp;
{
	register Node *np;

	if ( opcp->o_igroup & IG_CALL )
	{
		/* Have a "call <reg>,<count-of-args-in-out-regs>", which
		 * is the same as "call <reg>" with one extra field filled
		 * in, in the operands structure.
		 */

		/* Form a regular JMPL node, just as if we had
		 * "call <reg>".
		 */
		np = eval_1opsR(opcp, regno);

		/* Now, fill in the extra field. */
		fill_in_call_argcount_field(np, vp);
	}
	else
	{
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}

	/* In any case, the scratch value structure has served its purpose,
	 * and is no longer needed.
	 */
	free_scratch_value(vp);

	return np;
}


/* 1 operand: an Expression. */
Node *
eval_1opsE(opcp, vp)
	register struct opcode *opcp;
	register struct value *vp;
{
	register Node *np;

	np = form_1opsE(opcp, vp);

	/* In any case, the scratch value structure has served its purpose,
	 * and is no longer needed.
	 */
	free_scratch_value(vp);

	return np;
}


static Node *
form_1opsE(opcp, vp)
	register struct opcode *opcp;
	register struct value *vp;
{
	register Node *np;

#ifdef CHIPFABS	/* Coprocessor Branches not implemented in fab-1 IU chip */
	if ( (iu_fabno == 1) &&
	     BIT_IS_ON(opcp->o_igroup, IG_BR) && (opcp->o_func.f_br_cc == CC_C)
	   )
	{
		/* Coprocessor Branches not implemented on 1st-fab IU */
		error(ERR_UNK_OPC, input_filename, input_line_no, opcp->o_name);
		np = NULL;
	}
	else if ( BIT_IS_ON(opcp->o_igroup, (IG_BR|IG_CALL)) )
#else /*CHIPFABS*/
	if ( BIT_IS_ON(opcp->o_igroup, (IG_BR|IG_CALL)) )
#endif /*CHIPFABS*/
	{
		if (vp->v_relmode == VRM_NONE)
		{	
			np = new_node(input_line_no, opcp);
			np->n_operands.op_symp     = vp->v_symp;
			np->n_operands.op_addend   = vp->v_value;
			np->n_operands.op_imm_used = TRUE;

			switch ( opcp->o_func.f_group )
			{
			case FG_BR:
				np->n_operands.op_reltype = REL_DISP22;
				break;
			case FG_CALL:
				np->n_operands.op_reltype = REL_DISP30;
				break;
#ifdef DEBUG
			default:  internal_error("form_1opsE(): f_group=%d", (int)opcp->o_func.f_group);
#endif
			}
#ifndef ALWAYS_BUILD_NODE_LIST
			if ( BIT_IS_ON(opcp->o_igroup, IG_BR) &&
			     (opcp->o_func.f_br_cc == CC_F) &&
			     (assembly_mode == ASSY_MODE_QUICK)
			   )
			{
				/* Insert a NOP before every Bfcc, to make sure
				 * that this code is architecturally correct.
				 * This is normally done more optimally in
				 * check_list_for_errors() in a later pass.
				 *
				 * But if a list is NOT being built, do it
				 * here instead.
				 */
				np = prepend_a_NOP(np);
			}
#endif /*ALWAYS_BUILD_NODE_LIST*/
		}
		else
		{
			error(ERR_PCT_HILO, input_filename, input_line_no);
			np = NULL;
		}
	}
	else if ( BIT_IS_ON(opcp->o_igroup, (IG_JRI|IG_TICC)) )
	{
		/* TICC   (actually  Ticc   %0+expr)	*/
		/* JMP    (actually  jmpl   %0+expr,%0)	*/
		/* RETT   (actually  rett   %0+expr)	*/
		/* IFLUSH (actually  iflush %0+expr)	*/
		np = new_node(input_line_no, opcp);
		np->n_operands.op_regs[O_RS1] = RN_GP(0);
		np->n_operands.op_imm_used    = TRUE;
		np->n_operands.op_symp        = vp->v_symp;
		np->n_operands.op_addend      = vp->v_value;
		if ( opcp->o_func.f_group == FG_JMP )
		{
			np->n_operands.op_regs[O_RD]  = RN_GP(0);
		}

		if ( !VALUE_IS_ABSOLUTE(vp) )
		{
			warning(WARN_QUES_RELOC, input_filename, input_line_no);
		}

		switch (vp->v_relmode)
		{
		case VRM_NONE:	np->n_operands.op_reltype = REL_13;	break;
		case VRM_LO10:	np->n_operands.op_reltype = REL_LO10;	break;
		default:
			error(ERR_PCT_HILO, input_filename, input_line_no);
			free_node(np);
			np = NULL;
			break;
		}
	}
	else if ( BIT_IS_ON(opcp->o_igroup, IG_UNIMP) )
	{
		/* The UNIMP instruction. */

		np = new_node(input_line_no, opcp);
		np->n_operands.op_imm_used    = TRUE;
		np->n_operands.op_symp        = vp->v_symp;
		np->n_operands.op_addend      = vp->v_value;

		switch (vp->v_relmode)
		{
		case VRM_NONE:	np->n_operands.op_reltype = REL_22;	break;
		case VRM_HI22:	np->n_operands.op_reltype = REL_HI22;	break;
		case VRM_LO10:	np->n_operands.op_reltype = REL_LO10;	break;
		default:
			error(ERR_PCT_HILO, input_filename, input_line_no);
			free_node(np);
			np = NULL;
			break;
		}
	}
	else
	{
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}

	/* ...and in case there was a symbol */
#ifdef ALWAYS_BUILD_NODE_LIST
	if (np != NULL)	node_references_label(np, np->n_operands.op_symp);
#else /*ALWAYS_BUILD_NODE_LIST*/
	{
		register Node *p;
		for (p = np;   p != NULL;   p = p->n_next)
		{
			node_references_label(p, p->n_operands.op_symp);
		}
	}
#endif /*ALWAYS_BUILD_NODE_LIST*/

	return np;
}


Node *
eval_clr(opcp, operp)	/* "clr" */
	register struct opcode *opcp;
	register struct operands *operp;
{
	register Node *np;

	if (opcp->o_igroup & IG_CLR)
	{
		/* CLR (actually  "st/stb/sth %0,[xxxx]")	*/

		/* switch over to the opcode we really want */
		switch ( opcp->o_igroup & IG_SUBGRP_MASK )
		{
		case ISG_CLR:	opcp = opcode_ptr[OP_ST];	break;
		case ISG_CLRH:	opcp = opcode_ptr[OP_STH];	break;
		case ISG_CLRB:	opcp = opcode_ptr[OP_STB];	break;
#ifdef DEBUG
		default:
			internal_error("eval_clr(): subgrp = %d",
				(int)(opcp->o_igroup & IG_SUBGRP_MASK));
#endif
		}

		np = eval_ldst(opcp, operp, RN_GP(0));
	}
	else
	{
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}

	return np;
}


Node *
eval_sfa_clr(opcp, vp)		/* "clr" */
	register struct opcode *opcp;
	register struct value *vp;
{
	register Node *np;

	if (opcp->o_igroup & IG_CLR)
	{
		/* CLR (actually  "st/stb/sth %0,[%ad xxxx]")	*/
		np = form_sfa_ldst(opcp, vp, RN_GP(0));
	}
	else
	{
		error(ERR_SYNTAX, input_filename, input_line_no);
		np = NULL;
	}

	/* In any case, the scratch value structure has served its purpose,
	 * and is no longer needed.
	 */
	free_scratch_value(vp);

	return np;
}


static Node *
eval_byte_half_word(opcp, vlhp, default_reloc, alignment)
	register struct opcode  *opcp;
	register struct val_list_head *vlhp;
	register RELTYPE default_reloc;
	register int     alignment;
{
	register struct value *vp;
	register Node *np;
	register Node *first_np;
	register Node *prev_np;

	if (vlhp->vlh_listlen == 0)
	{
		/* This syntax is no longer supported.
		 * Its function has been subsumed by the ".align" pseudo.
		 */
		error(ERR_SYNTAX, input_filename, input_line_no);
		first_np = NULL;
	}
	else
	{
		first_np = NULL;
		for ( vp = vlhp->vlh_head, prev_np = NULL;    vp != NULL;
		      vp = vp->v_next, prev_np = np
		    )
		{
			np = new_node(input_line_no, opcp);
			np->n_operands.op_addend = vp->v_value;
			np->n_operands.op_symp   = vp->v_symp;

			node_references_label(np, np->n_operands.op_symp);

			switch (vp->v_relmode)
			{
			case VRM_NONE:
				np->n_operands.op_reltype = default_reloc;
				break;

			case VRM_LO10:
				switch (opcp->o_func.f_subgroup)
				{
				case FSG_BYTE:
				case FSG_HALF:
					error(ERR_PCT_HILO, input_filename,
						input_line_no);
					free_node(np);
					np = NULL;
					break;
				/***case FSG_HALF:
				 *** We'd like to be able to do %lo for a
				 *** .half, but since LO10 is really a Word
				 *** relocation, the linker might blow up on an
				 *** alignment error if it tried to apply it.
				 *** We'd really need a REL_LO10_HALF to do it
				 *** right...
				 *** So, make FSG_HALF an error, above.
				 ***/
				case FSG_WORD:
					np->n_operands.op_reltype = REL_LO10;
					break;
				}
				break;

			case VRM_HI22:
				switch (opcp->o_func.f_subgroup)
				{
				case FSG_BYTE:
				case FSG_HALF:
					error(ERR_PCT_HILO, input_filename,
						input_line_no);
					free_node(np);
					np = NULL;
					break;
				case FSG_WORD:
					np->n_operands.op_reltype = REL_HI22;
					break;
				}
				break;
#ifdef DEBUG
			default:
				internal_error("eval_byte_half_word(): relmode = %d?", vp->v_relmode);
#endif
			}

			if (prev_np == NULL)
			{
				first_np = np;
				if (first_np == NULL)	break;
			}
			else	prev_np->n_next = np;
		}
	}

	return first_np;
}


static Node *
eval_string_arg_pseudo(opcp, string)
	register struct opcode *opcp;
	register char *string;
{
	register Node *np;

	np = new_node(input_line_no, opcp);
	np->n_string = (char *)as_malloc(strlen(string)+1);
	strcpy(np->n_string, string);

	return np;
}


static Node *
eval_single_int_arg_pseudo(opcp, argcount, vp)
	register struct opcode *opcp;
	register long int argcount;
	register struct value  *vp;
{
	/* For pseudo's requiring single, integer arguments. */

	register Node *np;

	if (argcount != 1)
	{
		/* There was more than one argument, which there shouldn't be.*/
		error(ERR_OP_COUNT, input_filename, input_line_no);
		np = NULL;
	}
	else if ( VALUE_IS_QUOTED_STRING(vp) )
	{
		/* Operand was a quoted string, which it shouldn't be. */
		error(ERR_VAL_INVALID, input_filename, input_line_no,
			vp->v_strptr);
		np = NULL;
	}
	else	/* All looks OK. */
	{
		np = new_node(input_line_no, opcp);
		np->n_operands.op_addend = vp->v_value;
	}

	return np;
}


static Node *
eval_align(opcp, argcount, vlistp)
	register struct opcode *opcp;
	register int argcount;
	register struct value *vlistp;
{
	register Node *np;

	np = eval_single_int_arg_pseudo(opcp, argcount, vlistp);
	if (np != NULL)
	{
		/* Check for value errors in the argument. */
		switch (np->n_operands.op_addend)
		{
		case 1:
			/* No need to do anything; we ALWAYS have Byte
			 * alignment.
			 */
			free_node(np);
			np = NULL;
			break;
		case 2:
		case 4:
		case 8:
			/* Use the already-created "align" node. */
			break;
		default:
			error(ERR_ALIGN_BDY, input_filename,
				input_line_no);
			free_node(np);
			np = NULL;
		}
	}

	return np;
}


static Node *
eval_reserve(vlhp)
	register struct val_list_head *vlhp;
{
	register Node *n1p, *n2p, *n3p;
	register struct value *v1p, *v2p;
	register Node *skip_np, *label_np;
	register char *orig_segname;
	register struct value *v3p;
	register Node *n4p;
	register struct value *v4p;
	register Node *n5p;

	/* eval_reserve() changes the ".reserve" pseudo-op into a LABEL plus
	 * a ".skip" pseudo.
	 *
	 * And, if it specifies a segment other than the current one (via the
	 * optional 3rd argument), it adds ".seg" nodes before and after the
	 * LABEL-plus-SKIP node pair.
	 */

        if ( (vlhp->vlh_listlen < 2) ||
		/* expect symbol name as first arg */
	     ( (v1p = vlhp->vlh_head),
		((v1p->v_symp == NULL) || (v1p->v_value != 0))
	     ) ||
		/* expect NON-symbol, NON-string as second arg */
	     ( (v2p = v1p->v_next),
                ((v2p->v_symp != NULL) || VALUE_IS_QUOTED_STRING(v2p))
	     ) ||
		/* if present, 3rd arg must be a string (seg name) */
	     ( (vlhp->vlh_listlen >= 3) &&
	       ( (v3p = v2p->v_next), (!VALUE_IS_QUOTED_STRING(v3p)) )
	     ) ||
		/* if present, 4th arg must be a NON-string (i.e. number) */
	     ( (vlhp->vlh_listlen >= 4) &&
	       ( (v4p = v3p->v_next),
		 ( (v4p->v_symp != NULL) || VALUE_IS_QUOTED_STRING(v4p) )
	       )
	     )
           )
        {   
                /* 1st operand was a constant or expression, not just a symbol,
                 * or 2nd operand was not just a constant (or constant
                 *      expression) or was a string,
		 * or 3rd operand was present and was not a string,
		 * or 4th operand was present and was not just a constant (or
		 *	constant expression) or was a string.
                 */
		error(ERR_SYNTAX, input_filename, input_line_no);
		return (Node *)NULL;
	}

	chk_absolute(v2p);	/* 2nd operand (size) must be absolute # */
	/* all variations of ".reserve" need these two nodes: */
	label_np = eval_label(v1p->v_symp->s_symname);
	skip_np = eval_single_int_arg_pseudo(opcode_ptr[OP_PSEUDO_SKIP], 1,v2p);

	switch ( vlhp->vlh_listlen )
	{
	case 2:	/* 2 args in list: symbol,size		*/
		/* just link them together, for add_nodes() */
		/* Leave list: LABEL, SKIP. */
		n1p = label_np;
		n2p = skip_np;
		n1p->n_next = n2p;
		break;

	case 3:	/* 3 args in list: symbol,size,"segment_name"	*/
		/* Leave list: SEG, LABEL, SKIP, SEG. */
		orig_segname = get_dot_seg_name(); /*the segment to return to.*/
		n1p = eval_string_arg_pseudo(opcode_ptr[OP_PSEUDO_SEG],
						v3p->v_strptr);
		n2p = label_np;	/* label */
		n3p = skip_np;	/* .skip */
		n4p = eval_string_arg_pseudo(opcode_ptr[OP_PSEUDO_SEG],
						orig_segname);
		n1p->n_next = n2p;
		n2p->n_next = n3p;
		n3p->n_next = n4p;
		break;

	case 4:	/* 4 args in list: symbol,size,"segment_name",alignment	*/
		/* Leave list: SEG, ALIGN, LABEL, SKIP, SEG. */
		orig_segname = get_dot_seg_name(); /*the segment to return to.*/
		n1p = eval_string_arg_pseudo(opcode_ptr[OP_PSEUDO_SEG],
						v3p->v_strptr);
		n2p = eval_align(opcode_ptr[OP_PSEUDO_ALIGN], 1, v4p);
		n3p = label_np;	/* label */
		n4p = skip_np;	/* .skip */
		n5p = eval_string_arg_pseudo(opcode_ptr[OP_PSEUDO_SEG],
						orig_segname);
		if (n2p == NULL)
		{
			/* No "align" node was necessary, so none was
			 * generated.
			 */
			n1p->n_next = n3p;
		}
		else
		{
			n1p->n_next = n2p;
			n2p->n_next = n3p;
		}
		n3p->n_next = n4p;
		n4p->n_next = n5p;
		break;

	default:
		error(ERR_OP_COUNT, input_filename, input_line_no);
		free_node(skip_np);
		free_node(label_np);
		n1p = NULL;
		break;
	}

	return n1p;
}


Node *
eval_pseudo(opcp, vlhp)
	register struct opcode  *opcp;
	register struct val_list_head *vlhp;
{
	register struct value  *vp;
	register Node          *np;
	enum { MAKE_STANDARD_NODE, NULL_NP_AND_FREE_LIST,
		FREE_LIST_BUT_KEEP_NP, DO_NOTHING } to_do;

	switch (opcp->o_func.f_subgroup)
	{
	/*-------------------------------------------------------------------
	 * The cases for ALIGN, SKIP, PROC, BYTE, HALF, and WORD leave their
	 * arguments in np->n_operands instead of np->n_vlhp.
	 *
	 * Why?
	 *
	 * For ALIGN, SKIP, and PROC because it is simpler to not carry around
	 * a value list when they only have a single integer argument.
	 *
	 * For the other three, because we need the symbol references in
	 * np->n_operands in order to do optimizations easily & correctly,
	 * so must create a separate node for each argument.
	 *-------------------------------------------------------------------
	 */
	case FSG_ALIGN:
		np = eval_align(opcp, vlhp->vlh_listlen, vlhp->vlh_head);
		if (np == NULL)
		{
			to_do = NULL_NP_AND_FREE_LIST;	/* had error */
		}
		else	to_do = FREE_LIST_BUT_KEEP_NP;
		break;

	case FSG_SKIP:	/* ".skip" pseudo-op */
	case FSG_PROC:	/* ".proc" pseudo-op */
		np = eval_single_int_arg_pseudo(opcp, vlhp->vlh_listlen,
							vlhp->vlh_head);
		if (np == NULL)	to_do = NULL_NP_AND_FREE_LIST;  /* had error */
		else		to_do = FREE_LIST_BUT_KEEP_NP;
		break;

	case FSG_BYTE:
		np = eval_byte_half_word(opcp, vlhp, REL_8, 1);
		to_do = FREE_LIST_BUT_KEEP_NP;	/* ...we've done the rest here*/
		break;

	case FSG_HALF:
		np = eval_byte_half_word(opcp, vlhp, REL_16, 2);
		to_do = FREE_LIST_BUT_KEEP_NP;	/* ...we've done the rest here*/
		break;

	case FSG_WORD:
		np = eval_byte_half_word(opcp, vlhp, REL_32, 4);
		to_do = FREE_LIST_BUT_KEEP_NP;	/* ...we've done the rest here*/
		break;

	/*-------------------------------------------------------------------
	 * These pseudos leave their arg in np->n_string.
	 *-------------------------------------------------------------------
	 */

	case FSG_SEG:	/* ".seg" pseudo-op */
		/* This Pseudo-op is handled in add_nodes(), and will probably
		 * not be left on the node-list.
		 */
		to_do = FREE_LIST_BUT_KEEP_NP;

		if (vlhp->vlh_listlen == 1)
		{
			/* There is one operand, as is normally expected. */

			vp  = vlhp->vlh_head;		/* 1st operand */

			if ( VALUE_IS_QUOTED_STRING(vp) )
			{
				np = eval_string_arg_pseudo(opcp/*.seg*/,
								vp->v_strptr);
			}
			else
			{
				error(ERR_QSTRING, input_filename,
					input_line_no);
				to_do = NULL_NP_AND_FREE_LIST;
			}
		}
		else
		{
			/* Hmmm... if there are no operands, maybe it's one
			 * of the 680x0-compatible pseudos:
			 * ".text", ".data", or ".bss".
			 */
			if ( (vlhp->vlh_listlen == 0) &&
			     ( (strcmp(opcp->o_name, ".text") == 0) ||
			       (strcmp(opcp->o_name, ".data") == 0) ||
			       (strcmp(opcp->o_name, ".bss" ) == 0)
			     )
			   )
			{
				/* "(opcp->o_name)+1" reliably points to one
				 * of "text", "data", or "bss".
				 */
				np = eval_string_arg_pseudo(
						opcode_ptr[OP_PSEUDO_SEG],
					       (opcp->o_name)+1 );
			}
			else
			{
				error(ERR_OP_COUNT, input_filename,
					input_line_no);
				to_do = NULL_NP_AND_FREE_LIST;
			}
		}
		break;

	case FSG_OPTIM:	/* ".optim" pseudo-op */
		/* This Pseudo-op sets temporary per-procedure optmization
		 * options (if optimization is enabled).
		 */
		to_do = FREE_LIST_BUT_KEEP_NP;

		if (vlhp->vlh_listlen == 1)
		{
			/* There is one operand, as is expected. */

			vp  = vlhp->vlh_head;		/* 1st operand */

			if ( VALUE_IS_QUOTED_STRING(vp) )
			{
				np = eval_string_arg_pseudo(opcp/*.optim*/,
								vp->v_strptr);
			}
			else
			{
				error(ERR_QSTRING, input_filename,
					input_line_no);
				to_do = NULL_NP_AND_FREE_LIST;
			}
		}
		else
		{
			error(ERR_OP_COUNT, input_filename, input_line_no);
			to_do = NULL_NP_AND_FREE_LIST;
		}
		break;

        case FSG_IDENT: /* ".ident" pseudo-op */
                /* This Pseudo-op gives a string to stash in the .ident section.
                 * Stupid, but true.
                 */
                to_do = FREE_LIST_BUT_KEEP_NP;

                if (vlhp->vlh_listlen == 1)
                {
                        /* There is one operand, as is expected. */

                        vp  = vlhp->vlh_head;           /* 1st operand */

                        if ( VALUE_IS_QUOTED_STRING(vp) )
                        {
                                np = eval_string_arg_pseudo(opcp/*.ident*/,
                                                                vp->v_strptr);
                        }
                        else
                        {   
                                error(ERR_QSTRING, input_filename,
                                        input_line_no);
                                to_do = NULL_NP_AND_FREE_LIST;
                        }
                }
                else
                {
                        error(ERR_OP_COUNT, input_filename, input_line_no);
                        to_do = NULL_NP_AND_FREE_LIST;
                }
                break;

	/*-------------------------------------------------------------------
	 * These pseudos have no args to leave anywhere.
	 *-------------------------------------------------------------------
	 */

	case FSG_EMPTY:
	case FSG_ALIAS:
		np = new_node(input_line_no, opcp);
		to_do = FREE_LIST_BUT_KEEP_NP;
		break;

	/*-------------------------------------------------------------------
	 * The rest of these pseudos leave their args in np->n_vlhp.
	 *-------------------------------------------------------------------
	 */

	case FSG_SGL:
	case FSG_DBL:
	case FSG_QUAD:
		if (vlhp->vlh_listlen == 0)
		{
			/* This syntax is no longer supported.  Its function
			 * has been subsumed by the ".align" pseudo.
			 */
			error(ERR_SYNTAX, input_filename, input_line_no);
			to_do = NULL_NP_AND_FREE_LIST;
		}
		else	to_do = MAKE_STANDARD_NODE;
		break;

	case FSG_ASCII:
	case FSG_ASCIZ:
		to_do = MAKE_STANDARD_NODE;
		for (vp = vlhp->vlh_head;   vp != NULL;   vp = vp->v_next)
		{
			/* should be an Ascii string argument */
			if ( !VALUE_IS_QUOTED_STRING(vp) )
			{
				error(ERR_QSTRING,input_filename,input_line_no);
				to_do = NULL_NP_AND_FREE_LIST;
				break;	/* got an error; no need to continue */
			}
		}
		break;

	case FSG_GLOBAL:
		if (vlhp->vlh_listlen == 0)
		{
			error(ERR_SYNTAX, input_filename, input_line_no);
		}

		for (vp = vlhp->vlh_head;   vp != NULL;   vp = vp->v_next)
		{
			if ( (vp->v_symp == NULL) || (vp->v_value != 0) )
			{
				/* We have an invalid operand for ".global".
				 * Explanation follows:
				 *
				 * If vp->v_symp == NULL, then "vp" refers to
				 * some absolute value;  the operand was:
				 *	- a string
				 *	- a numeric constant
				 *   or - an absolute symbol (i.e. defined with
				 *		"="), which is a constant.
				 *
				 * If vp->v_symp != NULL, but vp->v_value != 0,
				 * then the operand is a symbol plus a (non-0)
				 * offset (e.g. "sym+5"), which is meaningless
				 * as an operand for the "global" pseudo.
				 *
				 * Either way, we have an "invalid operand".
				 */
				error(ERR_INV_OP,input_filename,input_line_no);
			}
			else
			{
				if (BIT_IS_ON(vp->v_symp->s_attr, SA_LOCAL) )
				{
					error(ERR_GBL_LOC, input_filename,
						input_line_no);
				}
				else if (BIT_IS_OFF(vp->v_symp->s_attr,
						    SA_GLOBAL) )
				{
					if (BIT_IS_ON(vp->v_symp->s_attr,
						      SA_DEFN_SEEN) )
					{
						error(ERR_GBL_DEF,
							input_filename,
							input_line_no,
							vp->v_symp->s_symname);
					}
					else	SET_BIT(vp->v_symp->s_attr,
							SA_GLOBAL);
				}
			}
		}

		/* Don't bother to make a node.  Disassembly will automatically
		 * generate a ".global" statement along with its defining label.
		 */
		to_do = NULL_NP_AND_FREE_LIST;
		break;

	case FSG_COMMON:
		to_do = NULL_NP_AND_FREE_LIST;	/* the default case... */

		switch ( vlhp->vlh_listlen )
		{
		case 2:	/* 2 args in list */
			ps_common(vlhp->vlh_head,	 /*1st operand*/
				  vlhp->vlh_head->v_next,/*2nd operand*/
				  "bss");
			/* Don't bother to make a node, unless we are
			 * going to need it for disassembly later.
			 */
			if (do_disassembly) to_do = MAKE_STANDARD_NODE;
			break;
		case 3:	/* 3 args in list */
			{
				register struct value *v3p;

				v3p = vlhp->vlh_head->v_next->v_next;
				if ( VALUE_IS_QUOTED_STRING(v3p) )
				{
					ps_common(vlhp->vlh_head,
						  vlhp->vlh_head->v_next,
						  v3p->v_strptr);
					/* Don't bother to make a node, unless
					 * we are going to need it for
					 * disassembly later.
					 */
					if (do_disassembly)
					{
						to_do = MAKE_STANDARD_NODE;
					}
				}
				else
				{
					/* 3rd operand was a number or symbol,
					 * not a quoted string.
					 */
					error( ERR_QSTRING, input_filename,
					       input_line_no);
				}
			}
			break;
		default:
			error(ERR_OP_COUNT, input_filename, input_line_no);
			break;
		}
		break;

	case FSG_RESERVE:
		np = eval_reserve(vlhp);
		to_do = FREE_LIST_BUT_KEEP_NP;	/*...we've done the rest here.*/
		break;

	case FSG_STAB:	/* stabd, stabn, stabs */
		if ( vlhp->vlh_listlen == ((int)(opcp->o_igroup/*3,4,or5*/)) )
		{
			if (optimization_level == 0)
			{
				if ( ((int)(opcp->o_igroup)) == 3/*STABD*/ )
				{
					/* STABD's must be put on this list,
					 * since it needs the value of "."
					 * which we get at emit time.
					 */
					to_do = MAKE_STANDARD_NODE;
				}
				else
				{
				/* Create the STAB node, and put it on a
				 * special list of nodes to be emitted after
				 * all code is emitted.
				 */
					np = new_node(input_line_no, opcp);
					np->n_vlhp = vlhp;
					add_stab_node_to_list(np);
					np = NULL;
					to_do = DO_NOTHING;
				}
			}
			else
			{
				/* We actually throw away STAB{N,D} as the easy
				 * way to fix a bug (1011656).  We pick the easy
				 * way since STABS will go away soon.
				 * Stabd's would have to be thrown away anyway since
				 * optimization would scramble them.
				 */
				static Bool stab_warn = FALSE;
				if (!stab_warn &&
				    ((int)(opcp->o_igroup)) != 5) /* ".stabs" are ok */
				{
				        warning(WARN_STABS_OPT, input_filename, input_line_no);
					stab_warn = TRUE;
					to_do = NULL_NP_AND_FREE_LIST;
				}
				else if (((int)(opcp->o_igroup)) == 5)
				{
                            	/* Create the STAB node, and put it on a
                                 * special list of nodes to be emitted after
                                 * all code is emitted.
                                 */
                                        np = new_node(input_line_no, opcp);
                                        np->n_vlhp = vlhp;
                                        add_stab_node_to_list(np);
                                        np = NULL;
                                        to_do = DO_NOTHING;
				}
				else /* ".stabn" or ".stabd" but already warned */	
				{
                                        to_do = NULL_NP_AND_FREE_LIST;
                                }
			}
		}
		else
		{
			error(ERR_OP_COUNT, input_filename, input_line_no);
			to_do = NULL_NP_AND_FREE_LIST;
		}

		break;

	default:
		error(ERR_SYNTAX, input_filename, input_line_no);
		to_do = NULL_NP_AND_FREE_LIST;
		break;
	}

	switch (to_do)
	{
	case MAKE_STANDARD_NODE:
		np = new_node(input_line_no, opcp);
		np->n_vlhp = vlhp;
		break;

	case FREE_LIST_BUT_KEEP_NP:
		free_value_list(vlhp);
		break;

	case NULL_NP_AND_FREE_LIST:
		free_value_list(vlhp);
		np = NULL;
		break;

	case DO_NOTHING:
		break;
	}

	return np;
}


struct val_list_head *
eval_elist(vlhp, vp)
	register struct val_list_head *vlhp;
	register struct value   *vp;
{
	if (vlhp == NULL)
	{
		/* This is the first item of the list;
		 *	allocate a list-head structure.
		 */
		vlhp = new_val_list_head();

		/* (the structure is init'd to an empty list when we get it) */
	}

	if ( vp != NULL )
	{
		vp->v_next = (struct value *)NULL;

		/* link the element onto the end of the list */
		if (vlhp->vlh_head == NULL)	vlhp->vlh_head         = vp;
		else				vlhp->vlh_tail->v_next = vp;
		vlhp->vlh_tail = vp;

		/* and increment the count of elements in the list */
		vlhp->vlh_listlen++;
	}

	return vlhp;
}


/* FP_ALIGNMENT_MASK() returns mask with which to test "alignment" of a
 * floating-point register number, given the operand's width in bytes.
 * AND'ing the register number with this mask tests the low bits of the
 * register number for alignment; the AND'ed result should = 0.
 *	width = 4  (mask = 00): no register alignment
 *	width = 8  (mask = 01): even-register alignment
 *	width = 16 (mask = 03): quad-register alignment
 */
#define FP_ALIGNMENT_MASK(width)	( ((width)>>2) - 1 )

static Node *
form_fp_node(opcp, rs1, rs2, rd)
	register struct opcode *opcp;
	register Regno rs1, rs2, rd;
{
	register Node *np;

	/* check the F.P. registers' alignment for this instruction,
	 * and if OK, setup instruction
	 */
	/* opcp->o_func.f_r*_width gives 4, 8, or 16 (in units of bytes) for
	 * SGL, DBL, or QUAD, respectively.
	 * The alignment masks will be 01, 02, or 03, respectively.
	 */
	if ( ( (RAW_REGNO(rs1) & FP_ALIGNMENT_MASK(opcp->o_func.f_rs1_width))
		== 0 ) &&
	     ( (RAW_REGNO(rs2) & FP_ALIGNMENT_MASK(opcp->o_func.f_rs2_width))
		== 0 ) &&
	     ( (RAW_REGNO(rd ) &  FP_ALIGNMENT_MASK(opcp->o_func.f_rd_width))
		== 0 )  ||
	     (opcp->o_func.f_subgroup == FSG_FMOV) || 
	     (opcp->o_func.f_subgroup == FSG_FABS) ||
	     (opcp->o_func.f_subgroup == FSG_FNEG)
	   )
	{
		np = new_node(input_line_no, opcp);
		/* The following assume that the CALLER has either applied
		 * chk_regno() or assigned RN_NONE to each of these.
		 */
		np->n_operands.op_regs[O_RS1] = rs1;
		np->n_operands.op_regs[O_RS2] = rs2;
		np->n_operands.op_regs[O_RD]  = rd;
	}
	else
	{
		error(ERR_REG_ALIGN, input_filename, input_line_no);
		np = NULL;
	}

	return np;
}


static Node *
form_cp_node(opcp, cp_opcode, rs1, rs2, rd)
	register struct opcode *opcp;
	register U8BITS cp_opcode;
	register Regno rs1, rs2, rd;
{
	register Node *np;

	np = new_node(input_line_no, opcp);
	/* The following assume that the CALLER has either applied
	 * chk_regno() or assigned RN_NONE to each of these.
	 */
	np->n_operands.op_regs[O_RS1] = rs1;
	np->n_operands.op_regs[O_RS2] = rs2;
	np->n_operands.op_regs[O_RD]  = rd;
	np->n_operands.op_cp_opcode = cp_opcode;

	return np;
}

