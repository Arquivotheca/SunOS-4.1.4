static char sccs_id[] = "@(#)actions_expr.c	1.1\t10/31/94";   


/*
 * This file contains actions associated with the parse rules in "parse.y".
 *
 * Also see "actions_*.c".
 *
 */

#include <stdio.h>
#include "structs.h"
#include "sym.h"
#include "y.tab.h"
#include "bitmacros.h"
#include "errors.h"
#include "actions.h"
#include "globals.h"
#include "build_instr.h"
#include "segments.h"
#include "alloc.h"
#include "node.h"
#include "scratchvals.h"
#include "symbol_expression.h"

struct value *eval_uadd();	/* forward declaration */

/*---------------------------------------------------------------------------*/
/*	Evaluation routines for "expr"-related rules follow...
/*---------------------------------------------------------------------------*/

static void
chk_relmode(vp, ok_relmode, err_number)
	register struct value *vp;
	register RELMODE ok_relmode;
	register int err_number;
{
	if ( vp->v_relmode == ok_relmode )
	{
		/* all is OK */
	}
	else
	{
		error(err_number, input_filename, input_line_no);
		vp->v_relmode = VRM_NONE;
	}
}

struct value *
eval_hilo(hilo, vp)
	register OperatorType  hilo;
	register struct value *vp;
{
	switch ( hilo )
	{
	case OPER_HILO_LO:	/* %lo */
		if ( VALUE_IS_ABSOLUTE(vp) )
		{
#ifdef CHIPFABS
			if (iu_fabno == 1)
			{
				/* make it 9-bit instead */
				vp->v_value &= MASK(9);
			}
			else	vp->v_value &= MASK(10);
#else /*CHIPFABS*/
			vp->v_value &= MASK(10);
#endif /*CHIPFABS*/
		}
		else
		{
       		if ( vp->v_relmode == VRM_NONE )
			{
				/* let relocate() or the linker do the job */
				vp->v_relmode = VRM_LO10;
			}
			else
			{
				error(ERR_PCT_HILO, input_filename,
					input_line_no);
			}
		}
		break;

	case OPER_HILO_HI:	/* %hi */
		if ( VALUE_IS_ABSOLUTE(vp) )
		{
#ifdef CHIPFABS
			if (iu_fabno == 1)
			{
				/* make it 23-bit instead */
				vp->v_value = (vp->v_value >> 9) & MASK(23);
			}
			else	vp->v_value = (vp->v_value >> 10) & MASK(22);
#else /*CHIPFABS*/
			vp->v_value = (vp->v_value >> 10) & MASK(22);
#endif /*CHIPFABS*/
		}
		else
		{
			if ( vp->v_relmode == VRM_NONE )
			{
				/* let relocate() or the linker do the job */
				vp->v_relmode = VRM_HI22;
			}
			else
			{
				error(ERR_PCT_HILO, input_filename,
					input_line_no);
			}
		}
		break;
#ifdef DEBUG
	default:
		internal_error("eval_hilo(): bad hilo value");
#endif
	}

	return vp;
}


/********************* s y m b o l _ t o _ v a l u e  ************************/
struct value *
symbol_to_value(symname)
	register char *symname;
{
	register struct value *vp;

	vp = get_scratch_value(NULL, 0);

	if  ( (*symname == '.') && (*(symname+1) == '\0') )
	{
		/* Symbol '.' always refers to the address of the beginning of
		 * the current statement;  this must become an expression
		 * which will be evaluated at or after emit()-time.  Also,
		 * each reference to "." must be UNIQUE, since its value
		 * changes from statement to statement.
		 *
		 * Thus, "." is never really stored in the symbol table.
		 * It is marked as "DEFINED" so that the user will not be
		 * allowed to redefine it (e.g. as a label, or the left-hand
		 * side of an Equate).
		 */
		vp->v_symp = sym_new(".");
		vp->v_symp->s_attr |= ( SA_DEFN_SEEN  | SA_DEFINED  |
					SA_EXPRESSION | SA_DYN_EVAL |
					SA_NOWRITE );
		vp->v_symp->s_opertype = OPER_DOT;
		vp->v_value = 0;
	}
	else
	{
		register struct symbol *symp;

		symp = sym_lookup(symname);
		if ( SYM_NOTFOUND(symp) )
		{
			/* if not found, then add it to symbol table	*/
			symp = sym_add(symname);
			if ( (*symname >= '0') && (*symname <= '9') )
			{
				/* first reference to a local symbol */
				symp->s_attr |= (SA_LOCAL|SA_NOWRITE);
			}
		}

		if (symp->s_segment == ASSEG_ABS)
		{
			/* the symbol represents an absolute value; just copy
			 * that value and let the parser see it exactly as if
			 * it was a numeric constant, throwing away any memory
			 of the
			 * original symbol.
			 */
			vp->v_value = symp->s_value;
			vp->v_symp  = NULL;
		}
		else
		{
			/* it's a relocatable symbol, or a forward reference
			 * to an absolute one.
			 */
			vp->v_value = 0;	/* value is in the sym*/
			vp->v_symp  = symp;
		}
	}

	vp->v_strptr  = NULL;		/* in either case */
	vp->v_relmode = VRM_NONE;	/* in either case */

	return(vp);
}


