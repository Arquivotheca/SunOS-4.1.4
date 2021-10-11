static char sccs_id[] = "@(#)stab.c	1.1\t10/31/94";

#include <stdio.h>	/* for definition of NULL */
#include <stab.h>
#include <sun4/a.out.h>
#include "structs.h"
#include "errors.h"
#include "globals.h"
#include "segments.h"
#include "node.h"
#include "emit.h"
#include "obj.h"


#ifdef INTERIM_OBJ_FMT
static void
convert_ntype_to_asym(asymp, ntype)
	register struct aout_symbol *asymp;
	register long int ntype;
{
	asymp->s_dbx     = (ntype >> 5) & 0x7;
	asymp->s_segment = (ntype >> 1) & 0xf;
	asymp->s_global  = (   ntype  ) & 0x1;
}

static void
convert_ndesc_to_asym(asymp, ndesc)
	register struct aout_symbol *asymp;
	register long int ndesc;
{
	asymp->s_index = ndesc & 0xffff;
}
#endif

Node *stab_marker_np;
Bool do_stabs;

void
add_stab_node_to_list(np)
	register Node *np;
{
	if (np == NULL)	return;	/* just to be careful */

	if (stab_marker_np == NULL)	stab_marker_np =new_marker_node("text");

	insert_before_node(stab_marker_np, np);
}


void
emit_stab_nodes()
{
	do_stabs = TRUE;
	emit_list_of_nodes(stab_marker_np);
}


void
emit_stab(nargs, vps)
	struct value *(vps[]);
{
	register int argidx;
	long int ntype;
#ifdef INTERIM_OBJ_FMT
	static struct aout_symbol asym;
#else
	static struct nlist asym;
#endif
	static Bool asym_initialized = FALSE;
	static Bool stab_R_warn = FALSE;

	/* The first time this routine is called, "asym" will get initialized.*/
	if (!stab_R_warn && make_data_readonly){
                warning(WARN_STABS_R,input_filename,input_line_no);
                stab_R_warn = TRUE;
                make_data_readonly = FALSE;
        }
	if (!asym_initialized)
	{
		/* Just-to-be-safe, zero the initial symbol record, so that any
		 * unused fields (e.g. uninitialized bitfields) are zeroed.
		 */
		bzero(&asym, sizeof(asym));
		asym_initialized = TRUE;
	}

	switch (nargs)
	{
	case 3:	/* stabd */
		ntype = vps[0]->v_value;
#ifdef INTERIM_OBJ_FMT
		asym.s_string.ptr = NULL;
		/* n_type = operand 1 (numeric) */
		convert_ntype_to_asym(&asym, ntype);
		/* n_desc = operand 3 (numeric) */
		convert_ndesc_to_asym(&asym, vps[2]->v_value);
		/* n_value = DOT */
		asym.s_value = vps[1]->v_value;  
#else
		asym.n_un.n_name = NULL;
		asym.n_type      = ntype;
		asym.n_desc      = vps[2]->v_value;
		asym.n_value     = vps[1]->v_value;
#endif
		break;
	case 4:	/* stabn */
		ntype = vps[0]->v_value;
#ifdef INTERIM_OBJ_FMT
		asym.s_string.ptr = NULL;
		/* n_type = operand 1 (numeric) */
		convert_ntype_to_asym(&asym, ntype);
		/* n_desc = operand 3 (numeric) */
		convert_ndesc_to_asym(&asym, vps[2]->v_value);
		/* n_value = operand 4 (numeric or label) */
		asym.s_value = vps[3]->v_value;
		if ( vps[3]->v_symp != NULL )
		{
			asym.s_value += vps[3]->v_symp->s_value;
		}
#else
		asym.n_un.n_name = NULL;
		asym.n_type      = ntype;
		asym.n_desc      = vps[2]->v_value;
		asym.n_value     = vps[3]->v_value;
		if ( vps[3]->v_symp != NULL )
		{
			asym.n_value += vps[3]->v_symp->s_value;
		}
#endif
		break;
	case 5:	/* stabs */
		ntype = vps[1]->v_value;
#ifdef INTERIM_OBJ_FMT
		/* n_strx/strg table = string from operand 1 */
		asym.s_string.ptr = vps[0]->v_strptr;
		/* n_type = operand 2 (numeric) */
		convert_ntype_to_asym(&asym, ntype);
		/* n_desc = operand 4 (numeric) */
		convert_ndesc_to_asym(&asym, vps[3]->v_value);
		/* n_value = operand 5 value (numeric or label) */
		asym.s_value = vps[4]->v_value;
		if ( vps[4]->v_symp != NULL )
		{
			asym.s_value += vps[4]->v_symp->s_value;
		}
#else
		asym.n_un.n_name = vps[0]->v_strptr;
		asym.n_type      = ntype;
		asym.n_desc      = vps[3]->v_value;
		asym.n_value     = vps[4]->v_value;
		if ( vps[4]->v_symp != NULL )
		{
			asym.n_value += vps[4]->v_symp->s_value;
		}
#endif
		break;
	default:	internal_error("emit_stab(): nargs=%d?", nargs);
	}

