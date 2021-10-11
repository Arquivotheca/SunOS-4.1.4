static char sccs_id[] = "@(#)symbol_expression.c	1.1\t10/31/94";

/* The functions in this file are all associated with the creation, storing,
 * evaluation, and disassembly(i.e. naming) of expressions whose evaluation
 * must be delayed until after input.
 *
 * So far, we only support taking the DIFFERENCE of two relocatable symbols,
 * but adding others should be pretty easy (if we wanted to).
 */

#include <stdio.h>
#include "structs.h"
#include "sym.h"
#include "bitmacros.h"
#include "errors.h"
#include "globals.h"
#include "alloc.h"
#include "node.h"
#include "opcode_ptrs.h"
#include "symbol_expression.h"

struct value *
add_null_symbol_to_value(vp)
	register struct value *vp;
{
	extern char *get_scratch_buf();
	register struct symbol *symp;
	char * symname;

	symname = get_scratch_buf(11);
	sprintf(symname,"%ld", vp->v_value);
	symp = sym_new(symname);
	symp->s_segment = ASSEG_ABS;
	SET_BITS(symp->s_attr,(SA_DEFINED|SA_DEFN_SEEN|SA_NOWRITE));
	symp->s_opertype = OPER_NONE;
	symp->s_value = 0;
	vp->v_symp = symp;
	return vp;
}
		

char *
oper_lookup(op)
	register OperatorType op;
{
	switch (op)
	{
	      case OPER_NONE:
	      case OPER_HILO_NONE:
	      case OPER_HILO_HI:
	      case OPER_HILO_LO: 
		return " ";
	      case OPER_ADD_PLUS:
		return "+";
	      case OPER_ADD_MINUS:
		return "-";
	      case OPER_MULT_MULT:
		return "*";
	      case OPER_MULT_DIV:
		return "/";
	      case OPER_MULT_MOD:
		return "%";
	      case OPER_SHIFT_RIGHT:
		return ">";
	      case OPER_SHIFT_LEFT:
		return "<";
	      case OPER_LOG_AND:
		return "&";
	      case OPER_LOG_OR:
		return "|";
	      case OPER_LOG_XOR:
		return "^";
	      case OPER_BITNOT:	
		return "~";
	      default:
		return " ";
	}
}



