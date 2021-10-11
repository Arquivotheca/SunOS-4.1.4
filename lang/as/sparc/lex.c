static char sccs_id[] = "@(#)lex.c	1.1\t10/31/94";
#include <stdio.h>
#include <sys/wait.h>
#include <sys/file.h>
#include "structs.h"
#include "sym.h"
#include "y.tab.h"
#include "bitmacros.h"
#include "errors.h"
#include "instructions.h"
#include "globals.h"
#include "lex.h"
#include "alloc.h"
#include "node.h"
#include "scratchvals.h"
#include "disassemble.h"
#include "lex_tables.h"
#include "localsyms.h"
#include "opcode_ptrs.h"
#include "registers.h"

#define INCHAR_MASK  0xff	/* AND all input chars with this mask	*/
/* must coordinate these "LEX" values with the char_fcn_table array!*/
#define LEXCH_EOF	(0xff)	/* same as lower 8 bits of EOF !!	*/
#define LEXCH_BOI	(0xfe)	/* "beginning-of-input" char		*/
#define LEXCH_BOF	(0xfd)	/* "beginning-of-input-file" char	*/
#define LEXCH_BOL	(0xfc)	/* "beginning-of-input-line" char	*/
#define LEXCH_BOS	(0xfb)	/* "beginning-of-statement" char	*/
#define LEXCH_NULL	(0xf0)
#if LEXCH_EOF != (EOF & INCHAR_MASK)	/* check the assumption */
	@error@
#endif

#define SCRATCHBUFLEN	2048	/* length of buffer for strings and symbol */
				/*	names from an input line.	   */
#define MAXFILENAME	256	/* max length of a retained file name	   */

static STDIOCHAR nxtc_tmp;	/* used only by nextchar(), as a temp value*/

/* note that nextchar() is the "safe" input-char routine to use;
 * raw_nextchar() can ONLY be used if it is guaranteed that there are
 * no input characters pushed back with unnextchar().
 * I.e., there have been no intervening unnextchar()'s since the last
 * nextchar().
 */
#define INPUT_STMT

#ifdef DEBUG
   /* save each input line, so we can print it at a debugger breakpoint. */
#  define MAX_SAVED_INPUT_LINE_LEN 4096
   char input_line[MAX_SAVED_INPUT_LINE_LEN+1];	/*MAX chars + null terminator*/
   char *ilp;			/* pointer to next char of input_line[] */
#  define INPUT_LINE	(*ilp++)=
#else
#  define INPUT_LINE
#endif

#define raw_nextchar()  ( (INPUT_STMT INPUT_LINE getc(currifp)) & INCHAR_MASK)

#define nextchar()				\
	((peekc==LEXCH_NULL)			\
		?raw_nextchar()			\
		:(nxtc_tmp=peekc, peekc=LEXCH_NULL, nxtc_tmp))

#define unnextchar(c)	(peekc = c)


extern YYSTYPE yylval;	/* for return value to YACC */


	/* scratch buffer;  reset at the beginning of each input statement */
static	char scratch_buf[SCRATCHBUFLEN];
static	char *scratchp;	/* ptr to next available char in "scratch_buf" */

static	char *pptmpfilename = NULL;/*temporary filename in use (for cleanup())*/

static char *StringsTooLong =
		"symbols+strings too long (>%d chars) in statement";

static Bool opcode_expected = FALSE;	/* TRUE when next symbol expected */
					/*	is opcode mnemonic	  */
static Bool lexdebug = FALSE;		/* debug flag	*/

static int paren_level;	/* depth of parens in a statement	*/

static FILE *currifp;	/* current input-file ptr; set by cf_boi()&cf_eof() */

static STDIOCHAR peekc;	/* lexical analyzer lookahead character	*/
			/* (initialized in lex_init)		*/

struct filelist *curr_flp = NULL;/* ptr to current file-list structure*/

/* forward routine declarations */
struct value *formstring();
char *collect_sym();
char *collect_fpnum();
void close_input_file();
void unlink_pp_tmp_file();
static void new_statement();
TOKEN	cf_bad(), cf_wht(), cf_ltr(), cf_dig(), cf_eos(), cf_cmt(), cf_quo();
TOKEN	cf_pls(), cf_min(), cf_mul(), cf_div(), cf_lp(),  cf_rp(),  cf_bad();
TOKEN   cf_and(), cf_or(), cf_xor(), cf_bnt();
TOKEN	cf_boi(), cf_bof(), cf_bol(), cf_bos(), cf_eof();
TOKEN	cf_pct(), cf_ID(),  cf_shf(), cf_eq();
char *get_scratch_buf();

/*
 * char_fcn_table is a table of pointers to functions, one of which will
 * be executed when the first character of a new token is examined from
 * the input.
 */
