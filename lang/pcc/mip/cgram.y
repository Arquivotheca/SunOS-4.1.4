%{
#ifndef lint
static	char sccsid[] = "@(#)cgram.y 1.1 94/10/31 SMI"; /* from S5R2 1.7 */
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#define YACC	/*KLUDGE*/
%}

/*
 * Guy's changes from System V lint (12 Apr 88):
 *
 * Updated "cgram.y".  Main changes are messages and stuff added for
 * "cxref".  (I have no idea why it's done with weird comments and a "sed"
 * script, rather than #ifdefs.)  Eliminated #ifdef FLEXNAMES (and #ifdef
 * FLEXSTRINGS; has that ever been defined?  What does it do?).  "lint"
 * cleanups (removing unused variables, casts of pointer arguments).
 * 
 * Changes from S5 "lint" (fix to eliminate bogus "statement not reached"
 * messages, check for label used as variable added, check for "long" as
 * "case:" constant added if "-p" flag specified).
 * 
 * Explicit checks for "malloc" failure added.
 * 
 * Message for item other than integer constant in "case:"  clause changed
 * to be "case expression must be an integer constant" in all cases,
 * rather than "non-constant case expression" for nodes other than ICON and
 * "illegal constant in case expression" for ICON nodes with names.  The
 * old compiler would print "non-constant case expression" if a
 * floating-point constant was used, which seems a bit confusing.
 */

/******/
/*	Note: In this file there are comments of the form
/*		/*CXREF <line-of-code> */
/*	When this file is used to build cxref, the beginning
/*	of the comment and the CXREF are removed along with
/*	the trailing end of the comment.  Therefore, these
/*	comments cannot be removed with out changing cxref's
/*	version of this file.
/******/

%term NAME  2
%term STRING  3
%term ICON  4
%term FCON  5
%term PLUS   6
%term MINUS   8
%term MUL   11
%term AND   14
%term OR   17
%term ER   19
%term QUEST  21
%term COLON  22
%term ANDAND  23
%term OROR  24

/*	special interfaces for yacc alone */
/*	These serve as abbreviations of 2 or more ops:
	ASOP	=, = ops
	RELOP	LE,LT,GE,GT
	EQUOP	EQ,NE
	DIVOP	DIV,MOD
	SHIFTOP	LS,RS
	ICOP	ICR,DECR
	UNOP	NOT,COMPL
	STROP	DOT,STREF

	*/
%term ASOP  25
%term RELOP  26
%term EQUOP  27
%term DIVOP  28
%term SHIFTOP  29
%term INCOP  30
%term UNOP  31
%term STROP  32

/*	reserved words, etc */
%term TYPE  33
%term CLASS  34
%term STRUCT  35
%term RETURN  36
%term GOTO  37
%term IF  38
%term ELSE  39
%term SWITCH  40
%term BREAK  41
%term CONTINUE  42
%term WHILE  43
%term DO  44
%term FOR  45
%term DEFAULT  46
%term CASE  47
%term SIZEOF  48
%term ENUM 49


/*	little symbols, etc. */
/*	namely,

	LP	(
	RP	)

	LC	{
	RC	}

	LB	[
	RB	]

	CM	,
	SM	;

	*/

%term LP  50
%term RP  51
%term LC  52
%term RC  53
%term LB  54
%term RB  55
%term CM  56
%term SM  57
%term ASSIGN  58
%term ASM 112

/* at last count, there were 7 shift/reduce, 1 reduce/reduce conflicts
/* these involved:
	if/else
	recognizing functions in various contexts, including declarations
	error recovery
	*/

%left CM
%right ASOP ASSIGN
%right QUEST COLON
%left OROR
%left ANDAND
%left OR
%left ER
%left AND
%left EQUOP
%left RELOP
%left SHIFTOP
%left PLUS MINUS
%left MUL DIVOP
%right UNOP
%right INCOP SIZEOF
%left LB LP STROP
%{
# include "cpass1.h"
# include "sw.h"
# include "messages.h"
# define yyerror( x ) ccerror( x, yychar )

# ifdef BROWSER
# include <cb_sun_c_tags.h>
# include <cb_ccom.h>
# include <cb_enter.h>
# endif
%}

	/* define types */
%start ext_def_list

%type <intval> con_e ifelprefix ifprefix whprefix forprefix doprefix switchpart
		enum_head str_head name_lp
#ifdef BROWSER
%type <intval> SIZEOF
%type <expr> e .e term elist funct_idn
#else
%type <nodep> e .e term elist funct_idn
#endif
%type <nodep> attributes oattributes type enum_dcl struct_dcl
		cast_type null_decl declarator fdeclarator nfdeclarator

%token <intval> CLASS RELOP CM DIVOP PLUS MINUS SHIFTOP MUL AND OR ER ANDAND OROR
		ASSIGN STROP INCOP UNOP ICON STRING
#ifdef BROWSER
%token <intval2> STRUCT ENUM
%token <name>	NAME
%token <type> TYPE
#else
%token <intval> NAME STRUCT
%token <nodep> TYPE
#endif

%%

%{
	static int fake = 0;
	static char fakename[24];	/* room enough for pattern */
	extern int errline, lineno;
#ifndef LINT
	static NODE *loop_test = NIL;
#endif /*!LINT*/
%}

ext_def_list:	   ext_def_list external_def
		|
			=ftnend();
		;
external_def:	   data_def
			={ curclass = SNULL;  blevel = 0; }
		|  ASM LP STRING RP SM
			={  
#ifdef BROWSER
			    cb_ccom_enter_string(cb_sun_c_asm_string,
						$3,
						cb_ccom_string);
#endif
			    asmout(); curclass = SNULL;  blevel = 0; }
		|  error
			={ curclass = SNULL;  blevel = 0; instruct = 0; }
		;
data_def:
		   oattributes  SM
			={  $1->in.op = FREE; }
		|  oattributes init_dcl_list  SM
			={  $1->in.op = FREE; }
		|  oattributes fdeclarator {
#ifndef LINT
				struct symtab *sp;
#endif
				defid( tymerge($1,$2), curclass==STATIC?STATIC:EXTDEF );
#ifndef LINT
				sp = STP($2->tn.rval);
				if (sp != NULL) {
					pfstab( sp->sname );
					}
#endif
				/*CXREF bbcode(); */
#ifdef BROWSER
				cb_ccom_wo_body2w_body(cb_ccom_func);
				cb_ccom_func = NULL;
#endif
				}  function_body
			={  
			    if( blevel ) cerror( "function level error" );
			    if( reached ) retstat |= NRETVAL; 
			    $1->in.op = FREE;
#ifdef BROWSER
			    cb_enter_function_end( lineno );
#endif
			    ftnend();
			    }
		;

function_body:	   arg_dcl_list compoundstmt
			={
#ifndef LINT
			    psline(lineno);
#endif
			    }
		;
arg_dcl_list:	   arg_dcl_list declaration
		| 	={  blevel = 1; }
		;

stmt_list:	   stmt_list statement
		|  /* empty */
			={  bccode();
			    locctr(PROG);
			    }
		;

r_dcl_stat_list	:  dcl_stat_list attributes SM
			={  $2->in.op = FREE; 
#ifndef LINT
			    plcstab(blevel);
#endif
			    }
		|  dcl_stat_list attributes init_dcl_list SM
			={  $2->in.op = FREE; 
#ifndef LINT
			    plcstab(blevel);
#endif
			    }
		;

dcl_stat_list	:  dcl_stat_list attributes SM
			={  $2->in.op = FREE; }
		|  dcl_stat_list attributes init_dcl_list SM
			={  $2->in.op = FREE; }
		|  /* empty */
		;
declaration:	   attributes declarator_list  SM
			={ curclass = SNULL;  $1->in.op = FREE; }
		|  attributes SM
			={ curclass = SNULL;  $1->in.op = FREE; }
		|  error  SM
			={  curclass = SNULL; }
		;
oattributes:	  attributes
		|  /* VOID */
			={  $$ = mkty(INT,0,INT);  curclass = SNULL; }
		;
attributes:	   class type
			={  $$ = $2; }
		|  type class
		|  class
			={  $$ = mkty(INT,0,INT); }
		|  type
			={ curclass = SNULL ; }
		|  type class type
			={  $1->in.type =
				types( $1->in.type, $3->in.type, UNDEF );
			    $3->in.op = FREE;
			    }
		;


class:		  CLASS
			={  curclass = $1; }
		;

