static char sccs_id[] = "@(#)disassemble.c	1.1\t10/31/94";
/*
 * This file contains routines which disassemble instructions from
 * "struct node"s.
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
#include "disassemble.h"
#include "registers.h"
#ifdef DEBUG
#  include "optimize.h"
#endif
#include "symbol_expression.h"

#define	DISFILE stdout
#define BADCHAR	'#'

static int disassemble_MARKER();	/* forward routine declaration */
static int disassemble_SYM_REF();	/* forward routine declaration */
static int disassemble_COMMENT();	/* forward routine declaration */
static int disassemble_NONE();		/* forward routine declaration */
static int disassemble_LD();		/* forward routine declaration */
static int disassemble_ST();		/* forward routine declaration */
static int disassemble_LDST();		/* forward routine declaration */
static int disassemble_ARITH();		/* forward routine declaration */
static int disassemble_WR();		/* forward routine declaration */
static int disassemble_SETHI();		/* forward routine declaration */
static int disassemble_BR();		/* forward routine declaration */
static int disassemble_TRAP();		/* forward routine declaration */
static int disassemble_CALL();		/* forward routine declaration */
static int disassemble_JMP();		/* forward routine declaration */
static int disassemble_RETT();		/* forward routine declaration */
static int disassemble_IFLUSH();	/* forward routine declaration */
static int disassemble_UNIMP();		/* forward routine declaration */
static int disassemble_RD();		/* forward routine declaration */
static int disassemble_FLOAT2();	/* forward routine declaration */
static int disassemble_FLOAT3();	/* forward routine declaration */
static int disassemble_RET();		/* forward routine declaration */
static int disassemble_NOP();		/* forward routine declaration */
static int disassemble_LABEL();		/* forward routine declaration */
static int disassemble_SET();		/* forward routine declaration */
static int disassemble_EQUATE();	/* forward routine declaration */
static int disassemble_PS();		/* forward routine declaration */
static int disassemble_CSWITCH();	/* forward routine declaration */

static int print_string();		/* forward routine declaration */

Bool	    disassemble_with_comments = TRUE;
static Bool disassemble_with_linenums = FALSE;
static Bool disassemble_with_regsets  = FALSE;	/* register life/usage info */
static Bool disassemble_with_seqnos   = FALSE;	/* internal opt. sequencing#s */
static Bool disassemble_with_BOF      = FALSE;	/* Beginning-Of-File info. */


Bool
disassembly_options(turn_opt_on, optptr)
	Bool  turn_opt_on;
	register char *optptr;
{
	/* Scan the option letters... */

#ifdef DEBUG
	if (*optptr != 'S')  internal_error("disassembly_options(): ! +/-S?");
#endif

	for (optptr++/*skip "S"*/;  *optptr != NULL;  optptr++)
	{
		switch(*optptr)
		{
		case 'B':
			disassemble_with_BOF      = !turn_opt_on;
			break;

		case 'C':
			disassemble_with_comments = !turn_opt_on;
			break;

		case 'L':
			disassemble_with_linenums = !turn_opt_on;
			break;

		case 'R':
			disassemble_with_regsets  = !turn_opt_on;
			break;

		case 'S':
			disassemble_with_seqnos   = !turn_opt_on;
			break;

		case 'b':
			disassemble_with_BOF      = turn_opt_on;
			break;

		case 'c':
			disassemble_with_comments = turn_opt_on;
			break;

		case 'l':
			disassemble_with_linenums = turn_opt_on;
			break;

		case 'r':
			disassemble_with_regsets = turn_opt_on;
			break;

		case 's':
			disassemble_with_seqnos = turn_opt_on;
			break;

		default:
			return FALSE;
		}
	}

	return TRUE;
}


void
disassemble_list_of_nodes(marker_np)
	register Node *marker_np;
{ 
	register Node *np;

	/* do nothing if list is non-existant */
	if (marker_np == NULL)	return;

	/* This works on the master (circular) list of doubly-linked nodes for
	 * a segment.  The beginning and end of the circular list is known by
	 * a marker node.
	 */

	if (!disassemble_node(marker_np))	return;

	for ( np = marker_np->n_next;
	      (np !=  NULL) && (!NODE_IS_MARKER_NODE(np));
	      np = np->n_next
	    ) 
	{
		if ( !disassemble_node(np) )	break;
	}
}


/*** #ifdef DEBUG	/* very useful when dbx'ing this program */
void
disassemble_counted_list_of_nodes(start_np, count)
	register Node *start_np;
	register int count;
{ 
	register Node *np;
	register int n;

	/* do nothing if list is non-existant */
	if (start_np == NULL)	return;

	/* This works on the master (circular) list of doubly-linked nodes for
	 * a segment.  The beginning and end of the circular list is known by
	 * a marker node.
	 */
	
	if (count < 0)
	{
		for ( n = 1;    count < 0;    n++, count++)
		{
			if (start_np->n_prev == NULL)
			{
				(void)disassemble_node(NULL);
				break;
			}
			start_np = start_np->n_prev;
			if (NODE_IS_MARKER_NODE(start_np))	break;
		}
		count = n;
	}

	if (!disassemble_node(start_np))	return;

	for ( np = start_np->n_next, n = 1;
	      (n < count) && ((np == NULL)||(!NODE_IS_MARKER_NODE(np)));
	      np = np->n_next, n++
	    ) 
	{
		if ( !disassemble_node(np) )	break;
	}
}

void
disassemble_node_to_node(start_np, end_np)
	register Node *start_np;
	register Node *end_np;
{ 
	register Node *np;

	/* do nothing if list is non-existant */
	if (start_np == NULL)	return;

	if (!disassemble_node(start_np))	return;

	for ( np = start_np->n_next;
	      ((np == NULL)||(!NODE_IS_MARKER_NODE(np)));
	      np = np->n_next
	    ) 
	{
		if ( !disassemble_node(np) )	break;
		if (np == end_np)	break;
	}
}
/*** #endif /*DEBUG*/


get_into_comment_field(old_linelen)
	register unsigned old_linelen;      /* current length of line */
{
	register unsigned new_linelen;	/* chars (output columns) added */

	new_linelen = old_linelen;

	/* "tab" over to the comment field */
	if (new_linelen >= 40)
	{
		putc(' ', DISFILE);
		new_linelen++;
	}
	else
	{
		for (new_linelen &= (~07);  new_linelen < 40;  new_linelen += 8)
		{
			putc('\t', DISFILE);
		}
	}

	/* print the comment-introduction character */
	putc('!', DISFILE);
	new_linelen++;

	return new_linelen - old_linelen;
}


static int
print_lineno(np, printed_comment, linelen)
	register Node *np;
	register Bool printed_comment;
	register int  linelen;	/* current length of line */
{
	register int len = 0;

	/* Only print line#s if their printing is enabled, and there was
	 * an original line# associated with the node.
	 */
	if ( disassemble_with_linenums ||
	     (disassemble_with_seqnos && (optimization_level != 0))
	   )
	{
		if ( !printed_comment )
		{
			len += get_into_comment_field(linelen);
		}

		if (disassemble_with_linenums)
		{
			if (np->n_lineno == 0)
			{
				len += fprintf(DISFILE, "     ");
			}
			else	len += fprintf(DISFILE, " #%3d", np->n_lineno);
		}

		if (disassemble_with_seqnos && (optimization_level != 0))
		{
			if (np->n_seq_no == 0)
			{
				len += fprintf(DISFILE, "          ");
			}
			else	len += fprintf(DISFILE, " (seq#%3d)",
							np->n_seq_no);
		}
	}

	return len;
}