TOKEN (*(char_fcn_table[]))() =
{
      /*<nul>	<soh>	<stx>	<ext>	<eot>	<enq>	<ack>	<bel>	*/
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,

      /*'\b'	'\t'	'\n'	'\f'	<np>	'\r'	<so>	<si>	*/
	cf_bad,	cf_wht,	cf_eos,	cf_wht,	cf_bad,	cf_bad,	cf_bad,	cf_bad,

      /*<dle>	<dc1>	<dc2>	<dc3>	<dc4>	<nak>	<syn>	<etb>	*/
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,

      /*<can>	<em>	<sub>	<esc>	<fs>	<gs>	<rs>	<us>	*/
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,

      /*<space>	'!'	'"'	'#'	'$'	'%'	'&'	'\''	*/
	cf_wht,	cf_cmt,	cf_quo,	cf_bad,	cf_ltr,	cf_pct,	cf_and,	cf_quo,

      /*'('	')'	'*'	'+'	','	'-'	'.'	'/'	*/
	cf_lp,	cf_rp,	cf_mul,	cf_pls,	cf_ID,	cf_min,	cf_ltr,	cf_div,

      /*'0'	'1'	'2'	'3'	'4'	'5'	'6'	'7'	*/
	cf_dig,	cf_dig,	cf_dig,	cf_dig,	cf_dig,	cf_dig,	cf_dig,	cf_dig,

      /*'8'	'9'	':'	';'	'<'	'='	'>'	'?'	*/
	cf_dig,	cf_dig,	cf_ID,	cf_eos,	cf_shf,	cf_eq,	cf_shf,	cf_bad,

      /*'@'	'A'	'B'	'C'	'D'	'E'	'F'	'G'	*/
      /*'H'	'I'	'J'	'K'	'L'	'M'	'N'	'O'	*/
      /*'P'	'Q'	'R'	'S'	'T'	'U'	'V'	'W'	*/
	cf_bad,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,
	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,
	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,

      /*'X'	'Y'	'X'	'['	'\'	']'	'^'	'_'	*/
	cf_ltr,	cf_ltr,	cf_ltr,	cf_ID,	cf_quo,	cf_ID,	cf_xor,	cf_ltr,

      /*'`'	'a'	'b'	'c'	'd'	'e'	'f'	'g'	*/
      /*'h'	'i'	'j'	'k'	'l'	'm'	'n'	'o'	*/
      /*'p'	'q'	'r'	's'	't'	'u'	'v'	'w'	*/
	cf_bad,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,
	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,
	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,	cf_ltr,

      /*'x'	'y'	'z'	'{'	'|'	'}'	'~'	<del>	*/
	cf_ltr,	cf_ltr,	cf_ltr,	cf_bad,	cf_or,	cf_bad,	cf_bnt, cf_bad,

      /* now, the upper 128 chars... */
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,
	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,	cf_bad,
	cf_bad,	cf_bad,	cf_bad,	cf_bos,	cf_bol,	cf_bof, cf_boi,	cf_eof
     /*			  	 ^-BOS   ^-BOL   ^-BOF   ^-BOI   ^-EOF	*/
};


/**************************  l e x _ i n i t  ********************************/

lex_init()
{
	sym_inittbl(&init_syms[0]);	/* init symbol table */
	opc_inittbl(&init_opcs[0]);	/* init opcode table */

	init_opcode_pointers();

	peekc = LEXCH_BOI;	/* setup initial lookahead character */
}


#ifdef DEBUG
Bool debug_tkn_count = FALSE;
long int tkn_count[512];     /* init to 0's by default */
long int lo_tkn_count[16];   /* init to 0's by default */
# define LOT_BAD	0
# define LOT_WHT	1
# define LOT_CMT	2
# define LOT_BOI	3
# define LOT_BOL	4
# define LOT_BOS	5
# define LOT_BOF1	6
# define LOT_BOF2	7
# define LOT_EOF	8
#endif

/****************************  y y l e x  ************************************/

TOKEN
yylex()
{
	register STDIOCHAR c;
	register TOKEN tkn;
	register TOKEN (*char_function)();

	do
	{
		/* fetch the first character of a new token,
		 * and dispatch the appropriate routine to process the
		 * rest of the token.
		 */

		/* MUST use nextchar() here, not raw_nextchar()!
		 * But because nextchar() is used here, the first character
		 * read in each of the cf_*() routines can safely use the
		 * faster raw_nextchar(), since there are obviously no
		 * intervening unnextchar()'s.
		 */
		char_function = char_fcn_table[(c=nextchar())];
		tkn = (*char_function)(c);
#ifdef DEBUG
		tkn_count[tkn]++;
#endif
	} while (tkn == TOK_LEXONLY);

#ifdef DEBUG
	if(lexdebug) /*** DEBUG CODE ***/
	{
		fprintf(stderr,"[yylex returns ");
		fprintf(stderr, ((tkn>=040)&&(tkn<=127)) ?"'%c'" :"%3d", tkn);
		fprintf(stderr," (0%03o), w/ yylval=0%o]\n", tkn,yylval.y_uint);
	}

	if ( ilp >= &input_line[MAX_SAVED_INPUT_LINE_LEN-1] )
	{
		internal_error("cf_eos(): input line too long (%d>%d chars)",
			(int)(ilp - &input_line[MAX_SAVED_INPUT_LINE_LEN-1]),
			MAX_SAVED_INPUT_LINE_LEN);
		/*NOTREACHED*/
	}
#endif
	return tkn;
}


TOKEN
cf_ID(c)	/* the input char is its own token value (IDentity) */
	register STDIOCHAR c;
{
	return ((TOKEN)c);
}


TOKEN
cf_bad(c)	/* invalid (bad) input char */
	register STDIOCHAR c;
{
	error(ERR_INVCHAR, input_filename, input_line_no, (int)c);
#ifdef DEBUG
	lo_tkn_count[LOT_BAD]++;
#endif
	return TOK_LEXONLY;
}


TOKEN
cf_wht()
{
	register int c;

	/* skip contiguous white space */
	while ( char_fcn_table[(c=raw_nextchar())] == cf_wht ) ;

	unnextchar(c);	/* put back the non-white char */

#ifdef DEBUG
	lo_tkn_count[LOT_WHT]++;
#endif
	return TOK_LEXONLY;
}