type:		   TYPE
			={
#ifdef BROWSER
			$$ = $1.nodep;
			if ( cb_flag ) {
				    cb_enter_type(0,
							$1.nodep->in.type,
							$1.typdef,
							$1.lineno);
				cb_ccom_maybe_cast[1] = NULL;
				cb_ccom_maybe_cast[2] = NULL;
			}
#endif
			}

		|  TYPE TYPE
			={
#ifdef BROWSER
			    if ( cb_flag ) {
					 cb_enter_type(0,
							$1.nodep->in.type,
							$1.typdef,
							$1.lineno);
					 cb_enter_type(1,
							$2.nodep->in.type,
							$2.typdef,
							$2.lineno);
				    cb_ccom_maybe_cast[2] = NULL;
			    }

			    $1.nodep->in.type = types( $1.nodep->in.type, $2.nodep->in.type, UNDEF );
			    $2.nodep->in.op = FREE;
			    $$ = $1.nodep;
#else
			    $1->in.type = types( $1->in.type, $2->in.type, UNDEF );
			    $2->in.op = FREE;
#endif
			    }
		|  TYPE TYPE TYPE
			={
#ifdef BROWSER
			    if ( cb_flag ) {
					 cb_enter_type(0,
							$1.nodep->in.type,
							$1.typdef,
							$1.lineno);
					 cb_enter_type(1,
							$2.nodep->in.type,
							$2.typdef,
							$2.lineno);
					 cb_enter_type(2,
							$3.nodep->in.type,
							$3.typdef,
							$3.lineno);
			    }

			    $1.nodep->in.type = types( $1.nodep->in.type, $2.nodep->in.type, $3.nodep->in.type );
			    $2.nodep->in.op = $3.nodep->in.op = FREE;
			    $$ = $1.nodep;
#else
			    $1->in.type = types( $1->in.type, $2->in.type, $3->in.type );
			    $2->in.op = $3->in.op = FREE;
#endif
			    }
		|  struct_dcl
		|  enum_dcl
		;

enum_dcl:	   enum_head LC moe_list optcomma RC
			={ $$ = dclstruct($1);
#ifdef BROWSER
			   cb_ccom_block_end(lineno, 1);
			   cb_ccom_maybe_cast[0] = NULL;
			   cb_ccom_maybe_cast[1] = NULL;
			   cb_ccom_maybe_cast[2] = NULL;
#endif
		   }
		|  ENUM NAME
			={
#ifdef BROWSER
				cb_enter_name_mark_hidden(STP($2.id),
						$2.lineno,
						cb_c_ref_enum_tag_decl,
						cb_c_ref_enum_tag_decl_hidden,
						cb_ccom_maybe_cast[0]);
				cb_ccom_maybe_cast[1] = NULL;
				cb_ccom_maybe_cast[2] = NULL;
				$$ = rstruct($2.id,0);  stwart = instruct;
#else
				$$ = rstruct($2,0);  stwart = instruct;
				/*CXREF ref($2, lineno); */
#endif
			}
		;

enum_head:	   ENUM
			={  $$ = bstruct(-1,0); stwart = SEENAME;
#ifdef BROWSER
			    cb_ccom_block_start(CB_EX_FOCUS_UNIT_ENUM_KEY,
						lineno);
			    if (cb_flag && ($1.i2 > 0))
					cb_enter_symbol("enum",
						$1.i2,
						cb_c_def_enum_tag_anonymous);
#endif
			}
		|  ENUM NAME
			={
#ifdef BROWSER
				cb_ccom_block_start(CB_EX_FOCUS_UNIT_ENUM_KEY,
						    lineno);
				cb_enter_name_hidden(STP($2.id),
						$2.lineno,
						cb_c_def_enum_tag,
						cb_c_def_enum_tag_hidden);
				$$ = bstruct($2.id,0); stwart = SEENAME;
#else
				$$ = bstruct($2,0); stwart = SEENAME;
				/*CXREF ref($2, lineno); */
#endif
			}
		;

moe_list:	   moe
		|  moe_list CM moe
		;

moe:		   NAME
			={
#ifdef BROWSER
				cb_enter_name_hidden(STP($1.id),
						$1.lineno,
						cb_c_def_enum_member,
						cb_c_def_enum_member_hidden);
				moedef( $1.id );
#else
				moedef( $1 );
				/*CXREF def($1, lineno); */
#endif
			}
		|  NAME ASSIGN con_e
			={
#ifdef BROWSER
				cb_enter_name_hidden(STP($1.id),
						     $1.lineno,
						     cb_c_def_enum_member_init,
						     cb_c_def_enum_member_init_hidden);
				strucoff = $3;  moedef( $1.id );
#else
				strucoff = $3;  moedef( $1 );
				/*CXREF def($1, lineno); */
#endif
			}
		;

struct_dcl:	   str_head LC type_dcl_list optsemi RC
			={ $$ = dclstruct($1);
#ifdef BROWSER
			   cb_ccom_block_end(lineno, 1);
			   cb_ccom_maybe_cast[0] = NULL;
			   cb_ccom_maybe_cast[1] = NULL;
			   cb_ccom_maybe_cast[2] = NULL;
			   array_delete(cb_ccom_in_struct,
					array_size(cb_ccom_in_struct)-1);
#endif
			}
		|  STRUCT NAME
			={
#ifdef BROWSER
			cb_enter_name_mark_hidden(STP($2.id),
					   $2.lineno,
					   $1.i1 == INSTRUCT ?
						cb_c_ref_struct_tag_decl:
						cb_c_ref_union_tag_decl,
					   $1.i1 == INSTRUCT ?
						cb_c_ref_struct_tag_decl_hidden:
						cb_c_ref_union_tag_decl_hidden,
					   cb_ccom_maybe_cast[0]);
			cb_ccom_maybe_cast[1] = NULL;
			cb_ccom_maybe_cast[2] = NULL;
			$$ = rstruct($2.id,$1.i1);
#else
			$$ = rstruct($2,$1);
			/*CXREF ref($2, lineno); */
#endif
			}
		;

str_head:	   STRUCT
			={
#ifdef BROWSER
			    $$ = bstruct(-1,$1.i1);  stwart=0;
			    cb_ccom_block_start($1.i1 == INSTRUCT ?
						CB_EX_FOCUS_UNIT_STRUCT_KEY:
						CB_EX_FOCUS_UNIT_UNION_KEY,
						lineno);
			    if (cb_flag && ($1.i2 > 0)) {
				cb_enter_symbol(
					$1.i1 == INSTRUCT ?
						"struct" :
						"union",
					$1.i2,
					$1.i1 == INSTRUCT ?
						cb_c_def_struct_tag_anonymous :
						cb_c_def_union_tag_anonymous);
			    }
			    if (cb_ccom_in_struct == NULL) {
				    cb_ccom_in_struct =
				      array_create((Cb_heap)NULL);
			    }
			    array_append(cb_ccom_in_struct, $1.i1);
#else
			    $$ = bstruct(-1,$1);  stwart=0;
#endif
			}
		|  STRUCT NAME
			={
#ifdef BROWSER
				cb_ccom_block_start($1.i1 == INSTRUCT ?
						    CB_EX_FOCUS_UNIT_STRUCT_KEY:
						    CB_EX_FOCUS_UNIT_UNION_KEY,
						    lineno);
				cb_enter_name_hidden(STP($2.id),
					$2.lineno,
					$1.i1 == INSTRUCT ?
						cb_c_def_struct_tag :
						cb_c_def_union_tag,
					$1.i1 == INSTRUCT ?
						cb_c_def_struct_tag_hidden :
						cb_c_def_union_tag_hidden);
				if (cb_ccom_in_struct == NULL) {
					cb_ccom_in_struct =
					  array_create((Cb_heap)NULL);
				}
				array_append(cb_ccom_in_struct, $1.i1);
				$$ = bstruct($2.id,$1.i1);  stwart=0;
#else
			        $$ = bstruct($2,$1);  stwart=0;
				/*CXREF def($2, lineno); */
#endif
			}
		;

type_dcl_list:	   type_declaration
		|  type_dcl_list SM type_declaration
		|  error
		;

type_declaration:  type declarator_list
			={ curclass = SNULL;  stwart=0; $1->in.op = FREE; }
		|  type
			={  if( curclass != MOU ){
				curclass = SNULL;
				}
			    else {
				sprintf( fakename, "$%dFAKE", fake++ );
				/*
				* We will not be looking up this name in the
				* symbol table, so there is no reason to save
				* it anywhere.  (I think.)
				*/
				defid( tymerge($1,
					bdty(NAME,NIL,
					lookup( fakename, SMOS ))), curclass );
				/*"structure typed union member must be named"*/
				WERROR(MESSAGE( 106 ));
				}
			    stwart = 0;
			    $1->in.op = FREE;
			    }
		;


declarator_list:   declarator
			={ defid( tymerge($<nodep>0,$1), curclass);  stwart = instruct; }
		|  declarator_list  CM {$<nodep>$=$<nodep>0;}  declarator
			={ defid( tymerge($<nodep>0,$4), curclass);  stwart = instruct; }
		;
