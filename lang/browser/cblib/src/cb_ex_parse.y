/*
 *	@(#)cb_ex_parse.y 1.1 94/10/31 SMI
 *		Parser for the code browser extensibility compiler
 */

/*
 *	Copyright (c) 1989, Sun Microsystems, Inc.  All Rights Reserved.
 *	Sun considers its source code as an unpublished, proprietary
 *	trade secret, and it is available only under strict license
 *	provisions.  This copyright notice is placed here only to protect
 *	Sun in the event the source is deemed a published work.  Dissassembly,
 *	decompilation, or other means of reducing the object code to human
 *	readable form is prohibited by the license agreement under which
 *	this code is provided to the user or company in possession of this
 *	copy.
 *	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *	Government is subject to restrictions as set forth in subparagraph
 *	(c)(1)(ii) of the Rights in Technical Data and Computer Software
 *	clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *	NASA FAR Supplement.
 */

%{
#ifndef array_h_INCLUDED
#include "array.h"
#endif

#ifndef CB_BOOT
#ifndef cb_enter_h_INCLUDED
#include "cb_enter.h"
#endif
#endif

#ifndef cb_extend_h_INCLUDED
#include "cb_extend.h"
#endif

#ifndef CB_BOOT
#ifndef cb_extend_tags_h_INCLUDED
#include "cb_extend_tags.h"
#endif
#endif

#ifndef cb_libc_h_INCLUDED
#include "cb_libc.h"
#endif

#ifndef cb_string_h_INCLUDED
#include "cb_string.h"
#endif

#ifndef CB_BOOT
#define enter_symbol(sym, tag) \
	if (sym.hidden == 0) { \
		(void)cb_enter_symbol(sym.string, \
				      sym.lineno, \
				      (unsigned int)tag); \
	}

#else
#define enter_symbol(sym, tag)
#endif
%}

%token	<lineno>	EX_LP
%token	<lineno>	EX_RP
%token	<lineno>	EX_PLUS
%token	<lineno>	EX_DOT
%token	<lineno>	EX_EQ
%token	<lineno>	EX_LB
%token	<lineno>	EX_OR
%token	<lineno>	EX_RB

%token	<lineno>	EX_COLOR
%token	<lineno>	EX_DEFAULT_EQUIV_LANGUAGE
%token	<lineno>	EX_DEF_FOCUS_UNIT
%token	<lineno>	EX_EQUIV
%token	<lineno>	EX_EX_FILE
%token	<lineno>	EX_FOCUS_MENU_ITEMS
%token	<lineno>	EX_GRAPH
%token	<lineno>	EX_GREP_ABLE
%token	<lineno>	EX_HELP
%token	<lineno>	EX_INF
%token	<lineno>	EX_INSERT_MENU
%token	<lineno>	EX_INVISIBLE
%token	<lineno>	EX_LANGUAGE
%token	<lineno>	EX_LANGUAGE_START_COMMENT
%token	<lineno>	EX_LANGUAGE_END_COMMENT
%token	<lineno>	EX_MAKE_ABSOLUTE
%token	<lineno>	EX_MENU
%token	<lineno>	EX_NODE
%token	<lineno>	EX_NONE
%token	<lineno>	EX_PROPERTIES
%token	<lineno>	EX_REF_FOCUS_UNIT
%token	<lineno>	EX_RENDER
%token	<lineno>	EX_RENDER_SOLID
%token	<lineno>	EX_RENDER_DASHED
%token	<lineno>	EX_RENDER_DOTTED
%token	<lineno>	EX_RENDER_DASH_DOTTED
%token	<lineno>	EX_RENDER_NO_BORDER
%token	<lineno>	EX_RENDER_SQUARE
%token	<lineno>	EX_RENDER_OVAL
%token	<lineno>	EX_SECONDARY
%token	<lineno>	EX_START
%token	<lineno>	EX_STOP
%token	<lineno>	EX_TAGS
%token	<lineno>	EX_TAGS_SET
%token	<lineno>	EX_TAGS_H
%token	<lineno>	EX_USE_REFD
%token	<lineno>	EX_VERSION
%token	<lineno>	EX_WEIGHT
%token	<lineno>	EX_ZERO

%token	<symbol>	EX_SYMBOL
%token	<symbol>	EX_STRING
%token	<integer>	EX_NUMBER

%token	<lineno>	EX_LAST_TOKEN