	if (make_data_readonly && (ntype == N_DATA))
	{
		/* When we're making the Data segment ReadOnly, it will be
		 * appended to the end of the Text segment.  However, at the
		 * point when we're writing out these STAB's (during code
		 * emission), the Data symbols have not yet been changed into
		 * Text symbols.  Therefore, do not write them out at all.
		 *
		 * (later, maybe things can be organized so that they can be
		 *  written out AFTER code emission and symbol-value-changing.
		 */
	}
	else	(void) (objsym_write_aout_symbol(&asym));
}

/*** 
/*** .stabd:	/* used for line-number info, without creating a label. */
/*** 	3 operands
/*** 	n_un.n_strx = 0
/*** 	n_type      =  operand 1 (numeric)	[s_dbx + s_segment]
/*** 	n_other     =  operand 2 (numeric)  [ALWAYS 0]
/*** 	n_desc      =  operand 3 (numeric)	[s_index]
/*** 	n_value     =  DOT (value of the location counter)
/*** 
/*** .stabs:
/*** 	5 operands
/*** 	string table <- string from operand 1 (adding '\0')
/*** 	n_un.n_strx = index into string tbl of string from operand 1
/*** 	n_type      =  operand 2 (numeric)	[s_dbx + s_segment]
/*** 	n_other     =  operand 3 (numeric)  [ALWAYS 0]
/*** 	n_desc      =  operand 4 (numeric)	[s_index]
/*** 	n_value     =  operand 5 value (numeric or label)
/*** 
/*** .stabn:	/* used for line-number info */
/*** 	4 operands
/*** 	n_un.n_strx    = 0
/*** 	n_type      =  operand 1 (numeric)	[s_dbx + s_segment]
/*** 	n_other     =  operand 2 (numeric)  [ALWAYS 0]
/*** 	n_desc      =  operand 3 (numeric)	[s_index]
/*** 	n_value     =  operand 4 value (numeric or label)
/*** 		switch (n_type)
/*** 		{
/*** 		case N_FUN:
/*** 		case N_STSYM:
/*** 		case N_LCSYM:
/*** 		case N_SLINE:
/*** 		case N_SO:                                                
/*** 		case N_SOL:
/*** 		case N_ENTRY:
/*** 		case N_LBRAC:
/*** 		case N_RBRAC:
/*** 		case N_ECOML:
/*** 			/* it a label */
/*** 			if (operands[opno].sym_o == 0)
/*** 			{
/*** 			    DEBUG("stab last operand isn't a label\n");
/*** 			    PROG_ERROR(E_OPERAND); return;
/*** 			}
/*** 			break;
/*** 		default: 
/*** 			/* its a number */
/*** 			if (operands[opno].sym_o != 0)
/*** 			{
/*** 			    DEBUG("stab last operand isn't just a number\n");
/*** 			    PROG_ERROR(E_OPERAND); return;
/*** 			}
/*** 			break;
/*** 		}    
 ***/