/*********************** n u m _ t o _ s t r i n g  **************************/
struct value *
num_to_string(vp)
	register struct value *vp;
{
	extern char *get_scratch_buf();

	chk_absolute(vp);
	vp->v_strptr = get_scratch_buf(11);	/* '-', 9 digits, '\0', max */
	sprintf(vp->v_strptr, "%ld", vp->v_value);

	return(vp);
}


struct value *
eval_add(v1p, op, v2p)
	register struct value *v1p;
	register OperatorType  op;
	register struct value *v2p;
{
	register struct value *vp;

	/* We dead with this case without building a symbol expression because:
	 *  (1) It is a simple case
	 *  (2) It is the most frequent case
	 *  (3) Building the symbol expression uses a lot of memory.
	 */
	if ( VALUE_IS_ABSOLUTE(v2p) )
	{
		/* right-hand operand is absolute (i.e. {ABS|REL} +- ABS ) */

		chk_relmode(v1p, VRM_NONE, ERR_ARITH_HILO);

		/* no need to chk_relmode(v2p) here, because
		 * VALUE_IS_ABSOLUTE(v2p) implies that v2p->v_relmode==VRM_NONE.
		 */

		if (op == OPER_ADD_MINUS)
		{
			v2p->v_value = -(v2p->v_value);
		}
		vp = v2p;
		v1p->v_value += vp->v_value;
		vp = v1p;
	}
	else
	{
		/* right-hand operand is relocatable (i.e. {ABS|REL} +- REL ) */
		if ( VALUE_IS_ABSOLUTE(v1p) )
		{
			/* left-hand operand is absolute (i.e. ABS +- REL) */

			add_null_symbol_to_value(v1p);
			chk_relmode(v2p, VRM_NONE, ERR_ARITH_HILO);
		        v1p->v_symp = create_symbol_expression(
							      v1p->v_symp,
							      v1p->v_value,
							      op,
							      v2p->v_symp,
							      v2p->v_value,
							      input_line_no);
			v1p->v_value = 0;
		}
		else
		{
			/* left-hand operand is relocatable (i.e. REL+-REL) */

			chk_relmode(v1p, VRM_NONE, ERR_ARITH_HILO);
			chk_relmode(v2p, VRM_NONE, ERR_ARITH_HILO);

			v1p->v_symp = create_symbol_expression(
							       v1p->v_symp,
							       v1p->v_value,
							       op,
							       v2p->v_symp,
							       v2p->v_value,
							       input_line_no);
			v1p->v_value = 0;
		}
		vp = v1p;	
	}
	return vp;
}


/*  We use this function to evaluate *, /, %, >>, <<, &&, |, ^.  
 *  addition and substraction are treated as a special case because
 *  they are the most frequently used and we try to avoid to generate
 *  a symbol expression when we don't have to.
 */