static int
print_bitset( setbits, base, nvals, first )
	register unsigned long setbits;
	register base, nvals;
	Bool first;
{
	register int r,s;
	int len = 0;
	for ( r = 0 ; r < nvals; r++ ){
		if ( setbits & ( 1<<r )){
			/* got one -- see if we can make it a range */
			for ( s = r+1; s < 32; s++ )
				if ( ! ( setbits & ( 1 << s ) ))
					break;
			/* s now contains the first bit NOT set */
			s -= 1;
			/* s now contains the high end of the r - s range */
			len += print_register(base+r, TRUE, !first, ',');
			first = FALSE;
			if ( s > r ){
				len += print_register(base+s, TRUE,TRUE, '-');
				r = s; /* NOTE MODIFICATION OF INDEX */
			}
		}
	}
	return len;
}

static int
print_a_regset( rs )
	register_set rs;
{
	int len, i;
	/*
	 * we break the abstraction here. perhaps this routine belongs
	 * in opt_regsets.c. All the other disassembly routines seem to
	 * be here, though.
	 */
	/* make up for shortcomings in print_register here */
	static struct more_registers {
		unsigned bits;			char *name; 
	} more_registers[] = {
		(1<<RAW_REGNO(RN_ICC)),		"ICC",
		(1<<RAW_REGNO(RN_FCC)),		"FCC",
		(1<<RAW_REGNO(RN_MEMORY)),	"MEM",
		0,				0
	};

	/* first, print general regs */
	len = print_bitset( rs.r[0], RN_GP(0), 32, TRUE );
	/* now print floating regs */
	len += print_bitset( rs.r[1], RN_FP(0), 32, len==0);
	/* finally, print other junk */
	len += print_bitset( rs.r[2], RN_Y, 8, len==0 );
	/* as well as condition codes and memory */
	for ( i = 0; more_registers[i].bits; i += 1 ){
		if (rs.r[2] & more_registers[i].bits){
			if (len != 0){
				fputc(',',DISFILE);
				len += 1;
			}
			fputs(more_registers[i].name,DISFILE);
			len += 3;
		}
	}
	return len;
}

int
print_regsets( np, printed_comment, linelen )
	register Node *np;
	register Bool printed_comment;
	register int linelen;
{
	register int len;
	/*
	 * Only print register sets if printing is enabled.
	 * Also, there is no point in printing this information if it was
	 * never set up, i.e. if optimization wasn't enabled.
	 */
	len = 0;
	if (disassemble_with_regsets && (optimization_level != 0)){
		if (!printed_comment)
		{
			len += get_into_comment_field(linelen);
		}
		fputs(" [", DISFILE);
		len += 2;
		if (NODE_GENERATES_CODE( np )){
			fputs("Uses: ", DISFILE);
			len += 7;
			len += print_a_regset( REGS_READ(np) );
			fputs(" Sets:", DISFILE );
			len += 6;
			len += print_a_regset( REGS_WRITTEN(np) );
			fputc(' ', DISFILE);
			len++;
		}
		fputs("Live:", DISFILE );
		len += 6;
		len += print_a_regset( REGS_LIVE(np) );
		putc(']', DISFILE);
		len++;
	}
	return len;
}


#ifdef DEBUG
static int
print_optimization_name(optno)
	register OptimizationNumber optno;
{
	register char *optname;
	register int   len;
	char huh_name[15];

	len = 0;
	fputs("OPT_", DISFILE), len += 4;

	switch( optno )
	{
	case OPT_DEADCODE:	optname = "DEADCODE";		break;
	case OPT_DEADLOOP:	optname = "DEADLOOP";		break;
	case OPT_BR2BR:		optname = "BR2BR";		break;
	case OPT_TRIVBR:	optname = "TRIVBR";		break;
	case OPT_PROLOGUE:	optname = "PROLOGUE";		break;
	case OPT_MOVEBRT:	optname = "MOVEBRT";		break;
	case OPT_NOPS:		optname = "NOPS";		break;
	case OPT_BRARBR1:	optname = "BRARBR1";		break;
	case OPT_BRARBR2:	optname = "BRARBR2";		break;
	case OPT_BRARBR3:	optname = "BRARBR3";		break;
	case OPT_BRSOONER:	optname = "BRSOONER";		break;
	case OPT_LOOPINV:	optname = "LOOPINV";		break;
	case OPT_DEADARITH:	optname = "DEADARITH";		break;
	case OPT_CONSTPROP:	optname = "CONSTPROP";		break;
	case OPT_SCHEDULING:	optname = "SCHEDULING";		break;
	case OPT_EPILOGUE:	optname = "EPILOGUE";		break;
	case OPT_LEAF:		optname = "LEAF";		break;
	case OPT_SIMP_ANNUL:	optname = "SIMP_ANNUL";		break;
	case OPT_COALESCE_RS1:	optname = "COALESCE_RS1";	break;
	case OPT_COALESCE_RS2:	optname = "COALESCE_RS2";	break;
	default:
		sprintf(&huh_name[0], "<%2d?>", optno);
		optname = &huh_name[0];
	}
	len += fprintf(DISFILE, "%s", optname);

	return len;
}
#endif /*DEBUG*/

static int
print_optimizer_notes(np, printed_comment, linelen)
	register Node *np;
	register Bool printed_comment;
	register int linelen;
{
	/* Print "notes" left by peephole optimizer in the node.
	 */

	register int len = 0;

#ifdef DEBUG
	if ( (np->n_optno != OPT_NONE) ||  np->n_internal )
#else /*DEBUG*/
	if (np->n_internal)
#endif /*DEBUG*/
	{
		/* Here, we know that we are going to print something below,
		 * so make sure that a leading '!' has been printed.
		 */
		if ( !printed_comment )
		{
			len += get_into_comment_field(linelen);
		}
		putc(' ', DISFILE);
		len++;
	}

#ifdef DEBUG
	if ( np->n_optno != OPT_NONE )
	{
		fputc('[', DISFILE), len ++;
		len += print_optimization_name(np->n_optno);
		fputc(']', DISFILE), len++;
	}
#endif /*DEBUG*/

	if ( np->n_internal )
	{
#ifdef DEBUG
		if ( np->n_optno == OPT_NONE )
		{
			switch ( np->n_opcp->o_func.f_group )
			{
			case FG_MARKER:
				fputs("[marker node]", DISFILE);
				len += 16;
				break;

			case FG_NOP:
				fputs("[assembler-generated]", DISFILE);
				len += 23;
				break;
#  ifdef CHIPFABS
			case FG_FLOAT2:
				switch (np->n_opcp->o_func.f_subgroup)
				{
				case FSG_FMOV:
				case FSG_FCMP:
					/* for assembler-inserted FMOV/FCMP's */
					fputs("[FPU-fab-workaround]", DISFILE);
					len += 25;
					break;
				default:
					fputs("[internal]", DISFILE);
					len += 13;
				}
				break;
#  endif /*CHIPFABS*/
			default:
				fputs("[internal]", DISFILE);
				len += 13;
			}
		}
#else /*DEBUG*/
		fputs("[internal]", DISFILE), len += 10;
#endif /*DEBUG*/
	}

	return len;
}


