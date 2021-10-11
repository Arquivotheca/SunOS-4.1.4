%{
static char sccs_id[] = "@(#)parse.y	1.1\t10/31/94";
#include <stdio.h>	/* for def'n of NULL */
#include "structs.h"
#include "errors.h"
#include "actions.h"
#include "globals.h"
#include "registers.h"
%}

%union {     /* union for YYSTYPE (Yacc stack) */
        int                y_int;
        unsigned int       y_uint;
	char		  *y_cp;
        Regno		   y_regno;
        struct opcode     *y_opcode;
        struct symbol     *y_symbol;
        struct value      *y_value;
        struct operands   *y_operp;
        struct val_list_head *y_vlhp;	/* ptr to header for a list of values */
	INSTR	           y_instr;
	Node		  *y_nodep;	/* ptr to a C2 (peephole opt.) node */
       }

%{

extern TOKEN yylex();

yyerror() { } /* will handle our own errors */

void
stmt_error()
{
	error(ERR_SYNTAX,input_filename,input_line_no);
}
%}

		/*TOK_LEXONLY used only within lex;  parser should never see */
%token <y_void>   TOK_LEXONLY

%token <y_cp>     TOK_COMMENT	/* returned for comments to be saved */

%token <y_void>   TOK_ERROR	/* returned for certain lex errors */

%token <y_regno>  REG

%token <y_opcode> TOK_0OPS
%token <y_opcode> TOK_LD
%token <y_opcode> TOK_LDA
%token <y_opcode> TOK_ST
%token <y_opcode> TOK_STA
%token <y_opcode> TOK_ARITH
%token <y_opcode> TOK_4OPS
%token <y_opcode> TOK_3OPS
%token <y_opcode> TOK_2OPS
%token <y_opcode> TOK_1OPS
%token <y_opcode> TOK_PSEUDO
%token <y_opcode> TOK_FP_PSEUDO

	/* for internal-only opcodes which are never seen in input */
%token <y_opcode> TOK_NONE

%token <y_cp>     LABEL
%token <y_cp>     SYMBOL	/* symbols		*/

%token <y_value>  STRING	/* quoted strings	*/
%token <y_value>  VALUE		/* constant #s		*/
%token <y_value>  FP_VALUE	/* constant floating-pt #s*/

%token <y_uint>   TOK_HILO	/* %lo, %hi		*/

%token <y_uint>   TOK_ADD	/*  +, -		*/
%token <y_uint>   TOK_MULT	/*  *, /		*/
%token <y_uint>   TOK_SHIFT	/*  <<, >>		*/
%token <y_uint>   TOK_LOG       /*  &, |, ^             */
%token <y_uint>   TOK_BITNOT    /*  ~                   */

%token <y_void>   TOK_SFA_ADDR	/* %ad			*/

%token	<y_void>  EOSTMT
%token	<y_void>  EOFILE 0


			/* operator precedence, in increasing order */
%left TOK_LOG
%left TOK_SHIFT
%left TOK_ADD
%left TOK_MULT
%nonassoc TOK_BITNOT  TOK_HILO


%start whole_input	/* start-rule (below) for parse */

%type  <y_nodep> instruction

%type  <y_uint>  asi

%type  <y_nodep> optlabel
%type  <y_nodep> optcmt

%type  <y_operp> reg_plus_reg
%type  <y_operp> reg_plus_const13
%type  <y_operp> regsource
%type  <y_operp> source

%type  <y_value> sfa_addr

%type  <y_value> expr

%type  <y_vlhp>  expr_list
%type  <y_value> expr_list_element

%type  <y_vlhp>  fp_expr_list
%type  <y_value> fp_expr_list_element
%%  /*---------------------------------------------------------------------*/

asi:	  expr /* constant 0 .. 255 */	{ $$ = eval_asi($1);		} ;

optlabel:	
	  /* empty */			{ $$ = NULL;			}
	| LABEL				{ $$ = eval_label($1);		}
	;

optcmt:	
	  /* empty */			{ $$ = NULL;			}
	| TOK_COMMENT			{ $$ = eval_comment($1);	}
	;

	  /*-----------------------------------------------*/
	  /*	...the addressing modes			   */
	  /*-----------------------------------------------*/

reg_plus_reg:     REG     TOK_ADD  REG	{ $$ = eval_RpR($1,$2,$3);	   }
	;

	/* TOK_ADD is commented out before "expr" in the following	*/
	/* rule because the TOK_ADD would have been seen in "expr",	*/
	/* as a unary '+'/'-' on the expression.			*/
reg_plus_const13:
	  REG  /*TOK_ADD*/  expr	{ $$ = eval_RpC($1,OPER_ADD_PLUS,$2);}
	| expr   TOK_ADD    REG		{ $$ = eval_RpC($3,$2,$1);	   }
	;