TOKEN
cf_ltr(ch)
	STDIOCHAR ch; /*(don't make "register": we take its address below)*/
{
	register char  *symname;
	register TOKEN	ret_token;

	/* Collect rest of symbol name, and skip any blanks after it. */
	symname = collect_sym(&ch);

	if ( (ch == ':') || (ch == '=') ) /* Char which delimited the symbol.*/
	{
		/* We have a symbol sporting a trailing ':',
		 * therefore we take it as a LABEL.
		 */

		/* Make ':'s gets gobbled, but leave '='s in input. */
		if ( ch == ':' )	ch = nextchar();

		ret_token = LABEL;
		yylval.y_cp = symname;
	}
	else if (opcode_expected)
	{
		/* In opcode field: we will look up the name in the opcode
		 * table (not the regular symbol table), and return its
		 * associated token-type.
		 */

		struct opcode *opcp;

		/* If the next char is a "," and the opcode mnemonic we have
		 * so far collected (via collect_sym()) is a Branch mnemonic,
		 * then it's probably a branch-with-Annul mnemonic.
		 */
		if ( (ch == ',') &&
		     ( ((opcp = opc_lookup(symname)) != NULL) &&
		       (opcp->o_func.f_group == FG_BR)
		     )
		   )
		{
			/* Have ',' after a branch opcode: it must be a ",a"
			 * after the mnemonic, for "branch with Annul".
			 */

			/* Get the ',' which had been unnextchar()'d by
			 * collect_sym(), and ignore it.
			 */
			nextchar();

			strcpy(scratchp, symname);
			strcat(scratchp, ",");	/* Add "," to the mnemonic. */

			/* Now get the next char; is it an 'a'? */
			if ( (ch = nextchar()) == 'a' )
			{
				/* Add "a" to the mnemonic. */
				strcat(scratchp, "a");
			}
			else
			{
				/* Put back the non-'a' char after ','.
				 * This will surely cause an unknown-opcode
				 * error, below, since no opcode mnemonics
				 * end in '.'.
				 */
				unnextchar(ch);
			}

			symname = scratchp;
		}

		/* Try to look up the opcode. */
		yylval.y_opcode = opc_lookup(symname);
		if ( SYM_NOTFOUND(yylval.y_opcode) )
		{
			/* It's an unknown opcode. */
			error(ERR_UNK_OPC,input_filename,input_line_no,symname);
			ret_token = TOK_ERROR;
		}
		else	ret_token = yylval.y_opcode->o_token;

		opcode_expected = FALSE;	/* Have opcode now. */
	}
	else
	{
		/* We have a symbol outside of opcode field, and not sporting
		 * a trailing ':': we will just return the symbol name.
		 */

		ret_token = SYMBOL;
		yylval.y_cp = symname;
	}

	return ret_token;
}


TOKEN
cf_eq()		/* have '=' from input */
{
	if (opcode_expected)
	{
		/* then '=' is the "set" opcode (setting a variable to a
		 * value); stop looking for an opcode.
		 */
		opcode_expected = FALSE;
	}
	return (TOKEN)('=');
}

/*** 
/*** 
/*** static void
/*** fold_to_lower(sp)
/*** 	register char *sp;
/*** {
/*** 	for (    ;   *sp != '\0';    sp++)
/*** 	{
/*** 		if ( (*sp >= 'A') && (*sp <= 'Z') )	*sp += 'a' - 'A';
/*** 	}
/*** } ***/

/*---------------------------------------------------------------------------*/

static unsigned int			/* used by cf_pct() */
get_reg_number(c, maxnumber)
	register STDIOCHAR  c;
	unsigned int maxnumber;
{
	register unsigned int  regno;

	regno = (c - '0');
	while ( char_fcn_table[(c=raw_nextchar())] == cf_dig )
	{
		regno = (regno * 10) + (c - '0');
	}

	unnextchar(c);  /* put back the last (unwanted) character  */

	/* no need to check for "regno < 0", since it is unsigned */
	if ( regno > maxnumber )
	{
		error(ERR_INV_REG, input_filename, input_line_no);
		regno = 0;
	}

	return regno;
}

static TOKEN				/* used by cf_pct() */
get_expected_char(expected_char, tok, value)
	register STDIOCHAR  expected_char;	/* exp. char, in lower case */
	register TOKEN tok;			/* token to return, if OK   */
	register unsigned int value;		/* value for yylval, if OK  */
{
	register STDIOCHAR ch;

	ch = raw_nextchar();
	if ( (ch == expected_char) || ((ch-'A'+'a') == expected_char))
	{
		if (tok == REG)	yylval.y_regno = value;
		else		yylval.y_uint  = value;
		return tok;
	}
	else
	{
		/* special (%) symbol not found: error. */
		unnextchar(ch);
		error(ERR_UNK_SYM, input_filename, input_line_no);
		return TOK_ERROR;
	}
}