Bool
disassemble_node(np)
	register Node *np;
{
	register int len;
	register int tmp_len;
	register Bool printed_comment = FALSE;
#ifdef DEBUG
	Bool SAVED_values_used;
	Bool SAVED_disassemble_with_comments;
	Bool SAVED_disassemble_with_linenums;
	Bool SAVED_disassemble_with_regsets;
	Bool SAVED_disassemble_with_seqnos;
#endif /*DEBUG*/

	if (np == NULL)
	{
		fputs("\t! [NULL node pointer!]\n", DISFILE);
		error_count++;
		return FALSE;
	}
	else if (np->n_opcp == NULL)
	{
		fputs("\t! [NULL opcode pointer in node!]", DISFILE);
		(void)print_lineno(np, printed_comment, 40);
		putc('\n', DISFILE);
		error_count++;
		return FALSE;
	}

#ifdef DEBUG
	/* If we are disassembling, but a node list was built and we are not
	 * yet in the Assembly pass, then this routine is probably being called
	 * by a debugger (e.g. DBX).  In this case, make sure all disassembly
	 * options are on.
	 */
	if ((assembly_mode == ASSY_MODE_BUILD_LIST) && (pass != PASS_ASSEMBLE))
	{
		SAVED_disassemble_with_comments = disassemble_with_comments;
		SAVED_disassemble_with_linenums = disassemble_with_linenums;
		SAVED_disassemble_with_regsets  = disassemble_with_regsets;
		SAVED_disassemble_with_seqnos   = disassemble_with_seqnos;
		disassemble_with_comments = TRUE;
		disassemble_with_linenums = TRUE;
		if ( do_opt[OPT_DEADARITH] || do_opt[OPT_CONSTPROP] )
		{
			disassemble_with_regsets  = TRUE;
		}
		if ( do_opt[OPT_LOOPINV] )
		{
			disassemble_with_seqnos   = TRUE;
		}
		SAVED_values_used = TRUE;
	}
	else	SAVED_values_used = FALSE;
#endif /*DEBUG*/

	switch ( np->n_opcp->o_func.f_group )
	{
	case FG_MARKER:		len = disassemble_MARKER(np);		break;
	case FG_SYM_REF:	len = disassemble_SYM_REF(np);		break;
	case FG_NONE:		len = disassemble_NONE(np);		break;

	case FG_COMMENT:
		if(disassemble_with_comments)
		{
			len = disassemble_COMMENT(np);
		}
		break;

	case FG_LD:		len = disassemble_LD(np);		break;
	case FG_ST:		len = disassemble_ST(np);		break;
	case FG_LDST:		len = disassemble_LDST(np);		break;
	case FG_ARITH:		len = disassemble_ARITH(np);		break;
	case FG_WR:		len = disassemble_WR(np);		break;
	case FG_SETHI:		len = disassemble_SETHI(np);		break;
	case FG_BR:		len = disassemble_BR(np);		break;
	case FG_TRAP:		len = disassemble_TRAP(np);		break;
	case FG_CALL:		len = disassemble_CALL(np);		break;
	case FG_JMP:		len = disassemble_JMP(np);		break;
	case FG_JMPL:		len = disassemble_JMPL(np);		break;
	case FG_RETT:		len = disassemble_RETT(np);		break;
	case FG_IFLUSH:		len = disassemble_IFLUSH(np);		break;
	case FG_UNIMP:		len = disassemble_UNIMP(np);		break;
	case FG_RD:		len = disassemble_RD(np);		break;
	case FG_FLOAT2:		len = disassemble_FLOAT2(np);		break;
	case FG_FLOAT3:		len = disassemble_FLOAT3(np);		break;
	case FG_RET:		len = disassemble_RET(np);		break;
	case FG_NOP:		len = disassemble_NOP(np);		break;
	case FG_LABEL:		len = disassemble_LABEL(np);		break;
	case FG_SET:		len = disassemble_SET(np);		break;
	case FG_EQUATE:		len = disassemble_EQUATE(np);		break;
	case FG_PS:		len = disassemble_PS(np);		break;
	case FG_CSWITCH:	len = disassemble_CSWITCH(np);		break;

	default:
		len = fprintf(DISFILE, "\t! [func group in opcode == %u?]\n",
				(unsigned int)(np->n_opcp->o_func.f_group));
		error_count++;
		return FALSE;
	}

	/* Don't bother to print anything else, unless the node disassembled
	 * into something.
	 */
	if (len > 0)
	{
		len += (tmp_len = print_lineno(np, printed_comment, len));
		printed_comment = printed_comment || (tmp_len != 0);

		len += (tmp_len = print_optimizer_notes(np, printed_comment, len));
		printed_comment = printed_comment || (tmp_len != 0);

		switch ( np->n_opcp->o_func.f_group )
		{
		case FG_MARKER:
		case FG_SYM_REF:
		case FG_COMMENT:
		case FG_NONE:
			break;	/* Don't bother to print regsets for these. */
		default:
			len += print_regsets(np, printed_comment, len);
		}

		putc('\n', DISFILE);
	}

#ifdef DEBUG
	/* If we used the SAVED values, restore them. */
	if ( SAVED_values_used )
	{
		disassemble_with_comments = SAVED_disassemble_with_comments;
		disassemble_with_linenums = SAVED_disassemble_with_linenums;
		disassemble_with_regsets  = SAVED_disassemble_with_regsets;
		disassemble_with_seqnos   = SAVED_disassemble_with_seqnos;
	}
#endif /*DEBUG*/

	return TRUE;
}


static int
disassemble_MARKER(np)
	register Node *np;
{
	register int len;

	fputs("\t.seg\t", DISFILE);
	len = 16;
	len += print_string(np->n_string, strlen(np->n_string));

	return len;
}


static int
disassemble_SYM_REF(np)
	register Node *np;
{
	register int len;

	fputs("\t!*SYM_REF ", DISFILE);
	len = 18;
	len += print_symbol_name(np->n_operands.op_symp);

	return len;
}


static int
last_display_pos(start_pos, sp)
	register int  start_pos;	/* starting position in output */
	register char *sp;
{
	register int pos;

	for (pos = start_pos;   *sp != '\0';   sp++)
	{
		switch (*sp)
		{
		case '\t':	pos = (pos+8)&(~07);	break;
		case '\n':	pos = 0;		break;
		case '\b':	--pos;			break;
		default:	pos++;			break;
		}
	}

	return pos;
}


static int
disassemble_COMMENT(np)
	register Node *np;
{
	register int len;

	putc('!', DISFILE);
	fputs(np->n_string, DISFILE);
	len = last_display_pos(1, np->n_string);

	return len;
}


static int
disassemble_NONE(np)
	register Node *np;
{
	internal_error("disassemble_NONE(): shouldn't be here; opc=\"%s\"",
			np->n_opcp->o_name);
	return 0;	/* this is never really reached */
}


char *regnames[32] =
{
	"%g0", "%g1", "%g2", "%g3", "%g4", "%g5", "%g6", "%g7",
	"%o0", "%o1", "%o2", "%o3", "%o4", "%o5", "%sp", "%o7",
	"%l0", "%l1", "%l2", "%l3", "%l4", "%l5", "%l6", "%l7",
	"%i0", "%i1", "%i2", "%i3", "%i4", "%i5", "%fp", "%i7"
};