struct symbol *
create_symbol_expression(sym1p, addend1, op, sym2p, addend2, lineno)
	register struct symbol *sym1p;
	long int addend1;
	register OperatorType  op;
	register struct symbol *sym2p;
	long int addend2;
	LINENO lineno;
{
	/* Return a pointer to a new (artificial) symbol which is actually an
	 * expression involving other symbols.
	 *
	 * In the case of a binary operator:
	 * ====================================================================
	 *
	 *	We are given:
	 *	---------------------------------------------------------------
	 *
	 *		sym1p		    op				sym2p
	 *		  |		      				  |
	 *		  V		      				  V
	 *	      [struct sym]			          [struct sym]
	 *
	 *	We want to create:
	 *	---------------------------------------------------------------
	 *
	 *	sym1p		    symp			   sym2p
	 *	  |		      |				    |
	 *	  |		      V				    |
	 *	  |	         [struct sym]			    |
	 *	  |         +----.s_def_node			    |
	 *	  |         |    .s_opertype = op                   |
	 *	  |         |    .s_def_node2----+		    |
	 *	  |         |                    |		    |
	 *	  |         V                    V		    |
	 *	  | [struct node]	     [struct node]          |
	 *	  | .n_opcp---------------+ +-.n_opcp               |
	 *	  | .n_operands.op_addend | | .n_operands.op_addend |
	 *	  |        = addend1      | |        = addend2      |
	 *	  | .n_operands.op_symp   | | .n_operands.op_symp   |
	 *	  |	 |		  | |  	           |	    |
	 *	  V	 V		  | |		   V	    V
	 *	[struct sym]		  V V		  [struct sym]
	 *			  [struct opcode]
	 *			  .o_func.f_group = FG_SYM_REF
	 *
	 * In the case of the OPER_EQUATE operator:
	 * ====================================================================
	 *
	 *	We are given:
	 *	---------------------------------------------------------------
	 *
	 *		sym1p		    op = OPER_EQUATE		sym2p
	 *		  |		      				  |
	 *		  V		      				  |
	 *	+--->[struct sym]					  |
	 *	|  +---.s_def_node					  |
	 *	|  |   .s_def_node2 = NULL				  |
	 *	|  |							  |
	 *	|  |							  |
	 *	|  +->[struct node]					  |
	 *	+-----.n_operands.op_symp				  |
	 *	      .n_opcp--+					  V
	 *	   	       |				  [struct sym]
	 *	   	       V				(expression sym)
	 *	     [struct opcode]
	 *	      .o_func.f_group
	 *		  = FG_EQUATE
	 *
	 *
	 *	We want to create:
	 *	---------------------------------------------------------------
	 *
	 *		symp (== sym1p)					sym2p
	 *		  |		      				  |
	 *		  V		      				  |
	 *	+--->[struct sym]					  |
	 *	|  +---.s_def_node					  |
	 *	|  |   .s_opertype = OPER_EQUATE			  |
         *	|  |   .s_def_node2 ------------>[struct node]	          |
	 *	|  |			          .n_operands.op_addend   |
	 *	|  |			                 = addend2        |
	 *	|  |			          .n_operands.op_symp     |
	 *	|  +->[struct node]            +--.n_opcp          |      |
	 *	+-----.n_operands.op_symp      |                   |      |
	 *	      .n_opcp--+	       |                   |      |
	 *	   	       |	       |		   V      V
	 *	   	       |	       |	 	 [struct sym]
	 *	   	       |	       |	       (expression sym)
	 *	   	       V	       V
	 *	     [struct opcode]	  [struct opcode]
	 *	      .o_func.f_group	   .o_func.f_group
	 *		  = FG_EQUATE	      = FG_SYM_REF
	 */

	register struct symbol *symp;
	char *symname;

	switch ( op )
	{
	case OPER_ADD_MINUS:
        case OPER_ADD_PLUS:
        case OPER_MULT_MULT:
	case OPER_MULT_DIV:
	case OPER_MULT_MOD:
	case OPER_SHIFT_LEFT:
	case OPER_SHIFT_RIGHT:
        case OPER_LOG_AND:
	case OPER_LOG_OR:
	case OPER_LOG_XOR:
	case OPER_BITNOT:
		/* If sym1p->s_symname is "ABC" and sym2p->s_symname is "XYZ",
		 * then symp->s_symname will be "(ABC<op>XYZ)", where <op> is
		 * the expression operator (ie,"-" for MINUS).
		 */

		symname = (char *)as_malloc( strlen(sym1p->s_symname) +
					     strlen(sym2p->s_symname) + 4);
		strcpy(symname, "(");
		strcat(symname, sym1p->s_symname);
		strcat(symname, oper_lookup(op));
		strcat(symname, sym2p->s_symname);
		strcat(symname, ")");

		if ( BIT_IS_ON(sym1p->s_attr, SA_DYN_EVAL) ||
		     BIT_IS_ON(sym2p->s_attr, SA_DYN_EVAL) )
		{
			/* One or both of its subexpressions require dynamic
			 * evaluation, i.e. must be re-evaluated every time.
			 * Therefore, we must have a separate copy of this
			 * expression so it can have a unique value.
			 */
			symp = sym_new(symname);
			SET_BIT(symp->s_attr, SA_DYN_EVAL);
		}
		else
		{
			/* It doesn't have to be unique, since none of its
			 * subexpressions require dynamic evaluation.
			 */
			symp = sym_lookup(symname);
			if ( SYM_NOTFOUND(symp) )
			{
				symp = sym_add(symname);
			}
		}

		SET_BITS(symp->s_attr, (SA_EXPRESSION|SA_NOWRITE));
		symp->s_opertype  = op;
		symp->s_def_node  =
			new_node(lineno, opcode_ptr[OP_SYM_REF]);
		symp->s_def_node2 =
			new_node(lineno, opcode_ptr[OP_SYM_REF]);
		symp->s_def_node->n_operands.op_symp    = sym1p;
		symp->s_def_node->n_operands.op_addend  = addend1;
		symp->s_def_node2->n_operands.op_symp   = sym2p;
		symp->s_def_node2->n_operands.op_addend = addend2;
		node_references_label(symp->s_def_node,  sym1p);
		node_references_label(symp->s_def_node2, sym2p);

		break;

	case OPER_EQUATE:
		symp = sym1p;
		SET_BIT(symp->s_attr, SA_EXPRESSION);
		symp->s_opertype  = op;
		/* symp->s_def_node is already set. */
		symp->s_def_node2 = new_node(lineno, opcode_ptr[OP_SYM_REF]);
		symp->s_def_node2->n_operands.op_symp   = sym2p;
		symp->s_def_node2->n_operands.op_addend = addend2;
		node_references_label(symp->s_def_node2, sym2p);
		break;

	default:
		internal_error("create_symbol_expression(): op %d ?", op);
		/*NOTREACHED*/
	}

	return symp;
}


