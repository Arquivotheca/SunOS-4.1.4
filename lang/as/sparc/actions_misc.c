static char sccs_id[] = "@(#)actions_misc.c	1.1\t10/31/94";
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
#include "operands.h"
#include "label.h"
#include "opcode_ptrs.h"
#include "registers.h"
#include "symbol_expression.h"

static Node *
make_equate_node(symp, value)
	struct symbol *symp;
	long int value;
{
	register Node *np;

	np = new_node(input_line_no, opcode_ptr[OP_EQUATE]);
	np->n_operands.op_symp   = symp;
	np->n_operands.op_addend = value;

	/* Mark the equate symbol as to which node defined it. */
	node_defines_label(np, symp);

	return np;
}

Node *
eval_equate(symname, vp)
	register char *symname;		/* Name of symbol on LHS	*/
	register struct value *vp;	/* Expression (value) on RHS	*/
{
	register struct symbol *symp;
	register Node *np;

	symp = sym_lookup(symname);
	if ( SYM_FOUND(symp) )
	{
		/* symbol was found OK */
	}
	else
	{
		/* not found, therefore not yet defined.*/
		/* if it's not a "local" symbol, add it to symbol table.*/
		if ( (*symname >= '0') && (*symname <= '9') )
		{
			/* attempted "equate" of a local symbol */
			error(ERR_ILL_LOCAL, input_filename, input_line_no);
			return (Node *)NULL;
		}
		else	symp = sym_add(symname);
	}

	if ( chk_redefn(symp) )
	{
		/* symbol is being erroneously re-defined */
		np = NULL;
	}
	else
	{
		SET_BIT(symp->s_attr, SA_DEFN_SEEN);

		if (vp->v_symp == NULL)
		{
			/* The symbol is now down to an absolute value.  Mark
			 * that we've seen the definition of this symbol, and
			 * define it now (no need to wait until the ass'y pass).
			 */
			define_symbol(symp, ASSEG_ABS, vp->v_value);

			/* Don't bother to make a node, unless we are going to
			 * disassemble later.
			 */
			if (do_disassembly)
			{
				np = make_equate_node(symp, vp->v_value);
			}
			else	np = NULL;
		}
		else
		{
			/* The RHS isn't an absolute value, but is a symbolic
			 * expression which we will try to evaluate later,
			 * at code emission time.
			 */
			symp = create_symbol_expression(symp, 0, OPER_EQUATE,
					vp->v_symp, vp->v_value, input_line_no);
			np = make_equate_node(symp, vp->v_value);
		}
	}

	/* In any case, the scratch value structure has served its purpose,
	 * and is no longer needed.
	 */
	free_scratch_value(vp);

	return np;
}


Node *
eval_comment(sp)
	register char *sp;
{
	register Node *np;

	/* don't bother to make a node, unless we are going to disass.*/
	if (do_disassembly)
	{
		np = new_node(input_line_no, opcode_ptr[OP_COMMENT]);
		np->n_string = sp;
	}
	else	np = NULL;

	return np;
}


unsigned int
eval_asi(vp)
	register struct value *vp;
{
	chk_absolute(vp);

	if ( (vp->v_value < 0) || (vp->v_value > 255) )
	{
		error(ERR_VAL_ASI, input_filename, input_line_no);
		vp->v_value = 0;
	}

	/* In any case, the scratch value structure has served its purpose,
	 * and is no longer needed.
	 */
	free_scratch_value(vp);

	return (unsigned int)(vp->v_value);
}


Node *
create_label(symname, np)
	register char *symname;	/* ptr to symbol name */
	register Node *np;
{
	register struct symbol *symp;

	symp = sym_lookup(symname);
	if ( SYM_FOUND(symp) )
	{
		/* Symbol was found OK in symbol table.*/
	}
	else
	{
		/* It was not found, therefore not yet defined in the symbol
		 * table.  Add it to symbol table.
		 */
		symp = sym_add(symname);
		if ( (*symname >= '0') && (*symname <= '9') )
		{
			/* definition of a local symbol */
			symp->s_attr |= (SA_LOCAL|SA_NOWRITE);
		}
	}

	if ( chk_redefn(symp) )
	{
		/* The symbol is being erroneously re-defined.	*/
		/* Don't allow a label node to be returned.	*/
		free_node(np);
		np = NULL;
	}
	else
	{
		/* Mark that we've seen the definition of this symbol */
		SET_BIT(symp->s_attr, SA_DEFN_SEEN);

		/* OK to define the symbol, in emit_label */
		np->n_operands.op_symp = symp;

		/* mark the label symbol as to which node defined it */
		node_defines_label(np, symp);
	}

	return np;
}