TOKEN
cf_pct()		/* token starts with '%' */
{
	register STDIOCHAR ch;
	register TOKEN	ret_token;

	/* Originally, '%' symbols were predefined in the symbol table
	 * and this routine went through symbol-table lookup to get
	 * their token-types and values.
	 *
	 * Since '%' symbols are such high-runners, it turned out to be
	 * much faster to take them OUT of the symbol table and recognize
	 * them here.
	 *
	 * A serendipity was that the change cut 'way down on initialized
	 * data space usage, since it deleted the symbol-table entries
	 * (in lex_tables.c) for all (~105) of the "%" symbols.
	 */

	/* Note that raw_nextchar() is VERY carefully used in this routine, and
	 * in two routines which it calls (get_reg_number() and
	 * get_expected_char()).
	 *
	 * raw_nextchar() is never called again after any call to either of
	 * those routines, since both use unnextchar().
	 */

	ch = raw_nextchar();

	if (char_fcn_table[ch] == cf_ltr)
	{
		if (ch <= 'Z')	ch = ch - 'A' + 'a';	/* to lower case */

		switch (ch)
		{
		case 'a':	/* "%ad" ? */
			ret_token = get_expected_char('d', TOK_SFA_ADDR, 0);
			break;
		
		case 'c':	/* "%cNN", "%csr", or "%cq" (we hope) */
			if (optimization_level != 0)
			{
				/* Must increase "_NLI_FOR_ALLREGS" if we are
				 * going to accomodate Coprocessor instruction
				 * optimizations in the peephole optimizer...
				 */
				internal_error("cf_pct(): coprocessor instructions present -- must run WITHOUT \"-O\" option");
				/*NOTREACHED*/
			}

			if (char_fcn_table[(ch=raw_nextchar())] == cf_dig)
			{
				/* have "%cNN" register notation */
				yylval.y_regno = RN_CP(get_reg_number(ch, 31));
				ret_token = REG;
			}
			else
			{
				/* to lower case */
				if (ch <= 'Z')	ch = ch - 'A' + 'a';

				switch ( ch )
				{
				case 'q':	/* "%cq" */
					yylval.y_regno = RN_CQ;
					ret_token = REG;
					break;
				case 's':	/* "%csr" ? */
					ret_token = get_expected_char('r', REG,
							RN_CSR);
					break;
				default:
					error(ERR_UNK_SYM, input_filename,
						input_line_no);
					ret_token = TOK_ERROR;
				}
			}
			break;

		case 'f':	/* "%fp", "%fNN", "%fsr", or "%fq" (we hope) */
			if (char_fcn_table[(ch=raw_nextchar())] == cf_dig)
			{
				/* have "%fNN" register notation */
				yylval.y_regno = RN_FP(get_reg_number(ch, 31));
				ret_token = REG;
			}
			else
			{
				/* to lower case */
				if (ch <= 'Z')	ch = ch - 'A' + 'a';

				switch ( ch )
				{
				case 'p':	/* "%fp" (frame pointer) */
					yylval.y_regno = RN_GP(30);
					ret_token = REG;
					break;
				case 'q':	/* "%fq" */
					yylval.y_regno = RN_FQ;
					ret_token = REG;
					break;
				case 's':	/* "%fsr" ? */
					ret_token = get_expected_char('r', REG,
							RN_FSR);
					break;
				default:
					error(ERR_UNK_SYM, input_filename,
						input_line_no);
					ret_token = TOK_ERROR;
				}
			}
			break;

		case 'g':	/* "%gNN" */
			yylval.y_regno =
				RN_GP( 0 + get_reg_number(raw_nextchar(),7));
			ret_token = REG;
			break;

		case 'h':	/* "%hi" */
			ret_token = get_expected_char('i', TOK_HILO,
						(unsigned int)OPER_HILO_HI);
			break;

		case 'i':	/* "%iNN" */
			yylval.y_regno =
				RN_GP(24 + get_reg_number(raw_nextchar(),7));
			ret_token = REG;
			break;

		case 'l':	/* "%lo" or "%lNN" */
			if ( ((ch=raw_nextchar()) == 'o') || (ch == 'O') )
			{
				/* "%lo" */
				yylval.y_uint = (unsigned int)OPER_HILO_LO;
				ret_token = TOK_HILO;
			}
			else
			{
				/* "%lNN" */
				yylval.y_regno = RN_GP(16+get_reg_number(ch,7));
				ret_token = REG;
			}
			break;

		case 'o':	/* "%oNN" */
			yylval.y_regno =
				RN_GP( 8 + get_reg_number(raw_nextchar(),7));
			ret_token = REG;
			break;

		case 'p':	/* "%psr" ? */
			ret_token = get_expected_char('s', REG, RN_PSR);
			if ( ret_token != TOK_ERROR )
			{
				ret_token = get_expected_char('r', REG, RN_PSR);
			}
			break;

		case 'r':	/* "%rNN" */
			yylval.y_regno =
				RN_GP(get_reg_number(raw_nextchar(), 31));
			ret_token = REG;
			break;

		case 's':	/* "%sp" ? (stack pointer) */
			ret_token =get_expected_char('p', REG, RN_GP(14));
			break;

		case 't':	/* "%tbr" ? */
			ret_token = get_expected_char('b', REG, RN_TBR);
			if ( ret_token != TOK_ERROR )
			{
				ret_token = get_expected_char('r', REG, RN_TBR);
			}
			break;

		case 'w':	/* "%wim" ? */
			ret_token = get_expected_char('i', REG, RN_WIM);
			if ( ret_token != TOK_ERROR )
			{
				ret_token = get_expected_char('m', REG, RN_WIM);
			}
			break;

		case 'y':	/* "%y" */
			yylval.y_regno = RN_Y;
			ret_token = REG;
			break;

		default:
			error(ERR_UNK_SYM, input_filename, input_line_no);
			ret_token = TOK_ERROR;
		}
	}
	else
	{
		/* it was just the modulo operator (%) */
		ret_token     = TOK_MULT;
		yylval.y_uint = (unsigned int)OPER_MULT_MOD;
	}

	return  ret_token;
}

TOKEN
cf_shf(c)	/* token starts with '<' or '>' (should be a shift operator) */
	register STDIOCHAR c;
{
	register STDIOCHAR c2;
	register TOKEN	ret_token;

	if( (c2 = raw_nextchar()) == c)
	{
		ret_token     = TOK_SHIFT;
		if (c == '<')	yylval.y_uint = (unsigned int)OPER_SHIFT_LEFT;
		else		yylval.y_uint = (unsigned int)OPER_SHIFT_RIGHT;
	}
	else
	{
		error(ERR_EXPR_SYNTAX, input_filename, input_line_no);
		unnextchar(c2);
		ret_token = TOK_ERROR;
	}

	return  ret_token;
}

/*---------------------------------------------------------------------------*/