declarator:	   fdeclarator ={ noargs(); }
		|  nfdeclarator
		|  nfdeclarator COLON con_e
			%prec CM
			/* "field outside of structure" */
			={  if( !(instruct&INSTRUCT) ) UERROR( MESSAGE( 38 ) );
			    if( $3<0 || $3 >= FIELD ){
				/* "illegal field size" */
				UERROR( MESSAGE( 56 ) );
				$3 = 1;
				}
			    defid( tymerge($<nodep>0,$1), FIELD|$3 );
			    $$ = NIL;
#ifdef BROWSER
			if (cb_ccom_maybe_write != NULL) {
				switch (cb_ccom_maybe_write->name_ref.usage) {
				default:
					cb_ccom_adjust_tag(cb_ccom_maybe_write,
							   cb_c_def_struct_field_bitfield);
					break;
				case cb_c_def_struct_field_hidden:
					cb_ccom_adjust_tag(cb_ccom_maybe_write,
							   cb_c_def_struct_field_bitfield_hidden);
					break;
				}
			}
#endif
			    }
		|  COLON con_e
			%prec CM
			/* "field outside of structure" */
			={  if( !(instruct&INSTRUCT) ) UERROR( MESSAGE( 38 ) );
			    falloc( stab[0], $2, -1, $<nodep>0 );  /* alignment or hole */
			    $$ = NIL;
			    }
		|  error
			={  $$ = NIL; }
		;

		/* int (a)();   is not a function --- sorry! */
nfdeclarator:	   MUL nfdeclarator		
			={  umul:
				$$ = bdty( UNARY MUL, $2, 0 ); }
		|  nfdeclarator  LP   RP		
			={  uftn:
				$$ = bdty( UNARY CALL, $1, 0 );  }
		|  nfdeclarator LB RB		
			={  uary:
				$$ = bdty( LB, $1, 0 );  }
		|  nfdeclarator LB con_e RB	
			={  bary:
				/* "zero or negative subscript" */
				if( (int)$3 <= 0 ) WERROR( MESSAGE( 119 ) );
				$$ = bdty( LB, $1, $3 );  }
		|  NAME  		
			={
#ifdef BROWSER
			struct symtab *p = STP($1.id);
			int ccom_in_struct = cb_ccom_in_struct == NULL ||
			  array_size(cb_ccom_in_struct) == 0 ? 0:
			  array_fetch(cb_ccom_in_struct,
				      array_size(cb_ccom_in_struct)-1);
			if (ccom_in_struct != 0) {
				cb_enter_name_mark_hidden(p,
					$1.lineno,
					ccom_in_struct == INSTRUCT ?
						cb_c_def_struct_field :
						cb_c_def_union_field,
					ccom_in_struct == INSTRUCT ?
						cb_c_def_struct_field_hidden :
						cb_c_def_union_field_hidden,
					cb_ccom_maybe_write);
			} else {
			    switch (curclass) {
			    case TYPEDEF:
				cb_enter_name_mark_hidden(p,
						$1.lineno,
						cb_c_def_typedef,
						cb_c_def_typedef_hidden,
						cb_ccom_maybe_write);
				break;
			    case EXTERN:
				if (blevel > 0)
					cb_enter_name_mark_hidden(p,
						$1.lineno,
						cb_c_def_var_local_extern,
						cb_c_def_var_local_extern_hidden,
						cb_ccom_maybe_write)
				else
					cb_enter_name_mark_hidden(p,
						$1.lineno,
						cb_c_def_var_toplevel_extern,
						cb_c_def_var_toplevel_extern_hidden,
						cb_ccom_maybe_write);
				break;
			    case STATIC:
				if (blevel > 0)
					cb_enter_name_mark_hidden(p,
						$1.lineno,
						cb_c_def_var_local_static,
						cb_c_def_var_local_static_hidden,
						cb_ccom_maybe_write)
				else
					cb_enter_name_mark_hidden(p,
						$1.lineno,
						cb_c_def_var_toplevel_static,
						cb_c_def_var_toplevel_static_hidden,
						cb_ccom_maybe_write);
				break;
			    case REGISTER:
				switch (blevel) {
				case 1:
					cb_enter_name_mark_hidden(p,
						$1.lineno,
						cb_c_def_var_formal_register,
						cb_c_def_var_formal_register_hidden,
						cb_ccom_maybe_write)
					break;
				default:
					cb_enter_name_mark_hidden(p,
						$1.lineno,
						cb_c_def_var_local_register,
						cb_c_def_var_local_register_hidden,
						cb_ccom_maybe_write)
					break;
				}
				break;
			    default:
				switch (blevel) {
				case 0:
					cb_enter_name_mark_hidden(p,
						$1.lineno,
						cb_c_def_var_global,
						cb_c_def_var_global_hidden,
						cb_ccom_maybe_write);
					break;
				case 1:
					cb_enter_name_mark_hidden(p,
						$1.lineno,
						cb_c_def_var_formal_auto,
						cb_c_def_var_formal_auto_hidden,
						cb_ccom_maybe_write)
					break;
				default:
					cb_enter_name_mark_hidden(p,
						$1.lineno,
						cb_c_def_var_local,
						cb_c_def_var_local_hidden,
						cb_ccom_maybe_write)
					break;
				}
				break;
			    }
			}
			$$ = bdty( NAME, NIL, $1.id );
#else
			$$ = bdty( NAME, NIL, $1 );
			/*CXREF def($1, lineno); */
#endif
			}
		|   LP  nfdeclarator  RP 		
			={ $$=$2; }
		;
fdeclarator:	   MUL fdeclarator
			={  goto umul; }
		|  fdeclarator  LP   RP
			={  goto uftn; }
		|  fdeclarator LB RB
			={  goto uary; }
		|  fdeclarator LB con_e RB
			={  goto bary; }
		|   LP  fdeclarator  RP
			={ $$ = $2; }
		|  name_lp  name_list  RP
			={
				/* "function declaration in bad context" */
				if( blevel!=0 ) UERROR(MESSAGE( 44 ));
				$$ = bdty( UNARY CALL, bdty(NAME,NIL,$1), 0 );
				stwart = 0;
				}
		|  name_lp RP
			={
				$$ = bdty( UNARY CALL, bdty(NAME,NIL,$1), 0 );
				stwart = 0;
				}
		;

name_lp:	  NAME LP
			={
				/* turn off typedefs for argument names */
				struct symtab *sp;
				/*CXREF newf($1, lineno); def($1, lineno); */
				stwart = SEENAME;
#ifdef BROWSER
				sp = STP($1.id);
				if( sp != NULL && sp->sclass == SNULL )
				    sp->stype = FTN;
				switch (curclass) {
				    case EXTERN:
					if (blevel == 0) {
						cb_enter_name_mark_hidden(sp,
							$1.lineno,
							cb_c_def_toplevel_extern_func_wo_body,
							cb_c_def_toplevel_extern_func_wo_body_hidden,
							cb_ccom_func);
					} else {
						cb_enter_name_hidden(sp,
							$1.lineno,
							cb_c_def_local_extern_func,
							cb_c_def_local_extern_func_hidden);
						cb_ccom_func = NULL;
					}
					break;
				    case STATIC:
					if (blevel == 0) {
						cb_enter_name_mark_hidden(sp,
							$1.lineno,
							cb_c_def_toplevel_static_func_wo_body,
							cb_c_def_toplevel_static_func_wo_body_hidden,
							cb_ccom_func);
					} else {
						cb_enter_name_mark_hidden(sp,
							$1.lineno,
							cb_c_def_local_static_func,
							cb_c_def_local_static_func_hidden,
							cb_ccom_func);
					}
					break;
				    case FORTRAN:
					if (blevel == 0) {
						cb_enter_name_mark_hidden(sp,
							$1.lineno,
							cb_sun_c_def_global_fortran_func_wo_body,
							cb_sun_c_def_global_fortran_func_wo_body_hidden,
							cb_ccom_func);
					} else {
						cb_enter_name_mark(sp,
							$1.lineno,
							cb_sun_c_def_local_fortran_func,
							cb_ccom_func);
					}
					break;
				    default:
					if (blevel == 0) {
						cb_enter_name_mark_hidden(sp,
							$1.lineno,
							cb_c_def_global_func_wo_body,
							cb_c_def_global_func_wo_body_hidden,
							cb_ccom_func);
					} else {
						cb_enter_name_mark_hidden(sp,
							$1.lineno,
							cb_c_def_local_func,
							cb_c_def_local_func_hidden,
							cb_ccom_func);
					}
					break;
				}
				if ( blevel == 0 ) {
					cb_enter_function_start(sp->sname,
								lineno);
				}
#else
				sp = STP($1);
				if( sp != NULL && sp->sclass == SNULL )
				    sp->stype = FTN;
#endif
				}
		;