Node *
eval_label(symname)
	register char *symname;	/* ptr to symbol name */
{
	return  create_label(symname,
			     new_node(input_line_no, opcode_ptr[OP_LABEL]) );
}


struct operands *
eval_RpR(regno1, addop, regno2)	/* Register plus Register */
	register Regno regno1;
	register OperatorType addop;
	register Regno regno2;
{
	register Regno	   reg1, reg2;
	register struct operands *operp;

	operp = new_operands();

	reg1 = chk_regno(regno1, RM_GP);
	reg2 = chk_regno(regno2, RM_GP);

	if (addop == OPER_ADD_PLUS)
	{
		operp->op_regs[O_RS1] = reg1;
		operp->op_regs[O_RS2] = reg2;
	}
	else
	{
		/* can only have [REG+REG], not [REG-REG] ! */
		error(ERR_ADDRSYNTAX, input_filename, input_line_no);
	}

	return operp;
}


struct operands *
eval_RpC(regno1, addop, vp)		/* Register plus Constant */
	register Regno regno1;
	register OperatorType addop;
	register struct value *vp;
{
	register Regno reg1;
	register struct operands *operp;

	reg1 = chk_regno(regno1, RM_GP);

	if (addop == OPER_ADD_PLUS)
	{
		operp = get_operands13(vp);
		operp->op_regs[O_RS1] = RAW_REGNO(reg1  );
		operp->op_regs[O_RD]  = RAW_REGNO(RN_GP(0));
	}
	else
	{
		/* Can only have [const+REG], [REG+const], or [REG-const];
		 * not [const-REG] !
		 *
		 * (note that "addop" always comes in as OPER_ADD_PLUS when
		 * called from Yacc rule which recognizes "[ REG +/- const ]").
		 */
		error(ERR_ADDRSYNTAX, input_filename, input_line_no);
		operp = new_operands();
	}

	/* In any case, the scratch value structure has served its purpose,
	 * and is no longer needed.
	 */
	free_scratch_value(vp);

	return operp;
}


struct operands *
eval_REG_to_ADDR(regno)
	register Regno regno;
{
	register Regno reg1;
	register struct operands *operp;

	reg1 = chk_regno(regno, RM_GP);
	operp = new_operands();
	operp->op_regs[O_RS1] = RAW_REGNO(reg1  );
	operp->op_regs[O_RS2] = RAW_REGNO(RN_GP(0));

	return  operp;
}


struct operands *
get_operands13(vp)		/* get operands (with const13) */
	register struct value *vp;
{
	struct operands *operp;

	operp = new_operands();
	operp->op_symp     = vp->v_symp;
	operp->op_addend   = vp->v_value;
	operp->op_imm_used = TRUE;
	operp->op_reltype  = REL_13;

	if (vp->v_symp == NULL)
	{
		/* The value is an absolute number.			*/
		/* (i.e. no relocatable symbol is associated with it)	*/

		if ( (vp->v_value < -0x1000) || (vp->v_value > 0x0fff) )
				   /*-4096*/                  /*4095*/
		{
			error(ERR_VAL_13, input_filename, input_line_no);
			operp->op_addend = 0;
		}
	}
	else
	{
		/* a relocatable value. */

		switch (vp->v_relmode)
		{	
		case VRM_NONE:
			/* leave it set to REL_13 */
			break;
		case VRM_LO10:
			operp->op_reltype = REL_LO10;
			break;
		default:
			error(ERR_PCT_HILO, input_filename, input_line_no);
			operp->op_reltype = REL_NONE;
			operp->op_symp    = NULL;
			operp->op_addend  = 0;
		}
	}

	return operp;
}