static int
print_register(regno, print_g0, print_a_sep_char, separator_char)
	register Regno regno;
	register Bool  print_g0;	/* TRUE if should print even if %g0 */
	register Bool  print_a_sep_char;/* TRUE if should print sep. char 1st */
	register char separator_char;
{
	register int	len;

	len = 0;
	print_g0 = print_g0 || (separator_char == ',');

	if ( (regno == RN_GP(0)) && !print_g0 )
	{
		return len;
	}
	else
	{
		if (print_a_sep_char)
		{
			putc(separator_char, DISFILE);
			len++;
		}
	}

	if ( IS_A_GP_REG(regno) )
	{
		register int r = RAW_REGNO(regno);

		fputs(regnames[r], DISFILE);
		len += strlen(regnames[r]);
	}
	else if ( IS_AN_FP_REG(regno) )
	{
		len += fprintf(DISFILE, "%%f%u",(unsigned int)RAW_REGNO(regno));
	}
	else if ( IS_Y_REG(regno) )	fputs("%y",   DISFILE), len+=2;
	else if ( IS_WIM_REG(regno) )	fputs("%wim", DISFILE), len+=4;
	else if ( IS_PSR_REG(regno) )	fputs("%psr", DISFILE), len+=4;
	else if ( IS_TBR_REG(regno) )	fputs("%tbr", DISFILE), len+=4;
	else if ( IS_FSR_REG(regno) )	fputs("%fsr", DISFILE), len+=4;
	else if ( IS_FQ_REG(regno) )	fputs("%fq",  DISFILE), len+=3;
	else if ( IS_CSR_REG(regno) )	fputs("%csr", DISFILE), len+=4;
	else if ( IS_CQ_REG(regno) )	fputs("%cq",  DISFILE), len+=3;
	else if ( IS_CQ_REG(regno) )	fputs("%cq",  DISFILE), len+=3;
	else if ( IS_A_CP_REG(regno) )
	{
		len += fprintf(DISFILE, "%%c%u",(unsigned int)RAW_REGNO(regno));
	}
	/* Note: MEM should ONLY appear when this routine is called through
	 * print_regsets(), not in the actual disassembly of an instruction.
	 */
	else if ( IS_MEM_REG(regno) )	fputs("MEM",  DISFILE), len+=3;
	else internal_error("print_register(): regno = 0x%02x?", regno);

	return len;
}


static int
print_symbol_name(symp)
	register struct symbol *symp;
{
	register int len;
	register char *symname;

	len = 0;
	symname = get_dynamic_symbol_name(symp);

	fputs(symname, DISFILE);
	len += strlen(symname);

	return len;
}


static int
print_value(symp, addend, reltype, print_a_sep_char, separator_char)
	register struct symbol *symp;
	register long int addend;
	register RELTYPE reltype;
	register Bool	print_a_sep_char;
	register char separator_char;
{
	register Bool	print_a_plus = FALSE;
	register Bool	print_paren;
	register int	len;
	Bool really_print_the_sep_char = FALSE;
	char	number_string[20];

	len = 0;
	if ( print_a_sep_char )
	{
		switch (separator_char)
		{
		case ',':
			really_print_the_sep_char = TRUE;
			break;
		case '+':
			/* Don't really need to print the plus sign if the
			 * value is negative anyway.
			 */
			really_print_the_sep_char =
				((symp != NULL) || (addend >  0));
			break;
		default:
			really_print_the_sep_char =
				((symp != NULL) || (addend != 0));
		}

		if (really_print_the_sep_char)
		{
			putc(separator_char, DISFILE);
			len++;
		}
	}

	switch (reltype)
	{
	case REL_LO10:
		fputs("%lo(", DISFILE);
		len += 4;
		print_paren=TRUE;
		break;

	case REL_HI22:
		fputs("%hi(", DISFILE);
		len += 4;
		print_paren=TRUE;
		break;
	default:
		print_paren=FALSE;
		break;
	}

	if (symp != NULL)
	{
		len += print_symbol_name(symp);
		print_a_plus = TRUE;
	}


	number_string[0] = '\0';

	if (addend < 0)
	{
		if (addend < -1024)
		{
			/* Hex format doesn't include its own "+/-" sign,
			 * so we negate the number and insert a "-" sign.
			 */
			sprintf(number_string, "-0x%lx", -(addend));
		}
		else
		{
			/* Decimal format will include the "-" sign */
			sprintf(number_string, "%ld",   addend);
		}
	}
	else if (addend > 0)
	{
		if (print_a_plus)	putc('+', DISFILE), len++;

		if (addend > 1023)	sprintf(number_string, "0x%lx", addend);
		else			sprintf(number_string, "%ld",   addend);
	}
	else /*addend==0*/
	    if ( (symp == NULL) &&
		 ( (!print_a_sep_char) ||
		   really_print_the_sep_char ||
	           (separator_char != '+')
		 )
	       )
	{
		number_string[0] = '0';
		number_string[1] = '\0';
	}

	if (number_string[0] != '\0')
	{
		fputs(number_string, DISFILE);
		len += strlen(number_string);
	}

	if (print_paren)	putc(')', DISFILE), len++;

	return len;
}


static int
print_rs1_and_const13(operp, separator_char)
	register struct operands *operp;
	register char separator_char;
{
	register Bool	print_a_sep_char;
	register int	len;

	len = print_register(operp->op_regs[O_RS1], FALSE, FALSE,
			     separator_char);
	print_a_sep_char = len > 0;

	len += print_value(operp->op_symp, operp->op_addend, operp->op_reltype,
			   print_a_sep_char, separator_char);
	
	return len;
}


static int
print_rs1_and_rs2(operp, separator_char)
	register struct operands *operp;
	register char separator_char;
{
	register Bool	print_a_sep_char;
	register int	len;

	len = print_register(operp->op_regs[O_RS1], FALSE, FALSE,
			     separator_char);
	print_a_sep_char = len > 0;

	len += print_register(operp->op_regs[O_RS2], !print_a_sep_char,
			print_a_sep_char, separator_char);

	return len;
}


static int
print_sfa_address(operp)
	register struct operands *operp;
{
	register int len;

	fputs("%ad(", DISFILE);
	len = 4;
	len += print_value(operp->op_symp, operp->op_addend, operp->op_reltype, FALSE, BADCHAR);
	putc(')', DISFILE);
	len++;

	return len;
}


static int
print_source(np)
	register Node *np;
{
	if (np->n_operands.op_imm_used)
	{
		return print_rs1_and_const13( &(np->n_operands), '+');
	}
	else	return print_rs1_and_rs2( &(np->n_operands), '+');
}


static int
print_second_operand( operp )
	register struct operands *operp;
{
	if (operp->op_imm_used)
	{
		return print_value( operp->op_symp, operp->op_addend,
				    operp->op_reltype, FALSE, BADCHAR);
	}
	else	return print_register(operp->op_regs[O_RS2],
					TRUE, FALSE, BADCHAR);
}


static int
disassemble_mem_address(np)
	register Node *np;
{
	register int len;

	putc('[', DISFILE);
	len = 1;

	if (np->n_operands.op_reltype == REL_SFA)
	{
		len += print_sfa_address( &(np->n_operands) );
	}
	else	len += print_source(np);

	putc(']', DISFILE);
	len++;

	if (np->n_operands.op_asi_used)
	{
		len += fprintf(DISFILE, "%u", np->n_operands.op_asi);
	}

	return len;
}