name_list:	   NAME
			={
#ifdef BROWSER
				cb_enter_name_hidden(STP($1.id),
						$1.lineno,
						cb_c_def_var_formal_list,
						cb_c_def_var_formal_list_hidden);
				ftnarg( $1.id );  stwart = SEENAME;
#else
				ftnarg( $1 );  stwart = SEENAME;
				/*CXREF ref($1, lineno); */
#endif
			}
		|  name_list  CM  NAME
			={
#ifdef BROWSER
				cb_enter_name_hidden(STP($3.id),
						$3.lineno,
						cb_c_def_var_formal_list,
						cb_c_def_var_formal_list_hidden);
				ftnarg( $3.id );  stwart = SEENAME;
#else
				ftnarg( $3 );  stwart = SEENAME;
				/*CXREF ref($3, lineno); */
#endif
			}
		|  error
		;
		/* always preceeded by attributes: thus the $<nodep>0's */
init_dcl_list:	   init_declarator
			%prec CM
		|  init_dcl_list  CM {$<nodep>$=$<nodep>0;}  init_declarator
		;
		/* always preceeded by attributes */
xnfdeclarator:	   nfdeclarator
			={  defid( $1 = tymerge($<nodep>0,$1), curclass);
			    beginit($1->tn.rval);
			    }
		|  error
		;
		/* always preceeded by attributes */
init_declarator:   nfdeclarator
			={  nidcl( tymerge($<nodep>0,$1) ); }
		|  fdeclarator
			={  noargs(); defid( tymerge($<nodep>0,$1), uclass(curclass) );
			}
		|  xnfdeclarator ASSIGN e
			%prec CM
			={  
#ifndef LINT
                            psline(lineno);
#endif
#ifdef BROWSER
			    doinit( $3.nodep );
			    if (cb_ccom_maybe_write != NULL)
				cb_ccom_read2write(
					cb_ccom_alloc(cb_name,
							cb_ccom_maybe_write));
			    cb_ccom_read2vector_addrof($3.expr);
#else
			    doinit( $3 );
#endif
			    endinit();
			}
		|  xnfdeclarator ASSIGN LC
			{
#ifdef BROWSER
				cb_ccom_push_maybe_write();
#endif
			}
		init_list optcomma RC
			={  endinit();
#ifdef BROWSER
			    cb_ccom_pop_maybe_write();
			    if (cb_ccom_maybe_write != NULL)
			      cb_ccom_read2write(cb_ccom_alloc(cb_name,
							       cb_ccom_maybe_write));
#endif
			}
		|  error
			={  endinit(); }
		;

init_list:	   initializer
			%prec CM
		|  init_list  CM  initializer
		;
initializer:	   e
			%prec CM
			={
#ifdef BROWSER
			doinit( $1.nodep );
			if (cb_ccom_maybe_write != NULL)
				cb_ccom_read2write(cb_ccom_alloc(cb_name,
							cb_ccom_maybe_write));
			cb_ccom_read2vector_addrof($1.expr);
#else
			doinit( $1 );
#endif
			}
		|  ibrace init_list optcomma RC
			={  irbrace(); }
		;

optcomma	:	/* VOID */
		|  CM
		;

optsemi		:	/* VOID */
		|  SM
		;

ibrace		: LC
			={  ilbrace(); }
		;

/*	STATEMENTS	*/

compoundstmt:	   dcmpstmt
		|  cmpstmt
		;

dcmpstmt:	   begin r_dcl_stat_list stmt_list RC
			={  
#ifndef LINT
			    prcstab(blevel);
#endif
			    --blevel;
			    /*CXREF becode(); */
			    if( blevel == 1 ) blevel = 0;
			    clearst( blevel );
			    checkst( blevel );
			    POP_ALLOCATION(psavbc);
#ifdef BROWSER
			    cb_ccom_block_end(lineno, 1);
#endif
			    }
		;

cmpstmt:	   begin stmt_list RC
			={  --blevel;
			    /*CXREF becode(); */
			    if( blevel == 1 ) blevel = 0;
			    clearst( blevel );
			    checkst( blevel );
			    POP_ALLOCATION(psavbc);
#ifdef BROWSER
			    cb_ccom_block_end(lineno, 0);
#endif
			    }
		;

begin:		  LC
			={  
			    if( blevel == 1 ) dclargs();
			    /*CXREF else if (blevel > 1) bbcode(); */
			    ++blevel;
			    if( !bcstash(2) ) {
				    cerror( "nesting too deep" );
				    }
			    PUSH_ALLOCATION(psavbc);
#ifdef BROWSER
			    cb_ccom_block_start(CB_EX_FOCUS_UNIT_BLOCK_KEY,
						lineno);
#endif
			    }
		;

statement:	   e   SM
			={ 
#ifndef LINT
			    psline(lineno);
#endif
#ifdef BROWSER
			    ecomp( $1.nodep );
			    cb_ccom_func2proc( $1.expr );
#else
			    ecomp( $1 );
#endif
			    }
		|  compoundstmt
		|  ifprefix statement
			={ deflab($1);
			   reached = 1;
			   }
		|  ifelprefix statement
			={  if( $1 != NOLAB ){
				deflab( $1 );
				reached = 1;
				}
			    }
		|  whprefix statement
			={
#ifdef LINT
			    branch( contlab );
#else /*!LINT*/
			    if (loop_test) {
				    /*
				     * loop test duplicated;
				     * continuation point is here, not
				     * at the top of the loop
				     */
				    deflab( contlab );
				    if( flostat & FCONT ) reached = 1;
				    ecomp( buildtree( CBRANCH,
					buildtree( NOT, loop_test, NIL ),
					bcon($1) ) );
			    } else {
				    /*
				     * loop test not duplicated;
				     * continuation point is at loop top
				     */
				    branch( contlab );
			    }
#endif /*!LINT*/
			    deflab( brklab );
			    if( (flostat&FBRK) || !(flostat&FLOOP)) reached = 1;
			    else reached = 0;
			    resetbc(0);
			    }
		|  doprefix statement WHILE  LP  e  RP   SM
			={  
#ifndef LINT
			    psline(lineno);
#endif
			    deflab( contlab );
			    if( flostat & FCONT ) reached = 1;
#ifdef BROWSER
			    ecomp( buildtree( CBRANCH, buildtree( NOT, $5.nodep, NIL ), bcon( $1 ) ) );
			    cb_ccom_read2vector_addrof( $5.expr );
			    cb_ccom_dealloc( $5.expr );
#else
			    ecomp( buildtree( CBRANCH, buildtree( NOT, $5, NIL ), bcon( $1 ) ) );
#endif
			    deflab( brklab );
			    reached = 1;
			    resetbc(0);
			    }
		|  forprefix .e RP statement
			= {
			    deflab( contlab );
			    if (flostat & FCONT) reached = 1;
#ifndef LINT
			    psline( resetlineno());
#endif
#ifdef BROWSER
			    if( $2.nodep ) ecomp( $2.nodep );
			    cb_ccom_read2vector_addrof( $2.expr );
			    cb_ccom_dealloc( $2.expr );
#else
			    if( $2 ) ecomp( $2 );
#endif
#ifdef LINT
			    branch( $1 );
#else /*!LINT*/
			    if (loop_test) {
				    ecomp( buildtree( CBRANCH,
					buildtree( NOT, loop_test, NIL ),
					bcon($1) ) );
			    } else {
				    branch( $1 );
			    }
#endif /*!LINT*/
			    deflab( brklab );
			    if( (flostat&FBRK) || !(flostat&FLOOP) ) reached = 1;
			    else reached = 0;
			    resetbc(0);
			    }
		| switchpart statement
			={
			    if( reached ) branch( brklab );
#ifndef LINT
			    psline( resetlineno());
#endif
			    deflab( $1 );
			    swend();
			    deflab(brklab);
			    if( (flostat&FBRK) || !(flostat&FDEF) ) reached = 1;
			    resetbc(FCONT);
			    }
		|  BREAK  SM
			={  
#ifndef LINT
			    psline(lineno);
#endif
			    /* "illegal break" */
			    if( brklab == NOLAB ) UERROR( MESSAGE( 50 ));
			    else if(reached) branch( brklab );
			    flostat |= FBRK;
			    if( brkflag ) goto rch;
			    reached = 0;
			    }
		|  CONTINUE  SM
			={  
#ifndef LINT
			    psline(lineno);
#endif
			    /* "illegal continue" */
			    if( contlab == NOLAB ) UERROR( MESSAGE( 55 ));
			    else branch( contlab );
			    flostat |= FCONT;
			    goto rch;
			    }
		|  RETURN  SM
			={  
#ifndef LINT
			    psline(lineno);
#endif
			    retstat |= NRETVAL;
			    branch( retlab );
			rch:
			    /* "statement not reached" */
			    if( !reached && !reachflg ) WERROR( MESSAGE( 100 ));
			    reached = 0;
			    reachflg = 0;
			    }
		|  RETURN e  SM
			={  register NODE *temp;
			    struct symtab *sp;
			    TWORD temptype;
			    NODE *retval;
#ifndef LINT
			    psline(lineno);
#endif
			    idname = curftn;
			    temp = buildtree( NAME, NIL, NIL );
			    temp->in.type = DECREF( temp->in.type );
			    temptype = temp->in.type;
			    if(temptype == TVOID) {
				sp = STP(idname);
				/* "void function %s cannot return value" */
				UERROR(MESSAGE( 116 ),
					sp == NULL ? "" : sp->sname);
			    }
#ifdef BROWSER
			    retval = $2.nodep;
			    cb_ccom_read2vector_addrof( $2.expr );
			    cb_ccom_dealloc( $2.expr );
#else
                            retval = $2;
#endif
                            temp = buildtree( RETURN, temp, retval );
			    /* now, we have the type of the RHS correct */
			    temp->in.left->in.op = FREE;
			    temp->in.op = FREE;
			    ecomp( buildtree( FORCE, temp->in.right, NIL ) );
			    retstat |= RETVAL;
			    branch( retlab );
			    reached = 0;
			    reachflg = 0;
			    }
		|  GOTO NAME SM
			={  register NODE *q;
			    struct symtab *sp;
#ifndef LINT
			    psline(lineno);
#endif
			    q = block( FREE, NIL, NIL, INT|ARY, 0, INT );
#ifdef BROWSER
			    q->tn.rval = idname = $2.id;
#else
			    q->tn.rval = idname = $2;
#endif
			    defid( q, ULABEL );
			    sp = STP(idname);
#ifdef BROWSER
			    cb_enter_name(sp, $2.lineno, cb_c_ref_label);
#endif
			    if (sp != NULL) {
				    sp->suse = -lineno;
				    branch( sp->offset );
				    /*CXREF ref($2, lineno); */
				    }
			    goto rch;
			    }
		|   SM
		|   ASM LP STRING RP SM
			={  
#ifdef BROWSER
			    cb_ccom_enter_string(cb_sun_c_asm_string,
						$3,
						cb_ccom_string);
#endif
			    asmout();
#ifndef LINT
			    psline(lineno);
#endif
			    }
		|  error  SM
		|  error RC
		|  error compoundstmt
		|  label statement
		;