regsource:
	  REG				{ $$ = eval_REG_to_ADDR($1);	   }
	| reg_plus_reg			{ $$ = $1;			   }

source:
	  regsource			{ $$ = $1;			   }
	| reg_plus_const13		{ $$ = $1;			   }
	| expr				{ $$=eval_RpC(RN_GP(0),OPER_ADD_PLUS,$1);}
	;

sfa_addr:  TOK_SFA_ADDR expr		{ $$ = $2;			   }

	  /*-----------------------------------------------*/

whole_input:  input  EOFILE
	;

input:	  /* empty */
	| input  statement
	;

statement:
	  optlabel instruction optcmt EOSTMT	{ add_nodes($1);
						  add_nodes($2);
						  add_nodes($3);
						}
	| LABEL    '='  expr   optcmt EOSTMT	{ add_nodes(eval_equate($1,$3));
						  add_nodes($4);
						}
	| error			EOSTMT		{ stmt_error();		       }
	;
	  /*-----------------------------------------------*/

instruction:
	  /* empty */				{ $$ = (Node *)NULL;	     }

	| TOK_0OPS				{ $$=eval_opc0ops($1);       }

	| TOK_LD '[' source ']' ',' REG		{ $$=eval_ldst($1,$3,$6);    }

	| TOK_LD '[' sfa_addr ']' ',' REG	{ $$=eval_sfa_ldst($1,$3,$6);}

	| TOK_LDA '[' regsource ']' asi ',' REG { $$=eval_ldsta($1,$3,$7,$5);}


	| TOK_ST REG ',' '[' source ']'		{ $$=eval_ldst($1,$5,$2);    }

	| TOK_ST REG ',' '[' sfa_addr ']'	{ $$=eval_sfa_ldst($1,$5,$2);}

	| TOK_STA REG ',' '[' regsource ']' asi { $$=eval_ldsta($1,$5,$2,$7);}

	  /*-----------------------------------------------*/

	| TOK_ARITH/*only save & restore*/	{$$=eval_arithNOP($1);	       }
	| TOK_ARITH  REG  ',' REG /*only "wr"*/	{$$=eval_arithRR($1,$2,$4);    }
	| TOK_ARITH  REG  ',' REG  ',' REG	{$$=eval_arithRRR($1,$2,$4,$6);}
	| TOK_ARITH  REG  ',' expr ',' REG	{$$=eval_arithRCR($1,$2,$4,$6);}
	| TOK_ARITH  expr ',' REG  ',' REG	{$$=eval_arithCRR($1,$2,$4,$6);}

	  /*-----------------------------------------------*/

	  /* OK for  mov, cmp,sethi,   ,   ,       ,inc/dec,bit,set,        */
	  /* expr: 13 for mov, 22 for sethi */
	  /* expr with vp->v_relmode==VRM_HI22 only OK for sethi */
	| TOK_2OPS  expr  ','  REG		{ $$=eval_ER($1,$2,$4);       }

	  /* OK for  mov, cmp,     , rd, f*,not/neg,       ,bit,   ,.noalias */
	| TOK_2OPS  REG  ','  REG		{ $$=eval_RR($1,$2,$4);       }

	  /* OK for     , cmp,     ,   ,   ,       ,       ,   ,   ,        */
	| TOK_2OPS  REG  ','  expr		{ $$=eval_2opsRE($1,$2,$4);   }

	  /* OK for  mov,    ,     ,   ,   ,       ,       ,   ,   ,        */
	| TOK_2OPS  sfa_addr  ','  REG		{ $$=eval_2opsSfaR($1,$2,$4); }

	  /* OK for     ,    ,     ,   ,   ,not/neg,inc/dec,   ,   ,        */
	| TOK_2OPS  REG				{ $$=eval_2opsR($1,$2);	      }

	  /*-----------------------------------------------*/

	  /*OK for    ,  ,      ,unimp,               ,     ,   ,    ,   */
	| TOK_1OPS  				{ $$=eval_1opsNOARG($1);      }

	  /*OK for    ,br, call ,unimp,jmp/rett/iflush,traps,   ,    ,   */
	| TOK_1OPS  expr/* const13 for traps */	{ $$=eval_1opsE($1,$2);	      }

	  /*OK for    ,  ,(call),     ,jmp/rett/iflush,traps,   ,    ,   */
	| TOK_1OPS  reg_plus_const13		{ $$=eval_1opsA($1,$2);       }
	| TOK_1OPS  reg_plus_reg		{ $$=eval_1opsA($1,$2);       }

	  /*OK for tst,  ,(call),     ,jmp/rett/iflush,traps,clr,    ,   */
	| TOK_1OPS  REG				{ $$=eval_1opsR($1,$2);	      }

	  /*OK for    ,  ,(call),     ,               ,     ,   ,    ,   */
	| TOK_1OPS  reg_plus_const13 ',' expr	{ $$=eval_call_AE($1,$2,$4);  }
	| TOK_1OPS  reg_plus_reg ',' expr	{ $$=eval_call_AE($1,$2,$4);  }
	| TOK_1OPS  REG ',' expr		{ $$=eval_call_RE($1,$2,$4);  }

	  /*OK for    ,  ,      ,     ,               ,     ,   ,jmpl,   */
	| TOK_1OPS  reg_plus_const13  ','  REG	{ $$=eval_j_ARE($1,$2,$4,NULL);}
	| TOK_1OPS  reg_plus_reg  ','  REG	{ $$=eval_j_ARE($1,$2,$4,NULL);}
	| TOK_1OPS  REG   ','  REG		{ $$=eval_RR($1,$2,$4);        }
	| TOK_1OPS  reg_plus_const13 ',' REG ',' expr
						{ $$=eval_j_ARE($1,$2,$4,$6);  }
	| TOK_1OPS  reg_plus_reg ',' REG ',' expr
						{ $$=eval_j_ARE($1,$2,$4,$6);  }
	| TOK_1OPS  REG   ','  REG  ','  expr	{ $$=eval_j_RRE($1,$2,$4,$6);  }

	  /*OK for    ,  , call ,     ,               ,     ,   ,    ,   */
	| TOK_1OPS  expr  ','  expr		{ $$=eval_EE($1,$2,$4);       }

	  /*OK for    ,  ,      ,     ,               ,     ,clr,    ,   */
	| TOK_1OPS  '[' source ']'		{ $$=eval_clr($1,$3);         }

	  /*OK for    ,  ,      ,     ,               ,     ,clr,    ,   */
	| TOK_1OPS  '[' sfa_addr ']'		{ $$=eval_sfa_clr($1,$3);     }

	  /*-----------------------------------------------*/

	| TOK_PSEUDO        expr_list		{ $$=eval_pseudo($1,$2);      }
	| TOK_FP_PSEUDO  fp_expr_list		{ $$=eval_pseudo($1,$2);      }
 
	  /*-----------------------------------------------*/

	| TOK_3OPS REG ',' REG ',' REG		{ $$=eval_3ops($1,$2,$4,$6);  }


	  /* So far only cpop1 and cpop2 */
        | TOK_4OPS expr ',' REG ',' REG ',' REG { $$=eval_4ops($1,$2,$4,$6,$8);}

	; /*end of "instruction"*/