static void
print_opcode(np)
	register Node *np;
{
	putc('\t', DISFILE);
	fputs(np->n_opcp->o_name, DISFILE);
}


static int
disassemble_LD(np)
	register Node *np;
{
	register int len;

	if ( *np->n_opcp->o_name == '*' )
	{
		/* It must be one of the floating-point LD instructions. */

		putc('\t', DISFILE);
		switch (np->n_opcp->o_func.f_rd_width)
		{
		case 4:
			if(np->n_opcp->o_func.f_ldst_alt) fputs("lda",DISFILE);
			else	fputs("ld",DISFILE);
			break;
		case 8:
			if(np->n_opcp->o_func.f_ldst_alt) fputs("ldda",DISFILE);
			else if (np->n_opcp == opc_lookup("*ld2")) fputs("ld2",DISFILE);
			else fputs("ldd",DISFILE);
			break;
#ifdef DEBUG
		default:
			internal_error("disassemble_LD(): width = %d?",
					(int)(np->n_opcp->o_func.f_rd_width));
			/*NOTREACHED*/
#endif
		}
	}
	else	print_opcode(np);

	putc('\t', DISFILE);
	len = 16;

	len += disassemble_mem_address(np);

	putc(',', DISFILE);
	len++;

	len += print_register(np->n_operands.op_regs[O_RD],TRUE,FALSE,BADCHAR);

	return len;
}


static int
disassemble_ST(np)
	register Node *np;
{
	register int len;

	if ( *np->n_opcp->o_name == '*' )
	{
		/* It must be one of the floating-point ST instructions. */

		putc('\t', DISFILE);
		switch (np->n_opcp->o_func.f_rd_width)
		{
		case 4:
			if(np->n_opcp->o_func.f_ldst_alt) fputs("sta",DISFILE);
			else	fputs("st",DISFILE);
			break;
		case 8:
			if(np->n_opcp->o_func.f_ldst_alt) fputs("stda",DISFILE);
			else if (np->n_opcp == opc_lookup("*st2")) fputs("st2",DISFILE);
			else	fputs("std",DISFILE);
			break;
#ifdef DEBUG
		default:
			internal_error("disassemble_ST(): width = %d?",
					(int)(np->n_opcp->o_func.f_rd_width));
			/*NOTREACHED*/
#endif
		}
	}
	else	print_opcode(np);

	putc('\t', DISFILE);
	len = 16;

	len += print_register(np->n_operands.op_regs[O_RD],TRUE,FALSE,BADCHAR);

	putc(',', DISFILE);
	len++;

	len += disassemble_mem_address(np);

	return len;
}


static int
disassemble_LDST(np)
	register Node *np;
{
	return disassemble_LD(np);
}


static int
disassemble_RET(np)
	register Node *np;
{
	print_opcode(np);
	return 11;
}


static int
disassemble_NOP(np)
	register Node *np;
{
	print_opcode(np);
	return 11;
}


static int
disassemble_UNIMP(np)
	register Node *np;
{
	register int len;

	print_opcode(np);
	putc('\t', DISFILE);
	len = 16;
	len += print_value(np->n_operands.op_symp, np->n_operands.op_addend,
			   np->n_operands.op_reltype, FALSE, BADCHAR );

	return len;
}


static int
disassemble_ARITH(np)
	register Node *np;
{
	register int len;

	if (np->n_operands.op_imm_used)
	{
		if ( (np->n_opcp->o_func.f_subgroup == FSG_OR)	&&
		     (!(np->n_opcp->o_func.f_setscc))		&&
		     (np->n_operands.op_regs[O_RS1] == RN_GP(0))
		   )
		{
			/* We have "or %g0,const13,reg", which is more readable
			 * in ass'y language as "mov const13,reg".
			 */
			fputs("\tmov\t", DISFILE);
			len = 16;
			len += print_value(np->n_operands.op_symp,
					   np->n_operands.op_addend,
					   np->n_operands.op_reltype,
			   		   FALSE, ',');
			putc(',', DISFILE);
			len++;
			len += print_register(np->n_operands.op_regs[O_RD],
						TRUE, FALSE, BADCHAR);
		}
#ifdef TST_CONST	/* can't input this, so don't disassemble! */
		else if ( (np->n_opcp->o_func.f_subgroup == FSG_OR)	&&
			  (np->n_opcp->o_func.f_setscc)			&&
			  (np->n_operands.op_regs[O_RS1] == RN_GP(0))	&&
			  (np->n_operands.op_regs[O_RD]  == RN_GP(0))
		        )
		{
			/* We have "orcc %g0,const13,%g0", which is more
			 * readable in ass'y language as "tst const13".
			 * (Granted, this doesn't sound very useful, but we
			 *  didn't write it, we're just assembling it...)
			 */
			fputs("\ttst\t", DISFILE);
			len = 16;
			len += print_value(np->n_operands.op_symp,
					   np->n_operands.op_addend,
					   np->n_operands.op_reltype,
			   		   FALSE, BADCHAR);
		}
#endif /*TST_CONST*/
		else if ( (np->n_opcp->o_func.f_subgroup == FSG_SUB) &&
			  (np->n_opcp->o_func.f_setscc)		     &&
			  (np->n_operands.op_regs[O_RD] == RN_GP(0))
		        )
		{
			/* We have "subcc reg,const13,%g0", which is more
			 * readable in ass'y language as "cmp reg,const13".
			 */
			fputs("\tcmp\t", DISFILE);
			len = 16;
			len += print_register(np->n_operands.op_regs[O_RS1],
						TRUE, FALSE, BADCHAR);
			putc(',', DISFILE);
			len++;
			len += print_value(np->n_operands.op_symp,
					   np->n_operands.op_addend,
					   np->n_operands.op_reltype,
			   		   FALSE, ',');
		}
		else if ( ( (np->n_opcp->o_func.f_subgroup == FSG_ADD) ||
			    (np->n_opcp->o_func.f_subgroup == FSG_SUB) ) &&
			  ( np->n_operands.op_regs[O_RD] ==
			    np->n_operands.op_regs[O_RS1] ) &&
			  (np->n_operands.op_symp == NULL)
		        )
		{
			/* We have "add reg,<n>,reg", which is more
			 * readable in ass'y language as "inc <n>,reg",
			 * Or it's "sub reg,<n>,reg", which is "dec <n>,reg".
			 * If <n> is 1, we can just drop it.
			 */
			if (np->n_opcp->o_func.f_subgroup == FSG_ADD)
			{
				fputs("\tinc", DISFILE);
			}
			else	fputs("\tdec", DISFILE);

			if(np->n_opcp->o_func.f_setscc)	fputs("cc", DISFILE);
			putc('\t', DISFILE);
			len = 16;
			if (np->n_operands.op_addend != 1)
			{
				len += print_value(NULL,
						   np->n_operands.op_addend,
						   np->n_operands.op_reltype,
						   FALSE, ',');
				putc(',', DISFILE);
				len++;
			}
			len += print_register(np->n_operands.op_regs[O_RS1],
						TRUE, FALSE, BADCHAR);
		}
		else
		{
			print_opcode(np);
			putc('\t', DISFILE);
			len = 16;
			len += print_rs1_and_const13( &(np->n_operands), ',');
			putc(',', DISFILE);
			len++;
			len += print_register(np->n_operands.op_regs[O_RD],
						TRUE, FALSE, BADCHAR);
		}
	}
	else	/* a register, not an immediate value, was used for RS2. */
	{
		if ( (np->n_opcp->o_func.f_subgroup == FSG_OR)	&&
		     (!(np->n_opcp->o_func.f_setscc))		&&
		     (np->n_operands.op_regs[O_RS1] == RN_GP(0))
		   )
		{
			/* We have "or %g0,regS,regD", which is more readable
			 * in ass'y language as "mov regS,regD".
			 */
			fputs("\tmov\t", DISFILE);
			len = 16;
			len += print_register(np->n_operands.op_regs[O_RS2],
						TRUE, FALSE, BADCHAR);
			putc(',', DISFILE);
			len++;
			len += print_register(np->n_operands.op_regs[O_RD],
						TRUE, FALSE, BADCHAR);
		}
		else if ( (np->n_opcp->o_func.f_subgroup == FSG_OR)   &&
			  (np->n_opcp->o_func.f_setscc)		      &&
			  (np->n_operands.op_regs[O_RS1] == RN_GP(0)) &&
			  (np->n_operands.op_regs[O_RD]  == RN_GP(0))
		        )
		{
			/* We have "orcc %g0,reg,%g0", which is more
			 * readable in ass'y language as "tst reg".
			 */
			fputs("\ttst\t", DISFILE);
			len = 16;
			len += print_register(np->n_operands.op_regs[O_RS2],
						TRUE, FALSE, BADCHAR);
		}
		else if ( (np->n_opcp->o_func.f_subgroup == FSG_SUB) &&
			  (np->n_opcp->o_func.f_setscc)		     &&
			  (np->n_operands.op_regs[O_RD] == RN_GP(0))
		        )
		{
			/* We have "subcc reg1,reg2,%g0", which is more
			 * readable in ass'y language as "cmp reg1,reg2".
			 */
			fputs("\tcmp\t", DISFILE);
			len = 16;
			len += print_register(np->n_operands.op_regs[O_RS1],
						TRUE, FALSE, BADCHAR);
			putc(',', DISFILE);
			len++;
			len += print_register(np->n_operands.op_regs[O_RS2],
						TRUE, FALSE, BADCHAR);
		}
		else if ( ( (np->n_opcp->o_func.f_subgroup == FSG_SAVE) ||
		       (np->n_opcp->o_func.f_subgroup == FSG_REST))
					&&
		     ( (np->n_operands.op_regs[O_RS1] == RN_GP(0)) &&
		       (np->n_operands.op_regs[O_RS2] == RN_GP(0)) &&
		       (np->n_operands.op_regs[O_RD]  == RN_GP(0)) )
		   )
		{
			/* Then we have the degenerate case of a Save or
			 * Restore (all regs == %g0), and don't need to print
			 * ANY of them.
			 */
			print_opcode(np);
			len = 15;
		}
		else
		{
			print_opcode(np);
			putc('\t', DISFILE);
			len = 16;
			len += print_rs1_and_rs2( &(np->n_operands), ',');
			putc(',', DISFILE);
			len++;
			len += print_register(np->n_operands.op_regs[O_RD],
						TRUE, FALSE, BADCHAR);
		}
	}

	return len;
}