TOKEN
cf_dig(c)
	register STDIOCHAR c;
{
	register long int	number;		/* binary value of # scanned  */
	register unsigned int	new_digit;	/*    "     " of digit scanned*/
	register int		radix;		/* radix of # being scanned   */
					 /*(radix == RADIX_FP means float.pt.)*/
	register TOKEN		ret_token;
#define RADIX_FP  (-1)	/* magic-cookie radix, for floating-point number */

	if (c == '0')
	{
		c = raw_nextchar();
		if ( (c == 'x') || ( c == 'X') )
		{
			radix = 16;	/* have a hex number "0x..." */
			c = raw_nextchar();	/* read char after the 'x'*/
		}
		else if ( (c == 'r') || ( c == 'R') )
		{
			/* have a floating-point ("real") number "0r..." */
			radix = RADIX_FP;
			c = raw_nextchar();	/* read char after the 'r'*/
		}
		else	radix = 8;
	}
	else	radix = 10;


	if (radix == RADIX_FP)
	{
		register char *fpnum;

		fpnum = collect_fpnum(c);
		/* warning: can't use raw_nextchar() after call to
		 * collect_fpnum(), since it uses unnextchar().
		 */

		/* Got a F.P. number; return it as a string. */
		yylval.y_value = formstring(fpnum, strlen(fpnum));
		ret_token = FP_VALUE;
	}
	else
	{
		for ( number = 0;
		      (new_digit = numbers[c]) != NOTNUM;
		      c = raw_nextchar()
		    )
		{
			if(new_digit >= radix)
			{
				if ( (number >= 0) && (number <= 9) &&
				     ((c == 'b') || (c == 'f'))
				   )
				{
					/* then we have a ref to "Nf" or "Nb",*/
					/* which will be handled below	      */
				}
				else	/* actually have an error */
				{
					error(ERR_INVDIGIT, input_filename,
						input_line_no, radix);
					number = 0;
				}
				break;
			}
			number = (number * radix) + new_digit;
		}

		if ( (number >= 0) && (number <= 9) &&
		     ( (c == 'b') || (c == 'f') || (c == ':') )
		   )
		{
			/* we have a reference to (Nb or Nf),
			 * or definition of (N:), a local symbol
			 */

			switch (c)
			{
			case 'b': /* "Nb" -- backward local label reference */
				yylval.y_cp = scratchp;
				scratchp = local_sym_name(number, 0, scratchp);
				ret_token = SYMBOL;
				break;

			case 'f': /* "Nf" -- forward  local label reference */
				yylval.y_cp = scratchp;
				scratchp = local_sym_name(number, 1, scratchp);
				ret_token = SYMBOL;
				break;

			case ':': /* "N:" -- local label declaration	*/
				new_local_sym(number);
				yylval.y_cp = scratchp;
				scratchp = local_sym_name(number, 0, scratchp);
				ret_token = LABEL;
				break;
			}
		}
		else
		{
			/* just a plain integer constant from the input;
			 * no funny stuff making it a "local symbol".
			 */
			unnextchar(c);

			yylval.y_value = get_scratch_value(NULL, 0);

			yylval.y_value->v_value   = number;/* value is the #  */
			yylval.y_value->v_relmode = VRM_NONE;/* no relocation */
			/* below already set, by get_scratch_value() */
			/* yylval.y_value->v_symp = NULL; */
			/* yylval.y_value->v_strptr= NULL;/* not quoted str'g*/
			ret_token = VALUE;
		}
	}

	return ret_token;
#undef RADIX_FP
}

TOKEN
cf_eos(c)	/* End Of Statement (either '\n' or ';') */
	register STDIOCHAR c;
{
	/* check the parentheses level at the end of the statement */
	if(paren_level > 0)	error(ERR_RPAREN, input_filename,input_line_no);

#ifdef DEBUG
	if ( debug & (c == '\n') )
	{
		*(ilp-1) = '\0'; /* zap '\n' */
		fprintf(stderr, "[%2d]\t%s\n", input_line_no, &input_line[0]);
	}
#endif
	unnextchar( (c == '\n') ? LEXCH_BOL : LEXCH_BOS );
	return EOSTMT;
}


TOKEN
cf_cmt()	/* a comment-to-the-end-of-the-line */
{
	register STDIOCHAR c;

	if ( do_disassembly && disassemble_with_comments )
	{
#define MAX_SAVED_COMMENT 80
		char line[MAX_SAVED_COMMENT];
		register char *lp;
		register int comment_len;

		/* eat rest of line */
		for ( comment_len = 0, lp = &line[0];
		      ((c=raw_nextchar()) != '\n') && (c != (EOF&INCHAR_MASK));
		      /* no loop incr here */
		    )
		{
			if (lp < &line[MAX_SAVED_COMMENT-1])
			{
				*lp++ = c;
				comment_len++;
			}
		}
		*lp = '\0';	/* null-terminate string */

		peekc = '\n';

		yylval.y_cp = get_scratch_buf(comment_len+1);
		strcpy(yylval.y_cp, line);
		return TOK_COMMENT;
	}
	else
	{
		/* eat rest of line */
		while ( ((c=raw_nextchar()) != '\n') &&
			(c != (EOF&INCHAR_MASK))
		      ) ;

		/* ASSUME that '\n' is an end-of-statement character */
		return cf_eos('\n');
	}
}


TOKEN
cf_quo(c)	/* collect a quoted string (quoted with char c) */
	register STDIOCHAR c;
{
	register int   quote_char;
	register int   i;
	register char *p;

	quote_char = c;

	p = scratchp;
	do
	{
		/* can't use raw_nextchar() here because there are
		 * unnextchar()'s in this loop.
		 */
		switch(c = nextchar())
		{
		case EOF:
		case '\n':
			unnextchar(c);	/* put the EOF/'\n' back */
			*p++ = '\0';
			error(ERR_UNTERM_STR, input_filename, input_line_no);
			yylval.y_value = formstring(scratchp, p-scratchp-1);
			scratchp = p;	/* save string */
			return STRING;

		case '\\':	/* an escaped char follows */
			switch(c = raw_nextchar())
			{
			case 'b':	*p++ = '\b';	break;
			case 'f':	*p++ = '\f';	break;
			case 'n':	*p++ = '\n';	break;
			case 'r':	*p++ = '\r';	break;
			case 't':	*p++ = '\t';	break;
			default:
				if ( (c >= '0') && (c <= '7') )
				{
					/* backslash-escaped octal #	     */
					*p = '\0';
					for(i = 0;
					    ((i < 3) && (c>='0') && (c<='7'));
					    i++, c = raw_nextchar())
					{
						*p = (*p * 8) + (c - '0');
					}
					p++;
					unnextchar(c);
				}
				else	*p++ = c; /* includes: \\ \' \" */
				break;
			}
			break;

		case '"':
		case '\'':
			if(c == quote_char)
			{
				/* was closing quote of string*/
			    	*p++ = '\0';
				yylval.y_value =
					formstring(scratchp, p-scratchp-1);
			    	scratchp = p;	/*  hold onto string this stmt*/
			    	return(STRING);
			}
			else
			{
				/* collect some char other than	*/
				/* the closing quote		*/
				*p++ = c;
			}
			break;

		default:
			*p++ = c;	/* collect any other char */
		}
	}  while(p < &scratch_buf[SCRATCHBUFLEN]);
	/* if we ever break out, we have overflowed the			*/
	/* max. buffer size, for strings/symbol names in a statement	*/
	internal_error(StringsTooLong, SCRATCHBUFLEN);
	/* since internal_error never returns, the following "return" is
	 * here just to  keep lint happy.
	 */
	return TOK_ERROR;
}