%type	<array>		ref_props_list def_props_list
%type	<array>		menu_props_list opt_menu_props_list menu_path_list
%type	<array>		tags_item_decl_list
%type	<array>		menu_item_decl_list opt_secondary tags_list
%type	<tags_item>	tags_item_decl def_focus_unit
%type	<symbol>	opt_symbol opt_string opt_help
%type	<integer>	opt_weight
%type	<equiv>		equiv_spec
%type	<menu_item>	menu_item_decl
%type	<props_item>	def_props_item ref_props_item
%type	<n>		opt_invisible opt_use_refd
%type	<startstop>	opt_start opt_stop
%type	<n>		opt_make_absolute opt_grep_able
%type	<symbol>	string
%type	<color>		opt_color
%type	<rendering>	rendering

%union {
	struct Cb_ex_symbol_tag {
		char		*string;
		int		lineno;
		int		hidden;
	}				symbol;
	struct Cb_ex_integer_tag {
		int		value;
		int		lineno;
		int		hidden;
	}				integer;
	int				lineno;
	int				n;
	Cb_ex_color_im			color;
	Cb_ex_rendering			rendering;
	Cb_ex_tags_item			tags_item;
	Array				array;
	Cb_ex_equiv			equiv;
	Cb_ex_menu_item			menu_item;
	Cb_ex_props_item		props_item;
	Cb_ex_start_stop		startstop;
}
%%
start:			statement_list
			;
statement_list:		statement
			| statement_list statement
			;
statement:		default_equiv_language_statement
			| graph_statement
			| insert_menu_statement
			| invisible_statement
			| language_statement
			| menu_statement
			| node_statement
			| properties_statement
			| ref_focus_unit_statement
			| render_statement
			| tags_statement
			| version_statement
			;

default_equiv_language_statement:
			EX_DEFAULT_EQUIV_LANGUAGE
			{
				cb_ex_default_equiv_language_statement();
			}
			;

graph_statement:	EX_GRAPH string string
				EX_MENU EX_LB menu_item_decl_list EX_RB
			{
				cb_ex_graph_statement($2.string, $3.string,$6);
			}
			;

insert_menu_statement:	EX_INSERT_MENU EX_SYMBOL menu_path_list
				EX_MENU EX_LB menu_item_decl_list EX_RB
			{
				enter_symbol($2, cb_extend_language_);
				cb_ex_insert_menu_statement($2.string,
							    $3,
							    $6);
			}
			;
menu_path_list:		EX_STRING
			{
				enter_symbol($1, cb_extend_menu_path);
				$$ = cb_ex_first_list_item((int)$1.string);
			}
			| menu_path_list EX_STRING
			{
				enter_symbol($2, cb_extend_menu_path);
				$$ = cb_ex_next_list_item($1, (int)$2.string);
			}
			;

invisible_statement:	EX_INVISIBLE
			{
				cb_ex_invisible_statement();
			}
			;

language_statement:	EX_LANGUAGE EX_SYMBOL
					opt_string opt_string opt_grep_able
			{
				enter_symbol($2, cb_extend_language_);
				cb_ex_language_statement($2.string,
							 $3.string,
							 $4.string,
							 $5);
			}
			;
opt_grep_able:		EX_GREP_ABLE
			{
				$$ = 1;
			}
			|
			{
				$$ = 0;
			}
			;

menu_statement:		EX_MENU EX_LB menu_item_decl_list EX_RB
			{
				(void)cb_ex_menu_statement($3);
			}
			;
menu_item_decl_list:	menu_item_decl
			{
				$$ = cb_ex_first_list_item((int)$1);
			}
			| menu_item_decl_list menu_item_decl
			{
				$$ = cb_ex_next_list_item($1, (int)$2);
			}
			;
menu_item_decl:		string equiv_spec opt_menu_props_list
							opt_invisible opt_help
			{
				$$ = cb_ex_menu_regular_item($1.string,
							     $3,
							     $4,
							     $5.string,
							     $2);
			}
			| string equiv_spec
				EX_MENU EX_LB menu_item_decl_list EX_RB
				opt_invisible opt_help
			{
				$$ = cb_ex_menu_pullright_item($1.string,
							       $5,
							       $7,
							       $8.string,
							       $2);
			}
			;
opt_invisible:		EX_INVISIBLE
			{
				$$ = 1;
			}
			|
			{
				$$ = 0;
			}
			;
opt_help:		EX_HELP string
			{
				$$ = $2;
			}
			|
			{
				$$.string = NULL;
				$$.lineno = 0;
			}
			;
opt_menu_props_list:	EX_EQ menu_props_list
			{
				$$ = $2;
			}
			|
			{
				$$ = NULL;
			}