label:		   NAME COLON
			={  register NODE *q;
#ifdef BROWSER
			    cb_enter_name_hidden(STP($1.id),
					$1.lineno,
					cb_c_def_label,
					cb_c_def_label_hidden);
#endif
			    q = block( FREE, NIL, NIL, INT|ARY, 0, LABEL );
#ifdef BROWSER
			    q->tn.rval = $1.id;
#else
			    q->tn.rval = $1;
#endif
			    defid( q, LABEL );
			    reached = 1;
			    /*CXREF def($1, lineno); */
			    }
		|  CASE e COLON
			={
#ifdef BROWSER
			    addcase($2.nodep);
			    cb_ccom_read2vector_addrof($2.expr);
			    cb_ccom_dealloc($2.expr);
#else
			    addcase($2);
#endif
			    reached = 1;
			    }
		|  DEFAULT COLON
			={  reached = 1;
			    adddef();
			    flostat |= FDEF;
			    }
		;
doprefix:	DO
			={  
#ifndef LINT
			    psline(lineno);
#endif
			    savebc();
			    /* "loop not entered at top" */
			    if( !reached ) WERROR( MESSAGE( 75 ));
			    brklab = getlab();
			    contlab = getlab();
			    deflab( $$ = getlab() );
			    reached = 1;
			    }
		;
ifprefix:	IF LP e RP
			={  
#ifndef LINT
			    psline(lineno);
#endif
#ifdef BROWSER
			    ecomp( buildtree( CBRANCH, $3.nodep, bcon( $$=getlab()) ) ) ;
			    cb_ccom_read2vector_addrof($3.expr);
			    cb_ccom_dealloc($3.expr);
#else
			    ecomp( buildtree( CBRANCH, $3, bcon( $$=getlab()) ) ) ;
#endif
			    reached = 1;
			    }
		| IF LP error
			={ /* no use compiling expression */
			   /* but we do need to define a label */
			   $$ = getlab();
			   reached = 1;
			   }
		;
ifelprefix:	  ifprefix statement ELSE
			={  if( reached ) branch( $$ = getlab() );
			    else $$ = NOLAB;
			    deflab( $1 );
			    reached = 1;
			    }
		;

whprefix:	  WHILE  LP  e  RP
			={  
#ifndef LINT
			    psline(lineno);
#endif
			    savebc();
			    if (!reached)
				/* "loop not entered at top" */
				WERROR(MESSAGE(75));
#ifndef LINT
#    ifdef BROWSER
			    loop_test = $3.nodep;
#    else
			    loop_test = $3;
#    endif
#endif /*!LINT*/
#ifdef BROWSER
			    if( $3.nodep->in.op == ICON && $3.nodep->tn.lval != 0 ) flostat = FLOOP;
			    cb_ccom_read2vector_addrof($3.expr);
			    cb_ccom_dealloc( $3.expr );
#else
			    if( $3->in.op == ICON && $3->tn.lval != 0 ) flostat = FLOOP;
#endif
			    contlab = getlab();
			    reached = 1;
			    brklab = getlab();
#ifdef LINT
			    deflab( contlab );
#else /*!LINT*/
			    if (can_duplicate(loop_test)) {
				    /* make duplicate of loop test */
				    loop_test = pcc_tcopy(loop_test);
			    } else {
				    /*
				     * do not duplicate loop test;
				     * continuation label and loop top
				     * label are synonymous
				     */
				    deflab( contlab );
				    loop_test = NIL;
			    }
#endif /*!LINT*/
#ifdef BROWSER
			    if( flostat == FLOOP ) tfree( $3.nodep );
			    else ecomp( buildtree( CBRANCH, $3.nodep, bcon( brklab) ) );
#else
			    if( flostat == FLOOP ) tfree( $3 );
			    else ecomp( buildtree( CBRANCH, $3, bcon( brklab) ) );
#endif
#ifndef LINT
			    if (loop_test) {
				/*
				 * loop test is duplicated at bottom -
				 * emit loop top label here.  Note that
				 * loop top and continuation label are
				 * different.
				 */
				deflab( $$ = getlab() );
			    }
#endif /*!LINT*/
			}
		| WHILE LP error
		    ={ /* don't compile expression, but try other semantics */
			    savebc();
			    if( !reached )
					/* "loop not entered at top" */
					WERROR(MESSAGE(75));
			    deflab( contlab = getlab() );
			    reached = 1;
			    brklab = getlab();
			    }
		;
forprefix:	  FOR  LP  .e  SM .e  SM 
			={  
#ifndef LINT
			    psline(lineno);
#endif
#ifdef BROWSER
			    if( $3.nodep ) ecomp( $3.nodep );
			    else if (!reached)
					/* "loop not entered at top" */
					WERROR(MESSAGE(75));
			    cb_ccom_read2vector_addrof( $3.expr );
			    cb_ccom_dealloc( $3.expr );
			    cb_ccom_read2vector_addrof( $5.expr );
			    cb_ccom_dealloc( $5.expr );
#else
			    if( $3 ) ecomp( $3 );
			    else if (!reached)
					/* "loop not entered at top" */
					WERROR(MESSAGE(75));
#endif
			    savebc();
#ifndef LINT
			    savelineno();
#   ifdef BROWSER
			    loop_test = $5.nodep;
#   else
			    loop_test = $5;
#   endif
#endif /*!LINT*/
			    contlab = getlab();
			    brklab = getlab();
#ifdef LINT
			    deflab( $$ = getlab() );
#else /*!LINT*/
			    if (can_duplicate(loop_test)) {
				    loop_test = pcc_tcopy(loop_test);
			    } else {
				    /* do not duplicate loop test */
				    deflab( $$ = getlab() );
				    loop_test = NIL;
			    }
#endif /*!LINT*/
			    reached = 1;
#ifdef BROWSER
			    if( $5.nodep ) ecomp( buildtree( CBRANCH, $5.nodep, bcon( brklab) ) );
			    else flostat |= FLOOP;
			    cb_ccom_dealloc( $5.expr );
#else
			    if( $5 ) ecomp( buildtree( CBRANCH, $5, bcon( brklab) ) );
			    else flostat |= FLOOP;
#endif
#ifndef LINT
			    if (loop_test) {
				    deflab( $$=getlab() );
			    }
#endif /*!LINT*/
			}
		| FOR LP error
			={ /* do some semantics anyway */
			    savebc();
#ifndef LINT
			    savelineno();
#endif
			    contlab = getlab();
			    brklab = getlab();
			    deflab( $$ = getlab() );
			    reached = 1;
			    }
		;