TOKEN
cf_or()	/* '|' char */
{
	yylval.y_uint = (unsigned int)OPER_LOG_OR;
	return TOK_LOG;
}

TOKEN
cf_xor()	/* '^' char */
{
	yylval.y_uint = (unsigned int)OPER_LOG_XOR;
	return TOK_LOG;
}

TOKEN
cf_and()	/* '&' char */
{
	yylval.y_uint = (unsigned int)OPER_LOG_AND;
	return TOK_LOG;
}

TOKEN
cf_bnt()	/* '~' char */
{
	yylval.y_uint = (unsigned int)OPER_BITNOT;
	return TOK_BITNOT;
}

TOKEN
cf_pls()	/* '+' char */
{
	yylval.y_uint = (unsigned int)OPER_ADD_PLUS;
	return TOK_ADD;
}


TOKEN
cf_min()	/* '-' char */
{
	yylval.y_uint = (unsigned int)OPER_ADD_MINUS;
	return TOK_ADD;
}


TOKEN
cf_mul()	/* '*' char */
{
	yylval.y_uint = (unsigned int)OPER_MULT_MULT;
	return TOK_MULT;
}


#ifdef DEBUG
static void
new_input_line()
{
	input_line_no++;
	ilp = &input_line[0];
}
#else
#  define new_input_line()	(++input_line_no)
#endif /*DEBUG*/


TOKEN
cf_div()	/* '/' char */
{
	register STDIOCHAR c;

	if ( (c = raw_nextchar()) == '*' )	/* "/*" starts comment */
	{
		/* eat the rest of the comment */
		for ( ; ; )
		{
			/* can't use raw_nextchar() for the first one in the
			 * following "for" loop, because there is an
			 * unnextchar() in the enclosing loop.
			 */
			for ( c=nextchar();
			      ((c != '*') && (c != (EOF&INCHAR_MASK)));
			      c=raw_nextchar()
			    )
			{
				if (c == '\n')
				{
					new_input_line();
					new_statement();
				}
			}

			/* here, "c" is '*' or EOF&INCHAR_MASK */
			if (c == (EOF&INCHAR_MASK))	break;
			else	/* c == '*';  check for '/' now */
			{
				if ( (c = raw_nextchar()) == '/' )	break;
				else	unnextchar(c);
			}
		}

#ifdef DEBUG
		lo_tkn_count[LOT_CMT]++;
#endif
		return TOK_LEXONLY;	/* (for a comment) */
	}
	else
	{
		unnextchar(c);
		yylval.y_uint = (unsigned int)OPER_MULT_DIV;
		return TOK_MULT;
	}
}


TOKEN
cf_lp()		/* '(' char */
{
	paren_level++;	/* ...paren_level is checked in cf_eos() */
	return (TOKEN)('(');
}


TOKEN
cf_rp()		/* ')' char */
{
	if (paren_level <= 0)	error(ERR_LPAREN, input_filename,input_line_no);

	paren_level--;
	return (TOKEN)(')');
}


TOKEN
cf_boi()	/* Beginning Of (all) Input: before 1st file is even opened */
{
	error_count = 0;

	/* "curr_flp" was set up in main(),
	 * to point to beginning of list of input files.
	 */

	/* Implicitly, assembly is to start in the UNIX "text" segment */
	change_named_segment("text");

	unnextchar(LEXCH_BOF);
#ifdef DEBUG
	lo_tkn_count[LOT_BOI]++;
#endif
	return TOK_LEXONLY;
}


static int
getlineno()	/* for cf_bol()'s use only! */
{
	register int c;
	register int lineno;

	for ( lineno = 0, c=nextchar();
	      ((c != ' ') && (c!='\n') && (c != (EOF&INCHAR_MASK)));
	      c=raw_nextchar()
	    )
	{
		if ( (c < '0') || (c > '9') )	return 0;
		lineno = (lineno * 10) + (c - '0');
	}

	if ( c == ' ' )	return lineno;
	else
	{
		unnextchar(c);
		return 0;
	}
}


static char *
getfilename()	/* for cf_bol()'s use only! */
{
	static char filename[100];
	register int c;
	register char *np;

	if ( (c=nextchar()) != '"' )	return (char *)NULL;
	for ( np = &filename[0], c=nextchar();
	      ((c != '"') && (c!='\n') && (c != (EOF&INCHAR_MASK)));
	      c=raw_nextchar()
	    )
	{
		*np++ = c;
	}
	*np = '\0';

	if ( c == '"' )	return &filename[0];
	else
	{
		unnextchar(c);
		return (char *)NULL;
	}
}