static int
disassemble_WR(np)
	register Node *np;
{
	register int len;

	fputs("\twr\t", DISFILE);
	len = 16;

	if (np->n_operands.op_imm_used)
	{
		len += print_rs1_and_const13( &(np->n_operands), ',');
	}
	else	len += print_rs1_and_rs2( &(np->n_operands), ',');

	putc(',', DISFILE);
	len++;

	switch (np->n_opcp->o_func.f_subgroup)
	{
	case FSG_WRY:	len+=print_register(RN_Y,  FALSE,FALSE,BADCHAR); break;
	case FSG_WRPSR:	len+=print_register(RN_PSR,FALSE,FALSE,BADCHAR); break;
	case FSG_WRWIM:	len+=print_register(RN_WIM,FALSE,FALSE,BADCHAR); break;
	case FSG_WRTBR:	len+=print_register(RN_TBR,FALSE,FALSE,BADCHAR); break;
	}

	return len;
}


static int
disassemble_SETHI(np)
	register Node *np;
{
	register int len;

	print_opcode(np);
	putc('\t', DISFILE);
	len = 16;
	len += print_value(np->n_operands.op_symp, np->n_operands.op_addend,
			   np->n_operands.op_reltype, FALSE, BADCHAR);
	putc(',', DISFILE);
	len++;
	len += print_register(np->n_operands.op_regs[O_RD],  TRUE, FALSE);

	return len;
}


static int
disassemble_BR(np)
	register Node *np;
{
	register int len;

	print_opcode(np);
	putc('\t', DISFILE);
	len = 16;
	len += print_value(np->n_operands.op_symp, np->n_operands.op_addend,
			   np->n_operands.op_reltype, FALSE, BADCHAR);

	return len;
}


static int
disassemble_TRAP(np)
	register Node *np;
{
	register int len;

	print_opcode(np);
	putc('\t', DISFILE);
	len = 16;
	len += print_source(np);

	return len;
}


static int
disassemble_CALL(np)
	register Node *np;
{
	register int len;

	print_opcode(np);
	putc('\t', DISFILE);
	len = 16;
	len += print_value(np->n_operands.op_symp, np->n_operands.op_addend,
			   np->n_operands.op_reltype, FALSE, BADCHAR);
	if (np->n_operands.op_call_oreg_count_used)
	{
		putc(',', DISFILE), len++;
		len += print_value(NULL,
				(long int)(np->n_operands.op_call_oreg_count),
				REL_NONE, FALSE, ',');
	}

	return len;
}


static int
disassemble_JMP(np)
	register Node *np;
{
	register int len;

	if ( np->n_operands.op_regs[O_RD] == RN_GP(0) )
	{
		/* "jmp  %i7+8" is better-known as a "ret" pseudo-instruction,
		 * and "jmp  %o7+8" is better-known as a "retl" pseudo-
		 * instruction.
		 */
		if ( ( (np->n_operands.op_regs[O_RS1] == RN_GP(15)/*%o7*/) ||
		       (np->n_operands.op_regs[O_RS1] == RN_GP(31)/*%i7*/)
		     ) &&
		     np->n_operands.op_imm_used &&
		     (np->n_operands.op_addend == 8)
		   )
		{
			if (np->n_operands.op_regs[O_RS1] == RN_GP(31)/*%i7*/)
			{
				fputs("\tret", DISFILE);
				len = 11;
			}
			else
			{
				fputs("\tretl", DISFILE);
				len = 12;
			}
		}
		else	len = disassemble_TRAP(np);  /* should work just fine */
	}
	else	internal_error("disassemble_JMPL(): rd!=%%g0?");

	return len;
}