Bool
evaluate_symbol_expression(symp, dot_segment, dot_seg_offset)
	register struct symbol *symp;		/* ptr to symbol.	      */
	register SEGMENT	dot_segment;	/* segment to use for '.'     */
	register LOC_COUNTER	dot_seg_offset;	/* offset in segment, for '.' */
{
	if (BIT_IS_ON(symp->s_attr, SA_EXPRESSION))
	{
		/* Here, we have an expression to evaluate.
		 * We couldn't do so before optimization,
		 * since the symbols might move during optimization.
		 * But we MUST evaluate it now, before emitting the result!
		 */
		struct symbol *sym1p;
		struct symbol *sym2p;
		long int addend1;
		long int addend2;
		long int val1;
		long int val2;
		OperatorType op = symp->s_opertype;

		/* We are faced with a binary operator */
		if ((op != OPER_EQUATE) && (op != OPER_DOT))
		{
			sym1p   = symp->s_def_node->n_operands.op_symp;
			addend1 = symp->s_def_node->n_operands.op_addend;
			sym2p   = symp->s_def_node2->n_operands.op_symp;
			addend2 = symp->s_def_node2->n_operands.op_addend;

			/* First, (recursively) evaluate the 2 subexpressions.*/
			if ( !evaluate_symbol_expression( sym1p, dot_segment,
							  dot_seg_offset )
			     ||
			     !evaluate_symbol_expression( sym2p, dot_segment,
							  dot_seg_offset)
			   ) return FALSE;
			val1    = sym1p->s_value + addend1;
			val2    = sym2p->s_value + addend2;
		}
	        switch (op)
		{
		case OPER_ADD_MINUS:
		case OPER_ADD_PLUS:
			
			if ( SYMBOL_IS_ABSOLUTE_VALUE(sym2p) )
			{
				symp->s_value = (op == OPER_ADD_MINUS) ?
				    ((sym1p->s_value - sym2p->s_value) + (addend1 - addend2)) :
				    ((sym1p->s_value + sym2p->s_value) + (addend1 + addend2)) ;
				   
				symp->s_segment = sym1p->s_segment;
				/* Mark it seen/defined only if sym1 was
				 * seen/defined.
				 */
				symp->s_attr |=
					sym1p->s_attr&(SA_DEFN_SEEN|SA_DEFINED);
			}
			else if (SYMBOL_IS_ABSOLUTE_VALUE(sym1p) )
			{
				if (op == OPER_ADD_MINUS)
				{
					/* Cannot substarct a relocatable from
					 * an absolute.
					 */
					error(ERR_ABSREQD, input_filename,
					      symp->s_def_node->n_lineno);
					return FALSE;
				}
				else
				{				
					symp->s_value = sym1p->s_value + addend1 +
					                sym2p->s_value + addend2;
					
					symp->s_segment = sym2p->s_segment;
					/* Mark it seen/defined only if sym2 was
					 * seen/defined.
					 */
					symp->s_attr |=
                                            sym2p->s_attr&(SA_DEFN_SEEN|SA_DEFINED);
				}
			}
			else if ( BIT_IS_OFF(sym1p->s_attr, SA_DEFINED) ||
				  BIT_IS_OFF(sym2p->s_attr, SA_DEFINED)
				)
			{
				/* Cannot calculate a difference between them
				 * if either one is an external reference.
				 */
				error(ERR_CANTCALC_EX, input_filename,
						symp->s_def_node->n_lineno);
				return FALSE;
			}
			else if (sym1p->s_segment != sym2p->s_segment)
			{
				/* Cannot calculate a difference between them
				 * if they aren't in the same segment.
				 */

				error(ERR_CANTCALC_DS, input_filename,
						symp->s_def_node->n_lineno);
				return FALSE;
			}
			else
			{
				if (op == OPER_ADD_PLUS)
				{
					/* Cannot add two relocatables.
					 */
					error(ERR_ABSREQD, input_filename,
					      symp->s_def_node->n_lineno);
					return FALSE;
				}
				else
				{
					symp->s_value = (sym1p->s_value - sym2p->s_value) +
					                (addend1 - addend2);
					symp->s_segment = ASSEG_ABS;
					/* Now it's all defined; mark it as such. */
					SET_BITS( symp->s_attr,
						  (SA_DEFN_SEEN|SA_DEFINED) );
				}
			}
			/* Can only clear the SA_EXPRESSION bit if the
			 * expression doesn't have to be evaluated
			 * dynamically; i.e. it doesn't contain '.',
			 * which must be re-evaluated each time.
			 */
			if ( BIT_IS_OFF(symp->s_attr, SA_DYN_EVAL) )
			{
				CLR_BIT(symp->s_attr, SA_EXPRESSION);
			}
			break;

		case OPER_MULT_MULT:
		case OPER_MULT_DIV:
		case OPER_MULT_MOD:
		case OPER_SHIFT_RIGHT:
	        case OPER_SHIFT_LEFT:
                case OPER_LOG_AND:
		case OPER_LOG_OR:
		case OPER_LOG_XOR:
		case OPER_BITNOT:
			if ( SYMBOL_IS_ABSOLUTE_VALUE(sym1p) &&
			     SYMBOL_IS_ABSOLUTE_VALUE(sym2p) )
			{
				switch (op)
				{
				      case OPER_MULT_MULT:
					symp->s_value = val1 * val2;
					break;
					
				      case OPER_MULT_DIV:
					if (val2 == 0)
					{
						error(ERR_DIVZERO, input_filename, input_line_no);
						return FALSE;
					}
					else 
					    symp->s_value = val1 / val2;
					break;
					
				      case OPER_MULT_MOD:
					symp->s_value = val1 % val2;
					break;

				      case OPER_SHIFT_RIGHT:
					/* use UNSIGNED value to guarantee
					 * LOGICAL right shift 
					 */
					(unsigned long int)(symp->s_value) =
					       (unsigned long int)(val1) >> val2;
					break;

				      case OPER_SHIFT_LEFT:
					symp->s_value = val1 << val2;
					break;

				      case OPER_LOG_AND:
					symp->s_value = val1 & val2;
					break;

				      case OPER_LOG_OR:
					symp->s_value = val1 | val2
;					break;

				      case OPER_LOG_XOR:
					symp->s_value = val1 ^ val2;
					break;

				      case OPER_BITNOT:
					symp->s_value = ~(val2);
					break;

				}
				symp->s_segment = ASSEG_ABS;
				/* Now it's all defined; mark it as such. */
				SET_BITS( symp->s_attr,
					 (SA_DEFN_SEEN|SA_DEFINED) );
		        }
			else
			{
				error(ERR_ABSREQD, input_filename,
				      symp->s_def_node->n_lineno);
				return FALSE;
			}
			break;				
			
		case OPER_EQUATE:
			sym2p   = symp->s_def_node2->n_operands.op_symp;
			addend2 = symp->s_def_node2->n_operands.op_addend;

			/* First, (recursively) evaluate the equated-to symbol
			 * or expression.
			 */
			if ( !evaluate_symbol_expression( sym2p, dot_segment,
							  dot_seg_offset)
			   )
			{
				return FALSE;
			}


			/* Then, evaluate this one. */
			if ( BIT_IS_ON(sym2p->s_attr, SA_DEFINED) )
			{
				symp->s_value = sym2p->s_value + addend2;
				symp->s_segment = sym2p->s_segment;
				/* Don't need to set SA_DEFN_SEEN, as that was
				 * done in eval_equate().  Just set DEFINED.
				 */
				/* Now it's all defined; mark it as such. */
				SET_BITS(symp->s_attr, SA_DEFINED);

				/* Can only clear the SA_EXPRESSION bit if the
				 * expression doesn't have to be evaluated
				 * dynamically; i.e. it doesn't contain '.',
				 * which must be re-evaluated each time.
				 */
				if ( BIT_IS_OFF(symp->s_attr, SA_DYN_EVAL) )
				{
					CLR_BIT(symp->s_attr, SA_EXPRESSION);
				}
			}
			else
			{
				/* The equated-to value must involve an external
				 * symbol, therefore isn't defined.  If it isn't
				 * defined, then this one isn't, either.
				 */
				error(ERR_CANTCALC_EX, input_filename,
						(symp->s_def_node == NULL)
						  ? input_line_no
						  : symp->s_def_node->n_lineno);
				return FALSE;
			}
			break;
		
		case OPER_DOT:
			/* Provide a "real" value for DOT. */
			symp->s_segment = dot_segment;
			symp->s_value   = dot_seg_offset;
			SET_BITS(symp->s_attr, (SA_DEFN_SEEN|SA_DEFINED));
			break;

		default:
			internal_error("evaluate_symbol_expression(): op %d?",
					symp->s_opertype);
			/*NOTREACHED*/
		}
	}

	return TRUE;
}