switchpart:	   SWITCH  LP  e  RP
			={
			    NODE *q;
#ifndef LINT
			    psline(lineno);
#endif
			    savebc();
#ifndef LINT
			    savelineno();
#endif
			    brklab = getlab();
#ifdef BROWSER
			    if(!isscalar($3.nodep->in.type)) {
				/* "switch expression must have scalar type" */
				UERROR( MESSAGE( 130 ));
			    }
#ifdef LINT
			    /*
			     * "compile" the expression, which for lint means
			     * recording uses and defs
			     */
			    ecomp(buildtree(FORCE, makety($3.nodep,INT,0,INT), NIL));
			    q = NIL;
#else !LINT
			    /*
			     * stash laundered tree for genswitch
			     */
			    q = optim(makety($3.nodep,INT,0,INT));
#endif
			    cb_ccom_read2vector_addrof($3.expr);
			    cb_ccom_dealloc($3.expr);
#else
			    if(!isscalar($3->in.type)) {
				/* "switch expression must have scalar type" */
				UERROR( MESSAGE( 130 ));
			    }
#ifdef LINT
			    /*
			     * "compile" the expression, which for lint means
			     * recording uses and defs
			     */
			    ecomp(buildtree(FORCE, makety($3,INT,0,INT), NIL));
			    q = NIL;
#else !LINT
			    /*
			     * stash laundered tree for genswitch
			     */
			    q = optim(makety($3,INT,0,INT));
#endif
#endif
			    branch( $$ = getlab() );
			    swstart(q);
			    reached = 0;
			    }
		;
/*	EXPRESSIONS	*/
con_e:		   { 
			$<intval>$=instruct; stwart=instruct=0; } e
			%prec CM
			={
#ifdef BROWSER
			$$ = icons( $2.nodep );  instruct=$<intval>1;
			cb_ccom_dealloc( $2.expr );
#else
			$$ = icons( $2 );  instruct=$<intval>1;
#endif
			}
		;
.e:		   e
		|
			={
#ifdef BROWSER
			$$.nodep = 0; $$.expr = 0;
#else
			$$=0;
#endif
			}
		;
elist:		   e
			%prec CM
		|  elist  CM  e
			={
#ifdef BROWSER
			$$ = $3;
#endif
			goto bop; }
		;

e:		   e RELOP e
			={
			preconf:
			    if( yychar==RELOP||yychar==EQUOP||yychar==AND||yychar==OR||yychar==ER ){
			    precplaint:
				/* "precedence confusion possible: parenthesize!" */
				if( hflag ) WERROR( MESSAGE( 92 ) );
				}
			bop:
#ifdef BROWSER
			    $$.nodep = buildtree( $2, $1.nodep, $3.nodep );
			    $$.expr = cb_ccom_alloc(cb_binary, $1.expr, $3.expr);
#else
			    $$ = buildtree( $2, $1, $3 );
#endif
			    }
		|  e CM e
			={  $2 = COMOP;
			    goto bop;
			    }
		|  e DIVOP e
			={  goto bop; }
		|  e PLUS e
			={  if(yychar==SHIFTOP) goto precplaint; else goto bop; }
		|  e MINUS e
			={  if(yychar==SHIFTOP ) goto precplaint; else goto bop; }
		|  e SHIFTOP e
			={  if(yychar==PLUS||yychar==MINUS) goto precplaint; else goto bop; }
		|  e MUL e
			={  goto bop; }
		|  e EQUOP  e
			={  goto preconf; }
		|  e AND e
			={  if( yychar==RELOP||yychar==EQUOP ) goto preconf;  else goto bop; }
		|  e OR e
			={  if(yychar==RELOP||yychar==EQUOP) goto preconf; else goto bop; }
		|  e ER e
			={  if(yychar==RELOP||yychar==EQUOP) goto preconf; else goto bop; }
		|  e ANDAND e
			={  goto bop; }
		|  e OROR e
			={  goto bop; }
		|  e MUL ASSIGN e
			={  abop:
#ifdef BROWSER
				$$.nodep = buildtree( ASG $2, $1.nodep, $4.nodep );
				cb_ccom_dealloc($4.expr);
				cb_ccom_read2read_write($1.expr);
				$$.expr = 0;
#else
				$$ = buildtree( ASG $2, $1, $4 );
#endif
				}
		|  e DIVOP ASSIGN e
			={  goto abop; }
		|  e PLUS ASSIGN e
			={  goto abop; }
		|  e MINUS ASSIGN e
			={  goto abop; }
		|  e SHIFTOP ASSIGN e
			={  goto abop; }
		|  e AND ASSIGN e
			={  goto abop; }
		|  e OR ASSIGN e
			={  goto abop; }
		|  e ER ASSIGN e
			={  goto abop; }
		|  e QUEST e COLON e
			={
#ifdef BROWSER
			$$.nodep=buildtree(QUEST, $1.nodep, buildtree( COLON, $3.nodep, $5.nodep ) );
			$$.expr=NULL;
			cb_ccom_dealloc( $1.expr );
			cb_ccom_dealloc( $3.expr );
			cb_ccom_dealloc( $5.expr );
#else
			$$=buildtree(QUEST, $1, buildtree( COLON, $3, $5 ) );
#endif
			    }
		|  e ASSIGN e
			={
#ifdef BROWSER
			$$.nodep = buildtree($2, $1.nodep, $3.nodep);
			cb_ccom_read2write($1.expr);
			cb_ccom_read2vector_addrof($3.expr);
			$$.expr = 0;
#else
			goto bop;
#endif
			}
		|  term
		;