static int
disassemble_JMPL(np)
	register Node *np;
{
	register int len;

	if ( np->n_operands.op_regs[O_RD] == RN_GP(0) )
	{
		internal_error("disassemble_JMPL(): rd==%%g0?");
		/*NOTREACHED*/
	}
	else
	{
		if (np->n_operands.op_regs[O_RD] == RN_GP(15)/*%o7*/)
		{
			/* "jmpl <whatever>,%o7" is easier to read as
			 * "call <whatever>, even though it doesn't use a CALL
			 * instruction in hardware.
			 */
			fputs("\tcall\t", DISFILE);
			len = 16;
			len += print_source(np);
		}
		else
		{
			len = disassemble_TRAP(np);  /* should work just fine */

			putc(',', DISFILE);
			len++;
			len += print_register(np->n_operands.op_regs[O_RD],
						TRUE, FALSE, BADCHAR);
		}
		if (np->n_operands.op_call_oreg_count_used)
		{
			putc(',', DISFILE), len++;
			len += print_value(NULL,
				(long int)(np->n_operands.op_call_oreg_count),
				REL_NONE, FALSE, ',');
		}
	}

	return len;
}


static int
disassemble_RETT(np)
	register Node *np;
{
	return disassemble_TRAP(np);	/* should work just fine */
}


static int
disassemble_IFLUSH(np)
	register Node *np;
{
	return disassemble_TRAP(np);	/* should work just fine */
}


static int
disassemble_RD(np)
	register Node *np;
{
	register int len;

	fputs("\trd\t", DISFILE);
	len = 16;

	switch (np->n_opcp->o_func.f_subgroup)
	{
	case FSG_RDY:	len+=print_register(RN_Y,  FALSE,FALSE,BADCHAR); break;
	case FSG_RDPSR:	len+=print_register(RN_PSR,FALSE,FALSE,BADCHAR); break;
	case FSG_RDWIM:	len+=print_register(RN_WIM,FALSE,FALSE,BADCHAR); break;
	case FSG_RDTBR:	len+=print_register(RN_TBR,FALSE,FALSE,BADCHAR); break;
	}

	putc(',', DISFILE);
	len++;
	len += print_register(np->n_operands.op_regs[O_RD], TRUE,FALSE,BADCHAR);

	return len;
}


static int
disassemble_FLOAT2(np)
	register Node *np;
{
	register Regno reg1, reg2;
	register int len;

	if (np->n_opcp->o_func.f_subgroup == FSG_FCMP)
	{
		reg1 = np->n_operands.op_regs[O_RS1];
		reg2 = np->n_operands.op_regs[O_RS2];
	}
	else
	{
		reg1 = np->n_operands.op_regs[O_RS2];
		reg2 = np->n_operands.op_regs[O_RD];
	}

	print_opcode(np);
	putc('\t', DISFILE);
	len = 16;
	len += print_register(reg1, TRUE, FALSE, BADCHAR);
	putc(',', DISFILE);
	len++;
	len += print_register(reg2, TRUE, FALSE, BADCHAR);
	
	return len;
}


static int
disassemble_FLOAT3(np)
	register Node *np;
{
	register int len;

	print_opcode(np);
	putc('\t', DISFILE);
	len = 16;
	len += print_register(np->n_operands.op_regs[O_RS1],
				TRUE, FALSE, BADCHAR);
	putc(',', DISFILE);
	len++;
	len += print_register(np->n_operands.op_regs[O_RS2],
				TRUE, FALSE, BADCHAR);
	putc(',', DISFILE);
	len++;
	len += print_register(np->n_operands.op_regs[O_RD],
				TRUE, FALSE, BADCHAR);

	return len;
}


static int
print_string(sp, len)
	register char *sp;
	register STRLEN len;
{
	register int outlen;

	putc('"', DISFILE);
	outlen = 1;

	for (   ;   len > 0;   --len, sp++)
	{
		switch (*sp)
		{
		case '\0':	fputs("\\0", DISFILE), outlen+=2;	break;
		case '\b':	fputs("\\b", DISFILE), outlen+=2;	break;
		case '\f':	fputs("\\f", DISFILE), outlen+=2;	break;
		case '\n':	fputs("\\n", DISFILE), outlen+=2;	break;
		case '\r':	fputs("\\r", DISFILE), outlen+=2;	break;
		case '\t':	fputs("\\t", DISFILE), outlen+=2;	break;
		case '\'':	fputs("\\`", DISFILE), outlen+=2;	break;
		case '"':	fputs("\\\"",DISFILE), outlen+=2;	break;
		case '\\':	fputs("\\\\",DISFILE), outlen+=2;	break;
		default:
			if ( (*sp < ' ') || (*sp > 0x7f) )
			{
				fprintf(DISFILE, "\\%03o", *sp);
				outlen += 4;
			}
			else	putc(*sp, DISFILE), outlen++;
		}
	}
	putc('"', DISFILE);
	outlen++;

	return outlen;
}


static int
disassemble_LABEL(np)
	register Node *np;
{
	register int len;

	len = 0;

	if ( BIT_IS_ON(np->n_operands.op_symp->s_attr, SA_GLOBAL) )
	{
		fputs("\t.global\t", DISFILE);
		(void)print_symbol_name(np->n_operands.op_symp);
		putc('\n', DISFILE);
	}

	len += print_symbol_name(np->n_operands.op_symp);
	putc(':', DISFILE), len++;
	
	return len;
}


static int
disassemble_SET(np)
	register Node *np;
{
	register int len;

	print_opcode(np);
	putc('\t', DISFILE);
	len = 16;
	len += print_value(np->n_operands.op_symp, np->n_operands.op_addend,
			   np->n_operands.op_reltype, FALSE, BADCHAR);
	putc(',', DISFILE);
	len++;
	len += print_register(np->n_operands.op_regs[O_RD],
				TRUE, FALSE, BADCHAR);

	return len;
}


static int
disassemble_EQUATE(np)
	register Node *np;
{
	register int len;

	len = print_symbol_name(np->n_operands.op_symp);
	fputs("\t=\t", DISFILE);
	len = (len+7) & ~07;
	len += 8;
	if (np->n_operands.op_symp->s_def_node2 == NULL)
	{
		/* Just a simple absolute value... */
		len += print_value(NULL, np->n_operands.op_addend,
			np->n_operands.op_reltype, FALSE, BADCHAR);
	}
	else
	{
		/* The symbol was equated to an expression;
		 * print the text of the expression.
		 */
		len += print_value(
			np->n_operands.op_symp->s_def_node2->n_operands.op_symp,
			np->n_operands.op_addend,
			np->n_operands.op_reltype, FALSE, BADCHAR );
	}

	return len;
}


static int
disassemble_PS_OPS(np)
	register Node *np;
{
	register int len;

	switch (np->n_opcp->o_func.f_subgroup)
	{
	case FSG_ALIGN:		/* ".align"      */
	case FSG_SKIP:		/* ".skip"      */
	case FSG_PROC:		/* ".proc"      */
		print_opcode(np);
		putc('\t', DISFILE);
		len = 16;
		len += fprintf(DISFILE, "%lu", np->n_operands.op_addend);
		break;

	case FSG_NOALIAS:		/* ".noalias"      */
		print_opcode(np);
		putc('\t', DISFILE);
		len = 16;
		len += print_register(np->n_operands.op_regs[O_RS1],
					TRUE, FALSE, BADCHAR);
		putc(',', DISFILE);
		len++;
		len += print_register(np->n_operands.op_regs[O_RS2],
					TRUE, FALSE, BADCHAR);
		break;

	case FSG_BYTE:		/* ".byte"      */
	case FSG_HALF:		/* ".half"      */
	case FSG_WORD:		/* ".word"      */
		print_opcode(np);
		putc('\t', DISFILE);
		len = 16;
		len += print_value(np->n_operands.op_symp,
				   np->n_operands.op_addend,
				   np->n_operands.op_reltype, FALSE, ',');
		break;
#ifdef DEBUG
	default:
		internal_error("disassemble_PS_OPS(): sg == %u?",
			(unsigned int)(np->n_opcp->o_func.f_subgroup) );
#endif
	}

	return len;
}