menu_props_list:	EX_LP ref_props_list EX_RP
			{
				$$ = cb_ex_first_list_item((int)$2);
			}
			| menu_props_list EX_OR EX_LP ref_props_list EX_RP
			{
				$$ = cb_ex_next_list_item($1, (int)$4);
			}
			;
ref_props_list:		ref_props_item
			{
				$$ = cb_ex_first_list_item((int)$1);
			}
			| ref_props_list ref_props_item
			{
				$$ = cb_ex_next_list_item($1, (int)$2);
			}
			;
ref_props_item:		EX_SYMBOL
			{
				enter_symbol($1, cb_extend_ref_prop);
				$$ = cb_ex_property((char *)NULL, $1.string);
			}
			| EX_SYMBOL EX_DOT EX_SYMBOL
			{
				enter_symbol($1, cb_extend_ref_prop_prefix);
				enter_symbol($3, cb_extend_ref_prop);
				$$ = cb_ex_property($1.string, $3.string);
			}
			;
equiv_spec:		EX_EQUIV opt_symbol opt_string
			{
				if ($2.string != NULL) {
					enter_symbol($2,
						     cb_extend_equiv_language);
				}
				$$ = cb_ex_menu_equiv_spec($2.string,
							   $3.string);
			}
			|
			{
				$$ = NULL;
			}
			;

node_statement:		EX_NODE EX_SYMBOL EX_SYMBOL
				EX_MENU EX_LB menu_item_decl_list EX_RB
			{
				enter_symbol($2, cb_extend_node_tag);
				enter_symbol($3, cb_extend_node_equiv);
				cb_ex_node_statement($2.string,
						     $3.string,
						     $6);
			}
			;

properties_statement:	EX_PROPERTIES EX_SYMBOL EX_LB def_props_list EX_RB
			{
				enter_symbol($2, cb_extend_def_prop_prefix);
				cb_ex_properties_statement($2.string, $4);
			}
			;
def_props_list:		def_props_item
			{
				$$ = cb_ex_first_list_item((int)$1);
			}
			| def_props_list def_props_item
			{
				$$ = cb_ex_next_list_item($1, (int)$2);
			}
			;
def_props_item:		EX_SYMBOL
			{
				enter_symbol($1, cb_extend_def_prop);
				$$ = cb_ex_property((char *)NULL, $1.string);
			}
			;

ref_focus_unit_statement:
			EX_REF_FOCUS_UNIT EX_SYMBOL
			{
				enter_symbol($2, cb_extend_ref_focus_unit);
				cb_ex_ref_focus_unit_statement($2.string);
			}
			;

render_statement:	EX_RENDER EX_SYMBOL rendering opt_color
			{
				enter_symbol($2, cb_extend_rendering_tag);
				cb_ex_render_statement($2.string,
						       $3,
						       $4);
			}
			;
rendering:		EX_RENDER_SOLID
			{
				$$ = cb_ex_solid_rendering;
			}
			| EX_RENDER_DASHED
			{
				$$ = cb_ex_dashed_rendering;
			}
			| EX_RENDER_DOTTED
			{
				$$ = cb_ex_dotted_rendering;
			}
			| EX_RENDER_DASH_DOTTED
			{
				$$ = cb_ex_dash_dotted_rendering;
			}
			| EX_RENDER_NO_BORDER
			{
				$$ = cb_ex_no_border_rendering;
			}
			| EX_RENDER_SQUARE
			{
				$$ = cb_ex_square_rendering;
			}
			| EX_RENDER_OVAL
			{
				$$ = cb_ex_oval_rendering;
			}
			;
opt_color:		EX_COLOR EX_NUMBER EX_NUMBER EX_NUMBER
			{
				$$ = cb_ex_color($2.value, $3.value, $4.value);
			}
			|
			{
				$$ = NULL;
			}
			;
tags_statement:		EX_TAGS EX_SYMBOL EX_NUMBER
						EX_LB tags_item_decl_list EX_RB
			{
				enter_symbol($2, cb_extend_def_tag_prefix);
				cb_ex_tags_statement($5, $2.string, $3.value);
			}
			;
tags_item_decl_list:	tags_item_decl
			{
				$$ = cb_ex_first_list_item((int)$1);
			}
			| tags_item_decl_list tags_item_decl
			{
				$$ = cb_ex_next_list_item($1, (int)$2);
			}
			;
tags_item_decl:		EX_SYMBOL EX_EQ opt_weight EX_LP ref_props_list EX_RP
				opt_secondary
			{
				enter_symbol($1, cb_extend_def_tag);
				$$ = cb_ex_tags_item($1.string,
						     $3.value,
						     $5,
						     $7);
			}
			| def_focus_unit
			;