term:		   term INCOP
			={
#ifdef BROWSER
			$$.nodep = buildtree( $2, $1.nodep, bcon(1) );
			cb_ccom_read2read_write($1.expr);
			$$.expr = 0;
#else
			$$ = buildtree( $2, $1, bcon(1) );
#endif
			}
		|  MUL term
			={ ubop:
#ifdef BROWSER
			    $$.expr = $2.expr;
			    cb_ccom_name2other($$.expr, cb_name_point);
			    $$.nodep = buildtree( UNARY $1, $2.nodep, NIL );
#else
			    $$ = buildtree( UNARY $1, $2, NIL );
#endif
			    }
		|  AND term
			={
#ifdef BROWSER
			cb_ccom_read2addrof($2.expr);
			if (hidden_strop($2.nodep)) {
				/* "unacceptable operand of &" */
				UERROR( MESSAGE( 110 ) );
				}
			if( ISFTN($2.nodep->in.type) || ISARY($2.nodep->in.type) ){
				/* "& before array or function: ignored" */
				WERROR( MESSAGE( 7 ) );
				$$ = $2;
				}
#else
			if (hidden_strop($2)) {
				/* "unacceptable operand of &" */
				UERROR( MESSAGE( 110 ) );
				}
			if( ISFTN($2->in.type) || ISARY($2->in.type) ){
				/* "& before array or function: ignored" */
				WERROR( MESSAGE( 7 ) );
				$$ = $2;
				}
#endif
			    else goto ubop;
			    }
		|  MINUS term
			={  
#ifdef BROWSER
				cb_ccom_dealloc($2.expr);
				$2.expr = NULL;
#endif
				goto ubop; }
		|  UNOP term
			={
#ifdef BROWSER
			    $$.nodep = buildtree( $1, $2.nodep, NIL );
			    $$.expr = $2.expr;
#else
			    $$ = buildtree( $1, $2, NIL );
#endif
			    }
		|  INCOP term
			={
#ifdef BROWSER
			$$.nodep = buildtree( $1==INCR ? ASG PLUS : ASG MINUS,
						$2.nodep,
						bcon(1)  );
			cb_ccom_read2read_write($2.expr);
			$$.expr = 0;
#else
			$$ = buildtree( $1==INCR ? ASG PLUS : ASG MINUS,
						$2,
						bcon(1)  );
#endif
			    }
		|  SIZEOF term
			={
#ifdef BROWSER
			$$.nodep = doszof( $2.nodep );
			cb_ccom_read2sizeof($2.expr);
			$$.expr = NULL;
#else
			$$ = doszof( $2 );
#endif
			}
		|  LP cast_type RP term  %prec INCOP

			={
#ifdef BROWSER
			    $$.nodep = buildtree( CAST, $2, $4.nodep );
			    $$.nodep->in.left->in.op = FREE;
			    $$.nodep->in.op = FREE;
			    $$.nodep = $$.nodep->in.right;
			    $$.expr = $4.expr;
#else
			    $$ = buildtree( CAST, $2, $4 );
			    $$->in.left->in.op = FREE;
			    $$->in.op = FREE;
			    $$ = $$->in.right;
#endif
			    }
		|  SIZEOF LP cast_type RP  %prec SIZEOF
			={
#ifdef BROWSER
			$$.nodep = doszof( $3 );
			$$.expr = 0;
			cb_ccom_cast2sizeof(cb_ccom_maybe_cast[0]);
			cb_ccom_cast2sizeof(cb_ccom_maybe_cast[1]);
			cb_ccom_cast2sizeof(cb_ccom_maybe_cast[2]);
#else
			$$ = doszof( $3 );
#endif
			}
		|  term LB e RB
			={
#ifdef BROWSER
			$$.nodep = buildtree( UNARY MUL, buildtree( PLUS, $1.nodep, $3.nodep ), NIL );
			$$.expr = $1.expr;
			cb_ccom_vector_name2name($$.expr);
			cb_ccom_dealloc($3.expr);
#else
			$$ = buildtree( UNARY MUL, buildtree( PLUS, $1, $3 ), NIL );
#endif
			}
		|  funct_idn  RP
			={
#ifdef BROWSER
			$$.nodep=buildtree(UNARY CALL,$1.nodep,NIL);
			if ($1.expr)
			  $$.expr = cb_ccom_alloc(cb_name, $1.expr);
#else
			$$=buildtree(UNARY CALL,$1,NIL);
#endif
			}
		|  funct_idn elist  RP
			={
#ifdef BROWSER
			$$.nodep=buildtree(CALL,$1.nodep,$2.nodep);
			cb_ccom_read2vector_addrof($2.expr);
			if ($1.expr)
			  $$.expr = cb_ccom_alloc(cb_name, $1.expr);
#else
			$$=buildtree(CALL,$1,$2);
#endif
			}
		|  term STROP NAME
			={
#ifdef BROWSER
			if( $2 == DOT ){
				if( notstref( $1.nodep ) ) {
				    /* "structure reference must be addressable" */
				    UERROR(MESSAGE( 105 ));
				    if ($1.nodep == NIL) {
					$1.nodep = bcon(0);
					$1.nodep->tn.type = (PTR|INT);
					$1.nodep = buildtree(UNARY MUL, $1.nodep, NIL);
					}
				    }
				$1.nodep = buildtree( UNARY AND, $1.nodep, NIL );
			    }
			    cb_ccom_expr = $1.expr;
			    idname = $3.id;
			    cb_ccom_line_no = $3.lineno;
			    $$.nodep = buildtree( STREF, $1.nodep, buildtree( NAME, NIL, NIL ) );
			    $$.expr = cb_ccom_expr;
			    if( $2 == DOT ) {
				    if ((cb_ccom_expr != NULL) &&
					(cb_ccom_expr->op == cb_binary) &&
					(cb_ccom_expr->e2 != NULL) &&
					(cb_ccom_expr->e2->op ==
					 cb_name_vector)) {
					    cb_ccom_name2other($1.expr,
							       cb_name_vector);
				    } else {
					    cb_ccom_name2other($1.expr,
							       cb_name_dot);
				    }
			    } else {
				    cb_ccom_name2other($1.expr, cb_name_point);
			    }
#else
			if( $2 == DOT ){
				if( notstref( $1 ) ) {
				    /* "structure reference must be addressable" */
				    UERROR(MESSAGE( 105 ));
				    if ($1 == NIL) {
					$1 = bcon(0);
					$1->tn.type = (PTR|INT);
					$1 = buildtree(UNARY MUL, $1, NIL);
					}
				    }
				$1 = buildtree( UNARY AND, $1, NIL );
				}
			    idname = $3;
			    $$ = buildtree( STREF, $1, buildtree( NAME, NIL, NIL ) );
			    /*CXREF ref($3, lineno); */
#endif
			    }
		|  NAME
			={  struct symtab *sp;
#ifdef BROWSER
			    idname = $1.id;
#else
			    idname = $1;
#endif
			    sp = STP(idname);
#ifdef BROWSER
			    $$.expr = term_name(sp, $1.lineno);
#endif
			    if (sp != NULL && sp->sclass == LABEL)
				/* Label used in expression */
				UERROR( MESSAGE(129) );

			    /* recognize identifiers in initializations */
			    if(blevel==0 && sp != NULL && sp->stype == UNDEF) {
				register NODE *q;
				/* "undeclared initializer name %s" */
				WERROR( MESSAGE( 111 ), sp->sname );
				q = block(FREE, NIL, NIL, INT, 0, INT);
				q->tn.rval = idname;
				defid( q, EXTERN );
				}
#ifdef BROWSER
			    $$.nodep=buildtree(NAME,NIL,NIL);
			    sp = STP($1.id);
#else
			    $$=buildtree(NAME,NIL,NIL);
			    sp = STP($1);
#endif
			    if (sp != NULL) {
				sp->suse = -lineno;
				/*CXREF ref($1, lineno); */
			    }
			}
		|  ICON
			={
#ifdef BROWSER
			    $$.nodep=bcon(0);
			    $$.expr = 0;
			    $$.nodep->tn.lval = lastcon;
			    $$.nodep->tn.rval = NONAME;
			    if( $1 ) $$.nodep->fn.csiz = $$.nodep->in.type = ctype(LONG);
#else
			    $$=bcon(0);
			    $$->tn.lval = lastcon;
			    $$->tn.rval = NONAME;
			    if( $1 ) $$->fn.csiz = $$->in.type = ctype(LONG);
#endif
			    }
		|  FCON
			={
#ifdef BROWSER
			    $$.nodep=buildtree(FCON,NIL,NIL);
			    $$.expr = 0;
			    $$.nodep->fpn.dval = dcon;
#else
			    $$=buildtree(FCON,NIL,NIL);
			    $$->fpn.dval = dcon;
#endif
			    }
		|  STRING
			={
#ifdef BROWSER
			cb_ccom_enter_string(cb_c_regular_string,
					$1,
					cb_ccom_string);
			$$.nodep = getstr(); /* get string contents */
			$$.expr = 0;
#else
			$$ = getstr(); /* get string contents */
#endif
			}
		|   LP  e  RP
			={ $$=$2; }
		;

cast_type:	  type null_decl
			={
			$$ = tymerge( $1, $2 );
			$$->in.op = NAME;
			$1->in.op = FREE;
#ifdef BROWSER
			cb_ccom_decl2cast(cb_ccom_maybe_cast[0]);
			cb_ccom_decl2cast(cb_ccom_maybe_cast[1]);
			cb_ccom_decl2cast(cb_ccom_maybe_cast[2]);
#endif
			}
		;

null_decl:	   /* empty */
			={ $$ = bdty( NAME, NIL, -1 ); }
		|  LP RP
			={ $$ = bdty( UNARY CALL, bdty(NAME,NIL,-1),0); }
		|  LP null_decl RP LP RP
			={  $$ = bdty( UNARY CALL, $2, 0 ); }
		|  MUL null_decl
			={  goto umul; }
		|  null_decl LB RB
			={  goto uary; }
		|  null_decl LB con_e RB
			={  goto bary;  }
		|  LP null_decl RP
			={ $$ = $2; }
		;

funct_idn:	   NAME  LP 
			={  struct symtab *sp;
#ifdef BROWSER
			    sp = STP($1.id);
			    if ( cb_flag )
				if ((sp->stype & ~PCC_BTMASK) == 0) {
					$$.expr = (Cb_expr)cb_enter_symbol_hidden(
						sp->sname,
						$1.lineno,
						cb_c_def_global_func_implicit,
						cb_c_def_global_func_implicit_hidden);
					cb_ccom_regular_call(sp->sname, $1.lineno);
				} else if (ISPTR(sp->stype)) {
					$$.expr = (Cb_expr)cb_enter_symbol_hidden(
						sp->sname,
						$1.lineno,
						cb_c_ref_func_call_using_var,
						cb_c_ref_func_call_using_var_hidden);
					cb_ccom_var_call(sp->sname, $1.lineno);
				} else {
					$$.expr = (Cb_expr)cb_enter_symbol_hidden(
						sp->sname,
						$1.lineno,
						cb_c_ref_func_call,
						cb_c_ref_func_call_hidden);
					cb_ccom_regular_call(sp->sname, $1.lineno);
				}
#else
			    sp = STP($1);
#endif
			    if( sp != NULL && sp->stype == UNDEF ){
				register NODE *q;
				q = block(FREE, NIL, NIL, FTN|INT, 0, INT);
#ifdef BROWSER
				q->tn.rval = $1.id;
#else
				q->tn.rval = $1;
#endif
				defid( q, EXTERN );
				}
#ifdef BROWSER
			    idname = $1.id;
			    $$.nodep=buildtree(NAME,NIL,NIL);
#else
			    idname = $1;
			    $$=buildtree(NAME,NIL,NIL);
#endif

			    sp = STP(idname);
			    if (sp != NULL) {
				sp->suse = -lineno;
				/*CXREF ref($1, lineno); */
			    }
			}
		|  term  LP 
		;
%%

NODE *
mkty( t, d, s ) unsigned t; {
	return( block( TYPE, NIL, NIL, t, d, s ) );
	}

NODE *
bdty( op, p, v ) NODE *p; {
	register NODE *q;

	q = block( op, p, NIL, INT, 0, INT );

	switch( op ){

	case UNARY MUL:
	case UNARY CALL:
		break;

	case LB:
		q->in.right = bcon(v);
		break;

	case NAME:
		q->tn.rval = v;
		break;

	default:
		cerror( "bad bdty" );
		}

	return( q );
	}