static int
disassemble_PS_STRG(np)
	register Node *np;
{
	register int len;

	switch (np->n_opcp->o_func.f_subgroup)
	{
	case FSG_SEG:		/* ".seg"	*/
	case FSG_OPTIM:		/* ".optim"	*/
		print_opcode(np);
		putc('\t', DISFILE);
		len = 16;
		len += print_string(np->n_string, strlen(np->n_string));
		break;
#ifdef DEBUG
	default:
		internal_error("disassemble_PS_STRG(): sg == %u?",
			(unsigned int)(np->n_opcp->o_func.f_subgroup) );
#endif
	}

	return len;
}


static int
disassemble_PS_VLHP(np)
	register Node *np;
{
	register struct value *vp;
	register Bool first = TRUE;
	register int len;

	switch (np->n_opcp->o_func.f_subgroup)
	{

	case FSG_ASCII:		/* ".ascii"     */
	case FSG_ASCIZ:		/* ".asciz"     */
		print_opcode(np);
		putc('\t', DISFILE);
		len = 16;
		vp = np->n_vlhp->vlh_head;
		len += print_string(vp->v_strptr, vp->v_strlen);
		break;

	case FSG_COMMON:	/* ".common"    */
	case FSG_RESERVE:	/* ".reserve"   */
		print_opcode(np);
		if (np->n_opcp->o_func.f_subgroup == FSG_RESERVE)
		{
			putc(' ',  DISFILE); /* 'cause ".reserve" is >7 chars */
			len = 17;
		}
		else
		{
			putc('\t', DISFILE);
			len = 16;
		}
		len += print_symbol_name(np->n_vlhp->vlh_head->v_symp);
		putc(',', DISFILE);
		len++;
		len += fprintf(DISFILE, "%lu",
				np->n_vlhp->vlh_head->v_next->v_value);
		/* The "segment" argument is optional for ".common" and
		 * ".reserve"; print it only if it was present in the input.
		 */
		if ( np->n_vlhp->vlh_listlen > 2 )
		{
			putc(',', DISFILE);
			len++;
			vp = np->n_vlhp->vlh_head->v_next->v_next;
			len += print_string(vp->v_strptr, vp->v_strlen);
		}
		if ( (np->n_opcp->o_func.f_subgroup == FSG_RESERVE) &&
		     (np->n_vlhp->vlh_listlen > 3 )
		   )
		{
			/* The alignment argument */
			putc(',', DISFILE);
			len++;
			len += fprintf(DISFILE, "%lu",
					np->n_vlhp->vlh_head->v_next
						->v_next->v_next->v_value);
		}
		break;

	case FSG_STAB:		/* ".stab[dns]" */
		print_opcode(np);
		putc('\t', DISFILE);
		len = 16;

		vp = np->n_vlhp->vlh_head;

		if ( np->n_opcp->o_igroup/*(the arg count)*/ == 5 )
		{
			/* It's a STABS; the first operand is a string. */
			len += print_string(vp->v_strptr, vp->v_strlen);
			vp = vp->v_next;
			putc(',', DISFILE), len++;
		}

		len += print_value(vp->v_symp, vp->v_value, REL_NONE,FALSE,',');
		vp = vp->v_next;

		len += print_value(vp->v_symp, vp->v_value, REL_NONE, TRUE,',');
		vp = vp->v_next;

		len += print_value(vp->v_symp, vp->v_value, REL_NONE, TRUE,',');

		switch ( np->n_opcp->o_igroup/*(the arg count)*/ )
		{
		case 4:	/* STABN */
		case 5: /* STABS */
			vp = vp->v_next;
			len += print_value(vp->v_symp, vp->v_value, REL_NONE,
						TRUE, ',');
			break;
		default:
			break;	/* do nothing */
		}

		break;

	case FSG_SGL:		/* ".single"    */
	case FSG_DBL:		/* ".double"    */
	case FSG_QUAD:		/* ".quad"      */
		print_opcode(np);
		putc('\t', DISFILE);
		len = 16;
		for ( vp = np->n_vlhp->vlh_head;  vp != NULL;  vp = vp->v_next)
		{
			if (first)	first = FALSE;
			else	putc(',', DISFILE), len++;
			fputs("0r", DISFILE);
			len++;
			fputs(vp->v_strptr, DISFILE);
			len += strlen(vp->v_strptr);
		}
		break;
#ifdef DEBUG
	default:
		internal_error("disassemble_PS_VLHP(): sg == %u?",
			(unsigned int)(np->n_opcp->o_func.f_subgroup) );
#endif
	}

	return len;
}


static int
disassemble_PS_FLP(np)
	register Node *np;
{
	register int len;

	switch (np->n_opcp->o_func.f_subgroup)
	{
	case FSG_BOF:		/* "*.bof" (internal: begin of file) */
		if (disassemble_with_BOF)
		{
			putc('!', DISFILE);
			print_opcode(np);
			putc('\t', DISFILE);
			len = 16;
			len += print_string(np->n_flp->fl_fnamep,
					    strlen(np->n_flp->fl_fnamep));
		}
		else len = 0;
		break;
#ifdef DEBUG
	default:
		internal_error("disassemble_PS_FLP(): sg == %u?",
			(unsigned int)(np->n_opcp->o_func.f_subgroup) );
#endif
	}

	return len;
}


static int
disassemble_PS_NONE(np)
	register Node *np;
{
	switch (np->n_opcp->o_func.f_subgroup)
	{
	case FSG_EMPTY:		/* ".empty"     */
	case FSG_ALIAS:		/* ".alias"     */
		print_opcode(np);
		return 14;
#ifdef DEBUG
	default:
		internal_error("disassemble_PS_NONE(): sg == %u?",
			(unsigned int)(np->n_opcp->o_func.f_subgroup) );
		/*NOTREACHED*/
#endif
	}
}


static int
disassemble_PS(np)
	register Node *np;
{
	register int len;

	switch (np->n_opcp->o_func.f_node_union)
	{
	case NU_NONE:	len = disassemble_PS_NONE(np);	break;
	case NU_FLP:	len = disassemble_PS_FLP(np);	break;
	case NU_VLHP:	len = disassemble_PS_VLHP(np);	break;

	case NU_OPS:	len = disassemble_PS_OPS(np);	break;

	case NU_STRG:	len = disassemble_PS_STRG(np);	break;
#ifdef DEBUG
	default:
		internal_error("disassemble_PS(): f_node_union == %u?",
			(unsigned int)(np->n_opcp->o_func.f_node_union) );
#endif
	}

	return len;
}


static int
disassemble_CSWITCH(np)
	register Node *np;
{
	register int len;

	fputs("\t.word\t", DISFILE);
	len = 16;
	len += print_value(np->n_operands.op_symp, np->n_operands.op_addend,
			   np->n_operands.op_reltype, FALSE, ',');
	
	return len;
}