opt_secondary:		EX_SECONDARY EX_LP tags_list EX_RP
			{
				$$ = $3;
			}
			|
			{
				$$ = NULL;
			}
			;
tags_list:		EX_SYMBOL
			{
				enter_symbol($1, cb_extend_ref_tag);
				$$ = cb_ex_first_list_item((int)$1.string);
			}
			| tags_list EX_SYMBOL
			{
				enter_symbol($2, cb_extend_ref_tag);
				$$ = cb_ex_next_list_item($1, (int)$2.string);
			}
			;
def_focus_unit:		EX_DEF_FOCUS_UNIT EX_SYMBOL EX_SYMBOL
				EX_LP ref_props_list EX_RP
				opt_use_refd
				opt_make_absolute
				opt_start
				opt_stop
				opt_invisible
			{
				enter_symbol($2,
					     cb_extend_def_focus_unit_name);
				enter_symbol($3,
					     cb_extend_def_focus_unit_code);
				
				$$ = cb_ex_def_focus_unit($2.string,
							  $3.string,
							  $5,
							  $7,
							  $8,
							  $9,
							  $10,
							  $11);
			}
			;
opt_use_refd:		EX_USE_REFD
			{
				$$ = 1;
			}
			|
			{
				$$ = 0;
			}
			;
opt_make_absolute:	EX_MAKE_ABSOLUTE
			{
				$$ = 1;
			}
			|
			{
				$$ = 0;
			}
			;
opt_start:		EX_START EX_NONE
			{
				$$ = cb_ex_none_start_stop;
			}
			| EX_START EX_ZERO
			{
				$$ = cb_ex_zero_start_stop;
			}
			| EX_START EX_INF
			{
				$$ = cb_ex_inf_start_stop;
			}
			|
			{
				$$ = cb_ex_regular_start_stop;
			}
			;
opt_stop:		EX_STOP EX_NONE
			{
				$$ = cb_ex_none_start_stop;
			}
			| EX_STOP EX_ZERO
			{
				$$ = cb_ex_zero_start_stop;
			}
			| EX_STOP EX_INF
			{
				$$ = cb_ex_inf_start_stop;
			}
			|
			{
				$$ = cb_ex_regular_start_stop;
			}
			;

version_statement:	EX_VERSION EX_NUMBER
			{
				cb_ex_version_statement($2.value);
			}
			;

string:			EX_STRING
			{
#ifndef CB_BOOT
				cb_ex_enter_string($1.string,
						   $1.lineno,
						   $1.hidden);
#endif
			}
			| string EX_PLUS EX_STRING
			{
				int	length = strlen($1.string) + strlen($3.string) + 1;
				static char *string = NULL;
				static int string_length = 0;

#ifndef CB_BOOT
				cb_ex_enter_string($3.string,
						   $3.lineno,
						   $3.hidden);
#endif

				if (length > string_length) {
					string_length = length * 2 + 100;
					string = alloca(string_length + 10);
				}
				(void)strcpy(string, $1.string);
				(void)strcat(string, $3.string);
				$$.string = cb_string_unique(string);
			}
			;
opt_symbol:		EX_SYMBOL
			|
			{
				$$.string = NULL;
				$$.lineno = 0;
			}
			;
opt_string:		string
			|
			{
				$$.string = NULL;
				$$.lineno = 0;
			}
			;
opt_weight:		EX_NUMBER
			|
			{
				$$.value = -1;
				$$.lineno = 0;
			}
			;

%%
/*
 *	yyerror(messgae)
 */
int
yyerror(message)
	char	*message;
{
extern	int	yylineno;
extern	char	yyfile[];

	(void)fprintf(stderr, "%s, line %d: %s\n", yyfile, yylineno, message);
}

/*
 *	yylex()
 */
static int
yylex() {
	int token = YYLEX();

/*	(void)printf("%s(%d): %d\n", yyfile, yylineno, token);*/

	return token;
}

#ifndef CB_BOOT
/*
 *	cb_ex_enter_string(string, lineno, hidden)
 */
static void
cb_ex_enter_string(string, lineno, hidden)
	char	*string;
	int	lineno;
	int	hidden;
{
	char	*symbol;
extern	char	*alloca();

	if (hidden == 0) {
		symbol = alloca(strlen(string)+3);
		(void)sprintf(symbol, "\"%s\"", string);
		(void)cb_enter_symbol(symbol,
				      lineno,
				      (unsigned)cb_extend_string);
	}
}
#endif