expr_list:
	  expr_list_element			{ $$ = eval_elist(NULL,$1);   }
	| expr_list ',' expr_list_element	{ $$ = eval_elist($1,$3);     }
	;

expr_list_element:
	  /* empty */				{ $$ = NULL;		      }
	| expr					{ $$ = $1;		      }
	;

fp_expr_list:
	  fp_expr_list_element			{ $$ = eval_elist(NULL,$1);   }
	| fp_expr_list ',' fp_expr_list_element	{ $$ = eval_elist($1,$3);     }
	;

fp_expr_list_element:
	  /* empty */				{ $$ = NULL;		      }
	| FP_VALUE				{ $$ = $1;		      }
	| expr					{ $$ = num_to_string($1);     }
	;

/*** expr_list:
/*** 	  expr					{ $$ = eval_elist(NULL,$1);   }
/*** 	| expr_list ',' expr			{ $$ = eval_elist($1,$3);     }
/*** 	;  ***/


expr:	/* expression */
	  '(' expr ')'				{ $$ = $2;		      }
	| TOK_HILO expr				{ $$ = eval_hilo($1,$2);      }
	| VALUE	/* a number or quoted string;*/	{ $$ = $1;		      }
		/* quoted string:	     */
		/*	$$->v_strptr != NULL */
	| SYMBOL				{ $$ = symbol_to_value($1);   }
	| STRING				{ $$ = $1;		      }
	| expr TOK_ADD  expr			{ $$ = eval_add($1,$2,$3);    }
	| expr TOK_MULT expr			{ $$ = eval_bin($1,$2,$3);    }
	| expr TOK_LOG  expr			{ $$ = eval_bin($1,$2,$3);    }
	| expr TOK_SHIFT expr			{ $$ = eval_bin($1,$2,$3);    }
        | TOK_BITNOT expr			{ $$ = eval_bitnot($1,$2);    }
	| TOK_ADD expr				{ $$ = eval_uadd($1,$2);      }
	;