char *
get_dynamic_symbol_name(symp)
	register struct symbol *symp;
{
	register char *symname;

	if ( BIT_IS_ON(symp->s_attr, SA_LOCAL) )
	{
		/* For local symbols, which are NEVER expressions! */
		symname = (char *)as_malloc(strlen(symp->s_symname) + 3);
		strcpy(symname, "L.");
		strcat(symname, symp->s_symname);
	}
	else if ( BIT_IS_ON(symp->s_attr, SA_EXPRESSION) )
	{
		/* Dynamically re-create the expression "name", since the
		 * constituent symbols may have changed names during
		 * optimization.
		 */
	
		register char *left_symname;
		register char *right_symname;

		switch (symp->s_opertype)
		{
		case OPER_ADD_MINUS:
			/* First, get the two constituent symbol names (which
			 * may be expressions, themselves).
			 */
			left_symname  = get_dynamic_symbol_name(
					 symp->s_def_node->n_operands.op_symp);
			right_symname = get_dynamic_symbol_name(
					 symp->s_def_node2->n_operands.op_symp);

			/* If left_symname is "ABC" and right_symname is "XYZ",
			 * then symp->s_symname will be "(ABC<op>XYZ)", where
			 * <op> is the expression operator (only "-" for now).
			 */
			symname = (char *)as_malloc( strlen(left_symname)
						   + strlen(right_symname) + 4);
			strcpy(symname, "(");
			strcat(symname, left_symname);
			strcat(symname, "-");
			strcat(symname, right_symname);
			strcat(symname, ")");

			if ( strcmp(symname, symp->s_symname) == 0 )
			{
				/* The name didn't change after all; free up
				 * the space allocated, and just return a
				 * pointer to the original string.
				 */
				as_free(symname);
				symname = symp->s_symname;
			}
			
			break;

		case OPER_EQUATE:
			/* Just use the EQUATE symbol's name. */
			symname = symp->s_symname;
			break;

		case OPER_DOT:
			/* Just use the symbol's name (i.e. "."). */
			symname = symp->s_symname;
			break;

		default:
			internal_error("get_dynamic_symbol_name(): oper %d?",
					symp->s_opertype);
			/*NOTREACHED*/
		}
	}
	else	symname = symp->s_symname;

	return symname;
}