TOKEN
cf_bol()	/* Beginning Of input Line */
{
	register int c;

	new_input_line();

	if ( (c=raw_nextchar()) == '#' )
	{
		/* We have a line from preprocessor: '#' in column 1. */

		/* if line format is: '#',[blank],number,blank,"filename",\n,
		 * then all is ok;
		 * else, should have run preprocessor.
		 */
		if ( (c=raw_nextchar()) == ' ' ) {
			/* eat it */
		} else {
			unnextchar(c);
		}
		if ( (input_line_no =getlineno()) != 0
		&&   (input_filename=getfilename()) != NULL )
		{
			/* all is OK */
			/* since line# given is for the NEXT line, adjust
			 * it so that it is correct for THIS line.
			 */
			input_line_no--;
		}
		/* in any case, gobble up to the end of the line (or file) */
		for ( c=nextchar();
		      ((c != '\n') && (c != (EOF&INCHAR_MASK)));
		      c=raw_nextchar()
		    ) ;
		unnextchar(c);	/* return the '\n' or EOF to input */
	}
	else
	{
		unnextchar(c);	/* return the non-'#' char to the input */

		/* now, at this point, we really want to say:
		 *
		 *	unnextchar(LEXCH_BOS);
		 *
		 * to be consistant and advance our state to Begin-Of-Stmt
		 * (because that would cause cf_bos() to be called),
		 * but since we've already pushed ONE lookahead character back
		 * into the input (the "unnextchar(c)" above), we're stuck.
		 *
		 * So, we call "new_statement()" directly, instead,
		 * assuming that it does not touch the lookahead char.
		 */
		new_statement();
	}

#ifdef DEBUG
	lo_tkn_count[LOT_BOL]++;
#endif
	return TOK_LEXONLY;
}


static void
new_statement()	/* prepares to start new ass'y-language statement */
{
	/* it is assumed by callers that this routine does NOT touch
	 * the lookahead char.
	 */

	scratchp = &scratch_buf[0];	/* reset scratch-area ptr to beginning*/

	opcode_expected = TRUE;
	paren_level = 0;
}


TOKEN
cf_bos()	/* Beginning Of Statement */
{
	new_statement();
#ifdef DEBUG
	lo_tkn_count[LOT_BOS]++;
#endif
	return TOK_LEXONLY;
}


TOKEN
cf_bof()	/* Beginning Of an input File */
{
	/* start over at beginning of next file */
	input_line_no = 0;

	for (    ;    curr_flp != NULL;    curr_flp = curr_flp->fl_next )
	{
		if (curr_flp->fl_options.opt_preproc)
		{
			/* must run the preprocessor on the new input file
			 * before we look at it...
			 */
			static char fname[25]; /*assume loaded as all NULLs */
			extern int getpid();

			sprintf(fname, "/tmp/as_pp_%05d", getpid());
			pptmpfilename = &fname[0];

			if ( fork() == 0 )
			{
				/* in child process... */

				char cpp_path[256];
				int ofd; /* output file descriptor, for cpp */
				int dummy;
				extern int find_file_under_roots();
				extern char *getenv();

				/* open /tmp file for writing */
				ofd = open(pptmpfilename,
						O_WRONLY|O_CREAT|O_TRUNC, 0666);
				if ( ofd < 0 )
				{
					/* print the sys error message */
					perror(pgm_name);
					/* now print our own error message */
					error(ERR_CANT_OPEN_O, NULL, 0,
						pptmpfilename);
					/* try next file in list, if any */
					continue;
				}
				/* now make it the std. output (fd 1) for cpp */
				dup2(ofd, 1);

				/* The preprocessor args given by the user
				 * (-I/D/U), if any, are already in cpp_argv.
				 */
				/* input file name */
				if ( strcmp(curr_flp->fl_fnamep, "-") == 0 )
				{
					/* then DON'T add any input filename
					 * to cpp's arg list; no input filename
					 * means "std input" to cpp.
					 */
				}
				else	cpp_argv[cpp_argc++] =
						curr_flp->fl_fnamep;
				/* terminate the list */
				cpp_argv[cpp_argc++] = (char *)NULL;

				/* find the CPP we want, using the VIRTUAL ROOT
				 * stuff, and exec() it.
				 */
				if (0 == find_file_under_roots("/lib/cpp", NULL,
					&dummy, getenv("VIRTUAL_ROOT"),
					&cpp_path[0], sizeof(cpp_path)) )
				{
					execv(cpp_path, cpp_argv);
					internal_error("cf_bof(): couldn't execv(\"%s\",...)", cpp_path);
				}
				else	internal_error("cf_bof(): ${VIRTUAL_ROOT}/lib/cpp path too long");

			}
			else
			{
				/* in parent process */
				union wait status;

				wait(&status);
				if (status.w_retcode != 0)	error_count++;
			}

			curr_flp->fl_fnamep = pptmpfilename;
		}

		if ( strcmp(curr_flp->fl_fnamep, "-") == 0 )
		{
			/*"-" given on cmd-line is abbreviation for std. input*/
			currifp = stdin;
		}
		else
		{
			/* try to open (next) input file */
			if ((currifp = fopen(curr_flp->fl_fnamep, "r")) == NULL)
			{
				error(ERR_CANT_OPEN_I, NULL, 0,
					curr_flp->fl_fnamep);
				/* failed;  try next file in list, if any */
				continue;
			}

			/* if got here, then succeeded in opening new file */
		}
		input_filename = curr_flp->fl_fnamep;

		/* Succeeded in opening new input file (or stdin) */
		/* Now, add a "beginning of new file" node */
		{
			register Node *np;
			np = new_node(0, opcode_ptr[OP_PSEUDO_BOF]);
			np->n_flp = curr_flp;
			np->n_internal = TRUE;
			add_nodes(np);
		}

		unnextchar(LEXCH_BOL);	/* now pre-stuff a Begin-Of-Line char*/

#ifdef DEBUG
		lo_tkn_count[LOT_BOF1]++;
#endif
		return TOK_LEXONLY;
	}

	/* no files left in input list (curr_flp == NULL); this is a hard EOF */
	unnextchar(LEXCH_EOF);	/* (gets us to cf_eof() routine)	*/
#ifdef DEBUG
	lo_tkn_count[LOT_BOF2]++;
#endif
	return TOK_LEXONLY;
}