struct value *
eval_bin(v1p, op, v2p)
	register struct value *v1p;
	register OperatorType  op;
	register struct value *v2p;
{
	if (VALUE_IS_ABSOLUTE(v1p) && VALUE_IS_ABSOLUTE(v2p))
	{
		/* (since both are absolute, no need to chk_relmode() ) */
		switch (op)
		{
		      case OPER_MULT_MULT:
			v1p->v_value *= v2p->v_value;
			break;
			
		      case OPER_MULT_MOD:
		      case OPER_MULT_DIV:
			if (v2p->v_value == 0)
			{
				error(ERR_DIVZERO, input_filename, input_line_no);
				v1p->v_value = 0;
			}
			else if (op == OPER_MULT_DIV) v1p->v_value /= v2p->v_value;
			else /*(op == OPER_MULT_MOD)*/ v1p->v_value %= v2p->v_value;
			break;
			
		      case OPER_SHIFT_RIGHT:
			/* use UNSIGNED value to guarantee
			 * LOGICAL right shift 
			 */
			((unsigned long int)(v1p->v_value)) >> = v2p->v_value;
			break;
			
		      case OPER_SHIFT_LEFT:
			v1p->v_value <<= v2p->v_value;
			break;
			
		      case OPER_LOG_AND:
			v1p->v_value &= v2p->v_value;
			break;
			
		      case OPER_LOG_OR:
			v1p->v_value |= v2p->v_value;
			break;
			
		      case OPER_LOG_XOR:
			v1p->v_value ^= v2p->v_value;
			break;
		}
		return v1p;
	}	
	else if (VALUE_IS_ABSOLUTE(v1p))
	{
		add_null_symbol_to_value(v1p);
		chk_relmode(v2p, VRM_NONE, ERR_ARITH_HILO);
	}
	else if (VALUE_IS_ABSOLUTE(v2p))
	{
		
		add_null_symbol_to_value(v2p);
		chk_relmode(v1p, VRM_NONE, ERR_ARITH_HILO);
	}
	else
	{
		chk_relmode(v1p, VRM_NONE, ERR_ARITH_HILO);
		chk_relmode(v2p, VRM_NONE, ERR_ARITH_HILO);
	}
	v1p->v_symp = create_symbol_expression(
					       v1p->v_symp,
					       v1p->v_value,
					       op,
					       v2p->v_symp,
					       v2p->v_value,
					       input_line_no);
	v1p->v_value = 0;
	return v1p;
}


struct value *
eval_bitnot(op, vp)		/* bitwise-not operation */
	register struct value *vp;
{
	register struct value *v_null_p;

	if (VALUE_IS_ABSOLUTE(vp))
	{
		/* (since is absolute, no need to chk_relmode() ) */
		vp->v_value = ~(vp->v_value);
		return vp;
	}
	/* otherwise                                                         */
	/* we turn this into an expression (0 ~ vp) which will be dealt with */
	/* in evaluate_symbol_expression.                                    */
	chk_relmode(vp, VRM_NONE, ERR_ARITH_HILO);
	v_null_p = get_scratch_value(NULL,0);
	add_null_symbol_to_value(v_null_p);
	v_null_p->v_symp = create_symbol_expression(
						    v_null_p->v_symp,
						    v_null_p->v_value,
						    op,
						    vp->v_symp,
						    vp->v_value,
						    input_line_no);
	return v_null_p;
}


struct value *
eval_uadd(op, vp)		/* unary '+' or '-' */
	register OperatorType  op;
	register struct value *vp;
{
	register struct value *v_null_p;

	/* if operator is '+', do nothing.			   */
	/* if operator is '-', turn it into an expression (0-vp)   */
	/* and deal with it in evaluate_symbol_expression().	   */
	if (op == OPER_ADD_MINUS)
	{
		if (VALUE_IS_ABSOLUTE(vp))
		{   
			vp->v_value = -(vp->v_value);
			return vp;
		}
		/* otherwise */
		chk_relmode(vp, VRM_NONE, ERR_ARITH_HILO);
		v_null_p = get_scratch_value(NULL,0);
		add_null_symbol_to_value(v_null_p);
		v_null_p->v_symp = create_symbol_expression(
							    v_null_p->v_symp,
							    v_null_p->v_value,
							    op,
							    vp->v_symp,
							    vp->v_value,
							    input_line_no);
		vp = v_null_p;
		
	}
	return vp;
}