dstash( n )
{
    /* put n into the dimension table */
    extern char *realloc();

    if( (unsigned)curdim % DIMTABSZ == 0 ) {
	dimtab = (int*)realloc((char *)dimtab,
	    (curdim+DIMTABSZ)*sizeof(dimtab[0]));
	if (dimtab == NULL) {
	    cerror("dimension table overflow");
	}
    }
    dimtab[ curdim++ ] = n;
}


/*
 * check for space on the break-continue stack,
 * reallocating if necessary.  Return 1 if n cells
 * are available, else return 0.
 */
static int
bcstash(n)
{
#define BCSZ 512
	extern char *malloc(), *realloc();
	static unsigned bcsz = BCSZ;
	static count = 0;

	count++;
	if (asavbc == NULL) {
		asavbc = (int*)malloc(bcsz*sizeof(asavbc[0]));
		if (asavbc == NULL) return(0);
		psavbc = asavbc;
	} else if (psavbc + n >= &asavbc[bcsz]) {
		int psavbc_index = psavbc - asavbc;
		bcsz += BCSZ;
		asavbc = (int*)realloc((char*)asavbc, bcsz*sizeof(asavbc[0]));
		if (asavbc == NULL) return(0);
		psavbc = asavbc + psavbc_index;
	}
	return(1);
}

savebc() {
	if( !bcstash(5) ){
		cerror( "whiles, fors, etc. too deeply nested");
		}
	*psavbc++ = brklab;
	*psavbc++ = contlab;
	*psavbc++ = flostat;
	*psavbc++ = swx;
#ifndef LINT
	*psavbc++ = (int)loop_test;
#endif /*!LINT*/
	flostat = 0;
	}

resetbc(mask){
#ifndef LINT
	loop_test = (NODE*)*--psavbc;
#endif /*!LINT*/
	swx = *--psavbc;
	flostat = *--psavbc | (flostat&mask);
	contlab = *--psavbc;
	brklab = *--psavbc;

	}

#ifndef LINT
savelineno(){
	if( !bcstash(1) ){
		cerror( "whiles, fors, etc. too deeply nested");
		}
	*psavbc++ = lineno;
	}

resetlineno(){
	return  *--psavbc;
	}
#endif

static void
more_swtab()
{
    int i;
    i = (swp - swtab);
    if( i % SWITSZ == 0 ){
	if(swtab == NULL) {
	    swtab = (struct sw*)malloc(SWITSZ*sizeof(struct sw));
	} else {
	    swtab = (struct sw*)realloc((char *)swtab,
		(i+SWITSZ)*sizeof(struct sw));
	}
	if (swtab == NULL) {
		fatal("no memory for switch statements");
		/*NOTREACHED*/
	}
	swp = swtab + i;
    }
}

reset_swtab()
{
    if (swtab != NULL) free((char *)swtab);
    swp = swtab = NULL;
}

addcase(p) NODE *p; { /* add case to switch */
  TWORD type;
	p = optim( p );  /* change enum to ints */
	if( p->in.op != ICON || p->tn.rval != NONAME ){
		/* "case expression must be an integer constant" */
		UERROR( MESSAGE( 80 ));
		return;
		}
	type = BTYPE(p->in.type);
	if( ( SZLONG > SZINT ) && ( type == LONG || type == ULONG ) )
		/* long in case or switch statement may be truncated */
		WERROR( MESSAGE( 123 ));
	if( swp == swtab ){
		/* "case not in switch" */
		UERROR( MESSAGE( 20 ));
		return;
		}
	more_swtab();
	swp->sval = p->tn.lval;
	deflab( swp->slab = getlab() );
	++swp;
	tfree(p);
	}

adddef(){ /* add default case to switch */
	more_swtab();
	if( swtab[swx].slab >= 0 ){
		/* "duplicate default in switch" */
		UERROR( MESSAGE( 34 ));
		return;
		}
	if( swp == swtab ){
		/* "default not inside switch" */
		UERROR( MESSAGE( 29 ));
		return;
		}
	deflab( swtab[swx].slab = getlab() );
	}


/*
 * is t a scalar type?
 * (required, e.g., for switch expressions)
 */
isscalar(t)
	TWORD t;
{
	if (t != BTYPE(t)) {
		return ISPTR(t);
	}
	switch(t) {
	case STRTY:
	case UNIONTY:
		return 0;
	default:
		return 1;
	}
}

swstart(p) NODE *p; {
	/* begin a switch block */
	more_swtab();
	swx = swp - swtab;
	swp->slab = -1;
	swp->sval = (int)p;
	++swp;
	}

swend(){ /* end a switch block */

	register struct sw *swbeg, *p, *q, *r, *r1;
	CONSZ temp;
	int tempi;

	swbeg = &swtab[swx+1];	/* point to first case in switch */

	/* sort cases */

	r1 = swbeg;
	r = swp-1;

	while( swbeg < r ){
		/* bubble largest to end */
		for( q=swbeg; q<r; ++q ){
			if( q->sval > (q+1)->sval ){
				/* swap */
				r1 = q+1;
				temp = q->sval;
				q->sval = r1->sval;
				r1->sval = temp;
				tempi = q->slab;
				q->slab = r1->slab;
				r1->slab = tempi;
				}
			}
		r = r1;
		r1 = swbeg;
		}

	/* it is now sorted; check for duplicates */

	for( p = swbeg+1; p<swp; ++p ){
		if( p->sval == (p-1)->sval ){
			/* "duplicate case in switch, %d" */
			UERROR( MESSAGE( 33 ), tempi=p->sval );
			return;
			}
		}

#ifndef LINT
	genswitch( (NODE*)swbeg[-1].sval, swbeg-1, swp-(swbeg-1)  );
#endif
	swp = swbeg-1;
	}

#ifndef LINT
/*
 * Return 1 for an expression that is considered sufficiently
 * small that the space penalty of duplicating it is negligible,
 * assuming that a speed payoff will result from optimizations
 * like linear function test replacement and loop unrolling, which
 * work better on loop bodies that end with a conditional branch
 * instead of branch to a branch.
 *
 * This function is used in the compilation of while loops
 * and for loops.  It is slightly machine dependent, but unless
 * someone comes up with actual machine-dependent variations
 * worth exploiting, it isn't worthwhile to require making N
 * versions of it.
 */
int
can_duplicate(p)
	NODE *p;
{
	if (p != NULL) {
		switch (optype(p->in.op)) {
		case LTYPE:
			return(1);
		case BITYPE:
			return(logop(p->in.op)
			    && optype(p->in.left->in.op) == LTYPE
			    && optype(p->in.right->in.op) == LTYPE);
		case UTYPE:
			return(optype(p->in.left->in.op) == LTYPE);
		}
	}
	return(0);
}
#endif /*!LINT*/

#ifdef BROWSER



cb_enter_type(index, type, typdef, lineno)
	int		index;
	PCC_TWORD	type;
	struct symtab	*typdef;
	int		lineno;
{
	char	*name;
	Cb_semtab_if result = NULL;

	if (cb_flag) {
		if (typdef == NULL) {
			switch (type) {
			case CHAR: name = "char"; break;
			case DOUBLE: name = "double"; break;
			case FLOAT: name = "float"; break;
			case INT: name = "int"; break;
			case LONG: name = "long"; break;
			case SHORT: name = "short"; break;
			case UNSIGNED: name = "unsigned"; break;
			case TVOID: name = "void"; break;
			}
			result = cb_enter_symbol_hidden(name,
					lineno,
					cb_c_ref_builtin_type_decl,
					cb_c_ref_builtin_type_decl_hidden);
		} else {
			result = cb_enter_symbol_hidden(typdef->sname,
					lineno,
					cb_c_ref_typedef_decl,
					cb_c_ref_typedef_decl_hidden);
		}
	}
	cb_ccom_maybe_cast[index] = result;
}

Cb_expr
term_name(sp, lineno)
	struct symtab *sp;
	int	lineno;
{
	Cb_semtab_if	sem = NULL;
	Cb_expr		result;
	static char	*prev_name;

	if (!cb_flag) {
		return NULL;
	}

	if (sp->sclass == MOE) {
		cb_enter_name_mark(sp, lineno, cb_c_ref_enum_member, sem);
		prev_name = sp->sname;
	} else if (PCC_ISFTN(sp->stype)) {
		(void)cb_enter_symbol_hidden(sp->sname,
					     lineno,
					     cb_c_ref_func_constant,
					     cb_c_ref_func_constant_hidden);
		cb_ccom_func_assignment(prev_name, sp->sname, lineno);
	} else {
		cb_enter_name_mark(sp, lineno, cb_c_ref_var_read, sem);
		prev_name = sp->sname;
	}

	if (sem == NULL) {
		result = NULL;
	} else {
		if (PCC_ISARY(sp->stype)) {
			result = cb_ccom_alloc(cb_name_vector,
					       (struct Cb_expr *)sem);
		} else {
			result = cb_ccom_alloc(cb_name, (struct Cb_expr *)sem);
		}
	}
	return result;
}
#endif