TOKEN
cf_eof()	/* End-Of-File */
{
	close_input_file();	/* close input file */

	/* remove /tmp file from preprocessor, if there was one */
	unlink_pp_tmp_file();

	if ( curr_flp == NULL )
	{
		/* no files left in input list;  this is a hard EOF */
		unnextchar(LEXCH_EOF);	/* make sure keep returning EOFILE */
#ifdef DEBUG
		if(debug_tkn_count)
		{
#			include <sys/file.h>
			int fd=open("sras.tkn_count",O_WRONLY|O_CREAT|O_TRUNC,0666);
			if (fd < 0)	internal_error("cf_eof(): open fail");
			write(fd, &tkn_count[0], sizeof(tkn_count));
			write(fd, &lo_tkn_count[0], sizeof(lo_tkn_count));
			close(fd);
		}
#endif
		return EOFILE;
	}
	else
	{
		/* "soft" EOF;  move on to next input file, for cf_bof() */
		curr_flp = curr_flp->fl_next;
		unnextchar(LEXCH_BOF);
#ifdef DEBUG
		lo_tkn_count[LOT_EOF]++;
#endif
		return TOK_LEXONLY;
	}
}


/******************  c l o s e _ i n p u t _ f i l e  **********************/

void
close_input_file()
{
	/* close input file */
	if (currifp != NULL)	fclose(currifp);
}


/***************  u n l i n k _ p p _ t m p _ f i l e  *********************/

void
unlink_pp_tmp_file()
{
	/* remove /tmp file from preprocessor, if there was one */
	if (pptmpfilename != NULL)
	{
		unlink(pptmpfilename);
		pptmpfilename = NULL;
	}
}


/***********************  c o l l e c t _ s y m  *****************************/

char *			/* returns ptr to collected symbol name */
collect_sym(cp)
	STDIOCHAR *cp;	/* 1st char, already read */
{
	register STDIOCHAR  c;
	register char	   *p;		/* temp. register copy of scratchp  */
	register char	   *symname;	/* ptr to begin of scanned sym name */
	register TOKEN	    (*chartype)();

	c = *cp;
	p = symname = scratchp;	/* store symbol name in scratch buffer */
	/* collect rest of symbol name */
	*p++ = c;
	for ( c=nextchar();
	      ((chartype=char_fcn_table[c]) == cf_ltr) || (chartype == cf_dig);
	      c=raw_nextchar()
	    )
	{
		*p++ = c;
	}
	*p++ = '\0';	/* null-terminate symbol name	*/

	/* skip blanks, so that in the case of "SYMBOL<white>:" or
	 * "SYMBOL<white>=", the ':' or '=' gets returned.
	 */
	while( chartype == cf_wht )
	{
		chartype = char_fcn_table[(c=raw_nextchar())];
	}
	unnextchar(c);	/* put back unwanted character	*/

	if(p >= &scratch_buf[SCRATCHBUFLEN-1])
	{
		internal_error(StringsTooLong, SCRATCHBUFLEN);
	}

	*cp = c;	/* return last (unwanted) character read */
	scratchp = p;	/* SAVE the symbol name in the buffer (until EOS) */
	return symname;
}


/*********************  c o l l e c t _ f p n u m  *************************/

char *			/* returns ptr to collected floating-point number */
collect_fpnum(c)
	register STDIOCHAR c;
{
	register char	   *p;		/* temp. register copy of scratchp  */
	register char	   *nump;	/* ptr to begin of scanned f.p. #   */

	p = nump = scratchp;	/* store F.P. number in scratch buffer */

	/* the leading "0r" has been read at this point, and the character
	 * passed in "c" is the first char of the following floating-point
	 * number.
	 */

	/* collect rest of floating-point # ...    */

	/* skip white space between "0r" and number */
	while (char_fcn_table[c] == cf_wht)	c = raw_nextchar();

	while ( (char_fcn_table[c] == cf_dig) ||
	        (c == '+') || (c == '-') ||	/* signs	*/
	        (c == 'e') || (c == 'E') ||	/* exponents	*/
	        (c == '.') ||			/* decimal point*/
			/* letters for "nan"/"inf" ("0r[+-]nan", "0r[+-]inf") */
	        (c == 'n') || (c == 'a') || (c == 'i') || (c == 'f')
	      )
	{
		*p++ = c;
		c = raw_nextchar();
	}
	*p++ = '\0';	/* null-terminate symbol name	*/
	unnextchar(c);	/* put back unwanted character	*/

	if(p >= &scratch_buf[SCRATCHBUFLEN-1])
	{
		internal_error(StringsTooLong, SCRATCHBUFLEN);
	}

	scratchp = p;
	return nump;
}


/************************  f o r m s t r i n g  ******************************/
struct value *
formstring(sp, length)
	register char *sp;
	register int length;
{
	register struct value *valp;

	valp = get_scratch_value(sp, length);

	valp->v_value	= *sp;	/* value is that of 1st char */
	/* valp->v_symp	= NULL;	*/ /* already set, by get_scratch_value() */

	return valp;
}

/********************  g e t _ s c r a t c h _ b u f *************************/
char *
get_scratch_buf(n)
	register int n;
{
	register char *bufp;
	register char *p;

	/* Return a pointer to some scratch memory of length "n" bytes.
	 *
	 * If we are building a node list then dynamically allocate what we
	 * need, as we don't want the buffer space to go away after the end
	 * of each statement.
	 *
	 * If we are not building a node list, take a statically-allocated
	 * piece of memory (from "scratch_buf").
	 */

	switch (assembly_mode)
	{
	case ASSY_MODE_BUILD_LIST:
		bufp = (char *)as_malloc(n);
		break;

	default:
		if( (p = (scratchp + n)) < &scratch_buf[SCRATCHBUFLEN-1])
		{
			bufp = scratchp;
			scratchp = p;
		}
		else
		{
			internal_error(StringsTooLong, SCRATCHBUFLEN);
			/*NOTREACHED*/
		}
		break;
	}

	return bufp;
}
