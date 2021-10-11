%{

#ifndef lint
static	char sccsid[] = "@(#)commands.y 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Yacc grammar for debugger commands.
 */

#include "defs.h"
#include "symbols.h"
#include "operators.h"
#include "tree.h"
#include "process.h"
#include "source.h"
#include "scanner.h"
#include "names.h"
#include "lists.h"
#include "machine.h"
#include "dbxmain.h"
#include "pipeout.h"
#include "types.h"
#include "languages.h"

private String curformat = "X";
char	*add_strlist();

%}

/*
 * This list must be in the same order as the keywords
 * in keywords.c.
 *
 * When adding a new command, remember
 * 1) Add it to the %term list,
 * 2) Add it to the %type list,
 * 3) Add rules in the grammar,
 * 4) Add it to the 'keyword' and 'cmdname' rules,
 * 5) Add the keyword to the list in keywords.c,
 * 6) Add an operator in operators.c,
 * 7) Add code to evaluate the operator in eval.c,
 * 8) Add help in help.c,
 * 9) You may need to add code to build() and prtree() 
 *    in tree.c (the defaults may be adequate),
 * 10) You may need to add code in assigntypes() in 
 *     symbols.c to get assign the correct type to a node,
 * 11) You may need to add code in check() in check.c
 *     to do semantic checking (some of this is done in 
 *     assigntypes() also).
 */
%term
    ALIAS ALL AND ASSIGN AT 
    BOTMARGIN BUTTON CALL CASE CATCH CD CLEAR CMDLINES COMMAND CONT 
    DBXDEBUG DBXENV DEBUG DELETE DETACH DISPLAY DISPLINES DIV DOWN DUMP
    EDIT ENUM EXPAND 
    FILE FONT FPAASM FPABASE FUNC HELP 
    IF IGNORE IN KILL LINENO LIST LITERAL 
    MAKE MAKEARGS MENU MOD 
    NEXT NEXTI 
    OR
    PRINT PSYM PWD QUIT 
    RERUN RUN 
    SET81 SETENV
    SH SIG SIZEOF SOURCE SOURCES SPEED SRCLINES STATUS
    STEP STEPI 
    STRINGLEN STRUCT STOP STOPI 
    TOOLENV TOPMARGIN TRACE TRACEI
    UNBUTTON UNDISPLAY UNIONX UNMENU UP USE 
    WHATIS WHEN WHERE WHEREIS WHICH WIDTH
    INUM REAL NAME STRING TYPENAME

%right '?' ':'
%left OROR ANDAND
%left AND OR
%left '&' '|' '^' 
%left '#' '<' '=' '>' IN
%left RSHIFT LSHIFT
%left '+' '-'
%left '*' '/' DIV MOD CMOD
%right UNARYSIGN '!'
%right SIZEOF
%left '(' '[' '`' '.' ARROW
%left '\\'

%union {
    Name y_name;
    Symbol y_sym;
    Node y_node;
    Integer y_int;
    Operator y_op;
    long y_long;
    double y_real;
    String y_string;
    Boolean y_bool;
    Cmdlist y_cmdlist;
    List y_list;
    Seltype y_seltype;
};

%type <y_op>	    trace stop runcommand run2 run
%type <y_seltype>   seltype
%type <y_long>	    INUM count
%type <y_real>	    REAL
%type <y_string>    STRING redirectout filename opt_filename mode
%type <y_string>    opt_filename1
%type <y_string>    namelist namelist_name
%type <y_name>	    ALIAS ALL AND ASSIGN AT 
%type <y_name>	    BOTMARGIN BUTTON 
%type <y_name>      CALL CASE CATCH CD CLEAR CMDLINES COMMAND CONT 
%type <y_name>      DBXDEBUG DBXENV DEBUG DELETE DETACH DISPLAY 
%type <y_name>      DISPLINES DIV DOWN DUMP 
%type <y_name>      EDIT ENUM EXPAND
%type <y_name>      FILE FONT FPAASM FPABASE FUNC HELP 
%type <y_name>      IF IGNORE IN KILL LINENO LIST LITERAL
%type <y_name>	    MAKE MAKEARGS MENU MOD NEXT NEXTI OR
%type <y_name>	    PRINT PSYM PWD QUIT RERUN RUN 
%type <y_name>      SET81 SETENV
%type <y_name>      SH SIG SIZEOF SOURCE SOURCES SPEED SRCLINES STATUS STEP STEPI
%type <y_name>	    STRINGLEN STRUCT STOP STOPI 
%type <y_name>      TOOLENV TOPMARGIN TRACE TRACEI
%type <y_name>	    UNBUTTON UNDISPLAY UNIONX UNMENU UP USE 
%type <y_name>      WHATIS WHEN WHERE WHEREIS WHICH WIDTH
%type <y_name>	    curfile identifier name NAME keyword TYPENAME tag cmdname
%type <y_node>      prefix symbol string cursource funcname
%type <y_node>	    command rcommand cmd step what where wherei examine
%type <y_node>	    opt_exp_list opt_cond int_list at_lineno
%type <y_node>	    exp_list exp term boolean_exp constant address
%type <y_node>	    alias_command list_command line_number
%type <y_node>	    button_command menu_command dbxenv_command toolenv_command
%type <y_node>      unbutton_command unmenu_command brkline
%type <y_node>	    typename typespec abstract_decl
%type <y_node>	    int real cmdindex speed_val namenode
%type <y_node>	    sig sig_list sources_command
%type <y_cmdlist>   actions
%type <y_list>	    sourcepath
%type <y_sym>       structue

%%

input:
	input command_line alias_on adb_off
|	alias_on adb_off
;

command_line:
	command terminator
	{
		if ($1 != nil) {
		    if (debug_flag[2]) {
			dumptree($1); 
		    }
		    eval($1);
		}
	}
|	rcommand redirectout terminator
	{
		if ($1 != nil) {
		    if ($2 != nil) {
			setout($2);
			if (debug_flag[2]) {
			    dumptree($1); 
			}
			eval($1);
			unsetout();
		    } else {
			if (debug_flag[2]) {
			    dumptree($1); 
			}
			eval($1);
		    }
		}
	}
|	terminator
;

terminator:
	'\n'
|	';'
;

redirectout:
	'>' beginshellmode NAME endshellmode	{ $$ = idnt($3); }
|	/* empty */			{ $$ = nil; }
;

/*
 * Non-redirectable commands.
 */
command:
	alias_command			{ $$ = $1; }
|	ASSIGN term '=' exp 		{ $$ = build(O_ASSIGN, $2, $4); }
|	button_command 			{ $$ = $1; }
|	CATCH 				{ $$ = build(O_CATCH, nil); }
|	CATCH sig_list 			{ $$ = build(O_CATCH, $2); }
|	CD opt_filename			{ $$ = build(O_CD, $2); }
|	CLEAR brkline			{ $$ = build(O_CLEAR, $2); }
|	CLEAR at_lineno			{ $$ = build(O_CLEAR, $2); }
|	CONT 				{ $$ = build(O_CONT, nil, nil); }
|	CONT SIG sig 			{ $$ = build(O_CONT, nil, $3); }
|	CONT AT at_lineno 		{ $$ = build(O_CONT, $3, nil); }
|	CONT AT at_lineno SIG sig	{ $$ = build(O_CONT, $3, $5); }
|	DBXDEBUG NAME			{ dbxdebug(idnt($2), nil, 0);
					  $$ = nil; }
|	DBXDEBUG NAME INUM		{ dbxdebug(idnt($2), nil, $3);
					  $$ = nil; }
|	DBXDEBUG NAME NAME		{ dbxdebug(idnt($2), idnt($3), 0);
					  $$ = nil; }
|	DBXDEBUG INUM			{ dbxdebug(nil, nil, $2); $$ = nil; }
|	DBXDEBUG HELP			{ dbxdebug(nil, nil, $2); $$ = nil; }
|	DBXDEBUG			{ dbxdebug(nil, nil, 0); $$ = nil; }
|	dbxenv_command			{ $$ = $1; }
|	DEBUG opt_filename opt_filename opt_filename	
					{ $$ = build(O_DEBUG, $2, $3, $4); }
|	DELETE int_list			{ $$ = build(O_DELETE, $2); }
|	DELETE ALL			{ $$ = build(O_DELETE, nil); }
|	DETACH				{ $$ = build(O_DETACH); }
|	DISPLAY				{ $$ = build(O_DISPLAY, nil); }
|	DISPLAY exp_list		{ $$ = build(O_DISPLAY, $2); }
|	DOWN count			{ $$ = build(O_DOWN, build(O_LCON,$2));}
|	EDIT opt_filename		{ $$ = build(O_EDIT, $2); }
|	KILL				{ $$ = build(O_KILL); }
|	FILE opt_filename		{ $$ = build(O_CHFILE, $2); }
|	FUNC				{ $$ = build(O_FUNC, nil); }
|	FUNC funcname			{ $$ = build(O_FUNC, $2); }
|	HELP				{ $$ = build(O_HELP, build(O_LCON, 0));}
|	HELP cmdindex			{ $$ = build(O_HELP, $2); }
|	IGNORE				{ $$ = build(O_IGNORE, nil); }
|	IGNORE sig_list			{ $$ = build(O_IGNORE, $2); }
|	list_command			{ $$ = $1; }
|	MAKE				{ $$ = build(O_MAKE); }
|	menu_command			{ $$ = $1; }
|	PRINT exp_list			{ $$ = build(O_PRINT, $2); }
|	PSYM term			{ $$ = build(O_PSYM, $2); }
|	PWD				{ $$ = build(O_PWD); }
|	QUIT				{ $$ = build(O_QUIT); }
|	runcommand 			{ $$ = build($1); }
|	SET81 symbol '=' INUM INUM INUM	{ $$ = build(O_SET81, $2, $4, $5, $6); }
|	SETENV identifier identifier	{ setenv(idnt($2), idnt($3));
					  $$ = nil; }
|	SETENV identifier STRING	{ setenv(idnt($2), $3);
					  $$ = nil; }
|	SH				{ shellline(); $$ = nil; }
|	SOURCE filename			{ $$ = build(O_SOURCE, $2); }
|	step				{ $$ = $1; }
|	STOP where opt_cond		{ $$ = build(O_STOP, nil, $2, $3); }
|	STOPI wherei opt_cond		{ $$ = build(O_STOPI, nil, $2, $3); }
|	stop what opt_cond		{ $$ = build($1, $2, nil, $3); }
|	stop IF boolean_exp		{ $$ = build($1, nil, nil, $3); }
|	toolenv_command			{ $$ = $1; }
|	TRACE what opt_cond		{ $$ = build(O_TRACE, $2, nil, $3); }
|	TRACE where opt_cond		{ $$ = build(O_TRACE, nil, $2, $3); }
|	TRACEI wherei opt_cond		{ $$ = build(O_TRACEI, nil, $2, $3); }
|	TRACE what where opt_cond	{ $$ = build(O_TRACE, $2, $3, $4); }
|	TRACEI what wherei opt_cond	{ $$ = build(O_TRACEI, $2, $3, $4); }
|	TRACEI what opt_cond		{ $$ = build(O_TRACEI, $2, nil, $3); }
|	trace opt_cond			{ $$ = build($1, nil, nil, $2); }
|	unbutton_command		{ $$ = $1; }
|	UNDISPLAY exp_list		{ $$ = build(O_UNDISPLAY, $2); }
|	unmenu_command			{ $$ = $1; }
|	UP count			{ $$ = build(O_UP, build(O_LCON, $2)); }
|	USE beginshellmode sourcepath endshellmode	
					{ use($3); $$ = nil; }
|	WHATIS term			{ $$ = build(O_WHATIS, $2); }
|	WHATIS prefix '`' TYPENAME	{ $$ = build(O_WHATIS, at($2, $4)); }
|	WHATIS typename			{ $$ = build(O_WHATIS, $2); }
|	WHEN where '{' alias_on adb_off actions '}'
	{
		Node sym;

		if ($2->op == O_QLINE) {
		    sym = build(O_SYM, linesym);
		} else {
		    sym = build(O_SYM, procsym);
		}
		$$ = build(O_ADDEVENT, build(O_EQ, sym, $2), $6);
	}
|	WHEN exp '{' alias_on adb_off actions '}'
	{
		Command action;
		Node cond;

		cond= build(O_EQ, build(O_SYM, procsym), build(O_SYM, program));
		action = build(O_IF, $2, $6);
		$$ = build(O_ADDEVENT, cond, buildcmdlist(action));
	}
|	WHEREIS typespec		{ $$ = build(O_WHEREIS, $2); }
|	WHEREIS symbol			{ $$ = build(O_WHEREIS, $2); }
|	WHICH typespec			{ $$ = build(O_WHICH, $2); }
|	WHICH symbol			{ $$ = build(O_WHICH, $2); }
|	'/' searchdown_on STRING search_off opt_slash 
					{ $$ = build(O_SEARCH, $3); }
|	'?' searchup_on STRING search_off opt_question 
					{ $$ = build(O_UPSEARCH, $3); }
;

runcommand:
	run2 arglist endshellmode	{ $$ = $1; }
|	run 				{ $$ = $1; if ($1==O_RERUN) arginit(); }
;

run2:
	run				{ $$ = $1; arginit(); }
;

run:
	RUN beginshellmode		{ $$ = O_RUN; }
|	RERUN beginshellmode		{ $$ = O_RERUN; }
;

arglist:
    	arglist arg
|	arg
;

arg:
   	NAME				{ newarg(idnt($1)); }
|	'<' NAME 			{ inarg(idnt($2)); }
|	'>' NAME 			{ outarg(idnt($2)); }
|       RSHIFT NAME			{ apparg(idnt($2)); }
;

dbxenv_command:
	DBXENV				{ $$ = build(O_PRDBXENV); }
|	DBXENV STRINGLEN int		{ $$ = build(O_DBXENV, STRINGLEN, $3); }
|	DBXENV SPEED speed_val		{ $$ = build(O_DBXENV, SPEED, $3); }
|	DBXENV CASE namenode		{ $$ = build(O_DBXENV, CASE, $3); }
|	DBXENV FPAASM namenode		{ $$ = build(O_DBXENV, FPAASM, $3); }
|	DBXENV FPABASE namenode		{ $$ = build(O_DBXENV, FPABASE, $3); }
|	DBXENV MAKEARGS init_strlist namelist
					{ $$ = build(O_DBXENV, MAKEARGS, 
					    build(O_SCON, $4)); }
;

namenode:
	identifier			{ $$ = build(O_NAME, $1); }
;

speed_val:
	real				{ $$ = $1; }
|	INUM				{ $$ = build(O_FCON, (double) $1); }
;

toolenv_command:
	TOOLENV				{ $$ = build(O_PRTOOLENV); }
|	TOOLENV BOTMARGIN int		{ $$ = build(O_BOTMARGIN, $3); }
|	TOOLENV CMDLINES int		{ $$ = build(O_CMDLINES, $3); }
|	TOOLENV DISPLINES int		{ $$ = build(O_DISPLINES, $3); }
|	TOOLENV FONT filename		{ $$ = build(O_FONT, $3); }
|	TOOLENV SRCLINES int		{ $$ = build(O_SRCLINES, $3); }
|	TOOLENV TOPMARGIN int		{ $$ = build(O_TOPMARGIN, $3); }
|	TOOLENV WIDTH int		{ $$ = build(O_WIDTH, $3); }
;

step:
	STEP				{ $$ = build(O_STEP, true, false, 1); }
|	STEPI				{ $$ = build(O_STEP, false, false, 1); }
|	STEP INUM 			{ $$ = build(O_STEP, true, false, $2); }
|	STEPI INUM			{ $$ = build(O_STEP, false, false, $2);}
|	NEXT				{ $$ = build(O_STEP, true, true, 1); }
|	NEXTI				{ $$ = build(O_STEP, false, true, 1); }
|	NEXT INUM			{ $$ = build(O_STEP, true, true, $2); }
|	NEXTI INUM			{ $$ = build(O_STEP, false, true, $2); }

beginshellmode:
	/* empty */			{ set_shellmode(true); }
;

endshellmode:
	/* empty */			{ set_shellmode(false); }
;

sourcepath:
	sourcepath NAME
	{
		$$ = $1;
		list_append(list_item(idnt($2)), nil, $$);
	}
|	/* empty */			{ $$ = list_alloc(); }
;

actions:
	actions cmd ';' alias_on adb_off
				{ $$ = $1; cmdlist_append($2, $$); }
|	cmd ';'	alias_on adb_off
				{ $$ = list_alloc(); cmdlist_append($1, $$); }
;

cmd:
	command
|	rcommand
;

/*
 * Redirectable commands.
 */
rcommand:
	WHERE				{ $$ = build(O_WHERE, build(O_LCON,0));}
|       WHERE int			{ $$ = build(O_WHERE, $2); }
|	examine 			{ $$ = $1; }
|	CALL term 			{ $$ = $2;
					  if ($$->op == O_CALL)
					      $$->op = O_CALLCMD;
					  else
					      error("Bad call statement");
				        }
|	DUMP	 			{ $$ = build(O_DUMP, nil); }
|	DUMP funcname			{ $$ = build(O_DUMP, $2); }
|	STATUS 				{ $$ = build(O_STATUS); }
|   PSYM                { $$ = build(O_PSYM, nil); }
|   sources_command			{ $$ = $1; }
;

alias_command:
   	alias identifier namelist	{ $$ = build(O_ALIAS, idnt($2), $3); }
|	alias identifier		{ $$ = build(O_ALIAS, idnt($2), nil);}
|	alias				{ $$ = build(O_ALIAS, nil, nil); }
;


alias:
	ALIAS init_strlist
;

sources_command:
   	sources namelist	{ $$ = build(O_SOURCES, $2); set_shellmode(false); }
|	sources				{ $$ = build(O_SOURCES, nil); set_shellmode(false); }
;

sources:
	SOURCES { set_shellmode(true); init_strlist(); }
;

button_command:
	BUTTON seltype init_strlist namelist	
					{ $$ = build(O_BUTTON, $2, $4); }
;

unbutton_command:
	UNBUTTON init_strlist namelist  { $$ = build(O_UNBUTTON, $3);}
;

menu_command:
	MENU seltype init_strlist namelist
					{ $$ = build(O_MENU, $2, $4); }
;

unmenu_command:
	UNMENU init_strlist namelist    { $$ = build(O_UNMENU, $3); }
;

seltype:
	COMMAND				{ $$ = SEL_COMMAND; }
|	EXPAND				{ $$ = SEL_EXPAND; }
|	IGNORE				{ $$ = SEL_IGNORE; }
|	LINENO				{ $$ = SEL_LINENO; }
|	LITERAL				{ $$ = SEL_LITERAL; }
;

searchdown_on:
	/* empty */			{ search(true, '/'); }
;

searchup_on:
	/* empty */			{ search(true, '?'); }
;

search_off:
	/* empty */			{ search(false, ' '); }
;

opt_slash:
	/* empty */
|	'/'
;

opt_question:
	/* empty */
|	'?'
;

trace:
	TRACE				{ $$ = O_TRACE; }
|	TRACEI				{ $$ = O_TRACEI; }
;

stop:
	STOP				{ $$ = O_STOP; }
|	STOPI				{ $$ = O_STOPI; }
;

wherei:
	AT address			{ $$ = $2; }
|	IN funcname			{ $$ = $2; }
;

what:
	exp				{ $$ = $1; }
|	string ':' line_number		{ $$ = build(O_QLINE, $1, $3); }
;

where:
	IN funcname			{ $$ = $2; }
|	AT at_lineno			{ $$ = $2; }
;


at_lineno:
	line_number cursource		{ $$ = build(O_QLINE, $2, $1); }
|	string ':' line_number		{ $$ = build(O_QLINE, $1, $3); }
;

brkline:
	/* empty */ 		
	{ 
	    Node file, line;

	    file = build(O_SCON, brksource);
	    line = build(O_LCON, brkline);
	    $$ = build(O_QLINE, file, line);
	}
;

cursource:
	/* empty */
	{ 
		if (cursource != nil) {
			$$ = build(O_SCON, strdup(cursource));
		} else {
			error("No current source file");
			/* NOTREACHED */
		}
	}
;

filename:
	beginshellmode NAME endshellmode{ $$ = idnt($2); }
;

opt_filename:
	beginshellmode opt_filename1 	{ $$ = $2; set_shellmode(false); }
;

opt_filename1:
	/* empty */			{ $$ = nil; }
|	NAME				{ $$ = idnt($1); }
;

opt_exp_list:
	exp_list			{ $$ = $1; }
|	/* empty */			{ $$ = nil; }
;

list_command:
	LIST
	{
		$$ = build(O_LIST,
		    build(O_LCON, (long) cursrcline),
		    build(O_LCON, (long) cursrcline + 9)
		);
	}
|	LIST line_number		{ $$ = build(O_LIST, $2, $2); }
|	LIST line_number ',' line_number{ $$ = build(O_LIST, $2, $4); }
|	LIST funcname			{ $$ = build(O_LIST, $2); }
;

curfile:
	name				{ curfile = find_file($1); }

;

funcname:
	'`' 				{ set_filenamemode(true); }
	curfile
				  	{ set_filenamemode(false); }
	'`' prefix		  	{ $$ = $6; }
|	prefix 				{ $$ = $1; }
;

line_number:
	INUM				{ $$ = build(O_LCON, $1); }
|	'$'				{ $$ = build(O_LCON, (long) LASTLINE);}
;

examine:
	address '/' adb_on count mode
					{ $$ = build(O_EXAMINE, $5, $1,nil,$4);}
|	address ',' address '/' adb_on mode
					{ $$ = build(O_EXAMINE, $6, $1, $3, 0);}
|	'+' '/' adb_on alias_off count mode
	   { $$ = build(O_EXAMINE, $6, build(O_LCON, (long)prtaddr), nil, $5); }
|	address '=' adb_on mode
					{ $$ = build(O_EXAMINE, $4, $1, nil,0);}
;

address:
	INUM alias_off			{ $$ = build(O_LCON, $1); }
|	'&' alias_off term		{ $$ = build(O_AMPER, $3); }
|	address '+' address		{ $$ = build(O_ADD, $1, $3); }
|	address '-' address		{ $$ = build(O_SUB, $1, $3); }
|	address '*' address		{ $$ = build(O_MUL, $1, $3); }
|	'*' address %prec UNARYSIGN	{ $$ = examine_indir($2); }
|	'(' alias_off exp ')'		{ $$ = $3; }
;

alias_on:
	/* empty */			{ set_alias(true); }
;

alias_off:
	/* empty */			{ set_alias(false); }
;


adb_on:
	/* empty */			{ set_adb(true); }
;

adb_off:
	/* empty */			{ set_adb(false); }
;

count:
	/* empty */			{ $$ = 1; }
|	INUM				{ $$ = $1; }
;

mode:
	identifier			{ $$ = idnt($1); curformat = $$; }
|	/* empty */			{ $$ = curformat; }
;

opt_cond:
	/* empty */			{ $$ = nil; }
|	IF boolean_exp			{ $$ = $2; }
;

exp_list:
	exp				{ $$ = build(O_COMMA, $1, nil); }
|	exp ',' exp_list		{ $$ = build(O_COMMA, $1, $3); }
;

exp:
 	term				{ $$ = build(O_RVAL, $1); }
|	'+' exp %prec UNARYSIGN		{ $$ = $2; }
|	'-' exp %prec UNARYSIGN		{ $$ = build(O_NEG, $2); }
|	'~' exp %prec UNARYSIGN		{ $$ = build(O_COMPL, $2); }
|	'!' exp %prec UNARYSIGN		{ $$ = build(O_NOT, $2); }
|	exp '+' exp			{ $$ = build(O_ADD, $1, $3); }
|	exp '-' exp			{ $$ = build(O_SUB, $1, $3); }
|	exp '*' exp			{ $$ = build(O_MUL, $1, $3); }
|	exp '/' exp			{ $$ = build(O_DIVF, $1, $3); }
|	exp DIV exp			{ $$ = build(O_DIV, $1, $3); }
|	exp MOD exp			{ $$ = build(O_MOD, $1, $3); }
|	exp CMOD exp			{ $$ = build(O_MOD, $1, $3); }
|	exp '|' exp			{ $$ = build(O_BITOR, $1, $3); }
|	exp '&' exp			{ $$ = build(O_BITAND, $1, $3); }
|	exp '^' exp			{ $$ = build(O_BITXOR, $1, $3); }
|	exp RSHIFT exp			{ $$ = build(O_RSHIFT, $1, $3); }
|	exp LSHIFT exp			{ $$ = build(O_LSHIFT, $1, $3); }
|	exp AND exp			{ $$ = build(O_LOGAND, $1, $3); }
|	exp ANDAND exp			{ $$ = build(O_LOGAND, $1, $3); }
|	exp OR exp			{ $$ = build(O_LOGOR, $1, $3); }
|	exp OROR exp			{ $$ = build(O_LOGOR, $1, $3); }
|	exp '<' exp			{ $$ = build(O_LT, $1, $3); }
|	exp '<' '=' exp			{ $$ = build(O_LE, $1, $4); }
|	exp '>' exp			{ $$ = build(O_GT, $1, $3); }
|	exp '>' '=' exp			{ $$ = build(O_GE, $1, $4); }
|	exp '=' exp			{ $$ = build(O_EQ, $1, $3); }
|	exp '=' '=' exp			{ $$ = build(O_EQ, $1, $4); }
|	exp '<' '>' exp			{ $$ = build(O_NE, $1, $4); }
|	exp '#' exp			{ $$ = build(O_NE, $1, $3); }
|	exp '!' '=' exp			{ $$ = build(O_NE, $1, $4); }
|	exp '?' exp ':' exp		{ $$ = build(O_QUEST, $1, $3, $5); }
;

term:
	symbol				{ $$ = $1; }
|	prefix '`' name			{ $$ = at($1, $3); }
|	'`' curfile '`' prefix		{ $$ = $4; }
|	'`' name			{ $$ = atglobal($2); }
|	constant			{ $$ = $1; }
|	SIZEOF term %prec SIZEOF	{ $$ = build(O_SIZEOF, $2); }
|	SIZEOF '(' typename ')' %prec SIZEOF 	
					{ $$ = build(O_SIZEOF, $3); }
|	'&' term %prec UNARYSIGN	{ $$ = build(O_AMPER, $2); }
|	'(' typename ')' term %prec UNARYSIGN
					{ $$ = build(O_TYPERENAME, $4, $2); }
|	term '[' exp_list ']'		{ $$ = subscript($1, $3); }
|	term '(' opt_exp_list ')'	{ if (($1 != nil) &&
					      ($1->nodetype != nil) &&
					      ($1->nodetype->class == VAR))
					    $$ = subscript($1, $3);
					  else
					    $$ = build(O_CALL, $1, $3); }
|	term ARROW { set_qualified(true); } name
						{ $<y_node>$ = dot($<y_node>1, $<y_name>4); }
|	term '.' { set_qualified(true); } name
						{ $<y_node>$ = dot($<y_node>1, $<y_name>4); }
|	'*' term %prec UNARYSIGN	{ $$ = build(O_INDIR, $2); }
|	'(' exp ')' 			{ $$ = $2; }
|	'#' term %prec UNARYSIGN	{ $$ = concrete($2); }
;

prefix:
	prefix '`' name		{ $$ = at($1, $3); }
|	name			{ $$ = build(O_SYM, which_func_file(curfile, $1)); }
;

sig_list:
	sig			{ $$ = build(O_COMMA, $1, nil); }
|	sig opt_comma sig_list  { $$ = build(O_COMMA, $1, $3); }
;

sig:
	int			{ $$ = $1;
				  if ($1->value.lcon <0 || $1->value.lcon >31) 
				      error("bad signal number: %d", $1->value.lcon);
			        }
|	identifier		{ $$ = build(O_LCON, name_to_signo(idnt($1)));}
;

int_list:
	int			{ $$ = build(O_COMMA, $1, nil); }
|	int opt_comma int_list	{ $$ = build(O_COMMA, $1, $3); }
;

opt_comma:
	/* empty */
|	','
;

boolean_exp:
	exp				{ chkboolean($1); $$ = $1; }
;

constant:
	int				{ $$ = $1; }
|	real 				{ $$ = $1; }
|	string 				{ $$ = $1; }
;

int:
	INUM				{ $$ = build(O_LCON, $1); }
;

real:
	REAL				{ $$ = build(O_FCON, $1); }
;

string:
	STRING 				{ $$ = build(O_SCON, $1); }
;

symbol:
	name 				{ $$ = build(O_SYM, which($1)); }
;

init_strlist:
	/* empty */			{ init_strlist(); }
;

namelist:
	namelist namelist_name 		{ $$ = $1; }
|	namelist_name			{ $$ = $1; }
;

namelist_name:
	identifier			{ $$ = add_strlist(idnt($1)); }
|	STRING				{ $$ = add_strlist($1); }
;

name:
	NAME				{ $$ = $1; }
|	keyword				{ $$ = $1; }
;

identifier:
	NAME				{ $$ = $1; }
|	keyword				{ $$ = $1; }
|	TYPENAME			{ $$ = $1; }
;

typename:
	typespec abstract_decl		{ $$ = buildtype( $1, $2 ); }
;

typespec:
	TYPENAME			{ $$ = build(O_SYM, which($1)); }
|	TYPENAME TYPENAME	
		{ $$ = build(O_SYM, combitypes( which($1), which($2) )); }
|	structue 			{ $$ = build(O_SYM, $1); }
;

abstract_decl:
	/* empty */			
		{ $$ = build( O_SYM, newSymbol( nil, 0, TYPE, t_int, nil )); }
|	'(' abstract_decl ')'		{ $$ = $2; }
|	'*' abstract_decl %prec UNARYSIGN { $$ = build_ptrdcl( $2 ) ; }
|	abstract_decl '(' ')'		{ $$ = build_funcdcl( $1 ) ; }
;

structue:
	STRUCT tag			{ $$ = lookup_structtag($2); } 
| 	UNIONX tag			{ $$ = lookup_uniontag($2); }
| 	ENUM tag			{ $$ = lookup_enumtag($2); }
;

tag:
	tag_on identifier tag_off	{ $$ = $2; }
;

tag_on:
	/* empty */ 			{ set_tag(true); }
;

tag_off:
	/* empty */ 			{ set_tag(false); }
;

keyword:
    ALIAS | ALL | AND | ASSIGN | AT | 
    BUTTON | BOTMARGIN |
    CALL | CASE | CATCH | CD | CLEAR | CMDLINES | COMMAND | CONT | 
    DBXDEBUG | DBXENV | DEBUG | DELETE | DETACH | DISPLAY | 
    DISPLINES | DIV | DOWN | DUMP | 
    EDIT | EXPAND | 
    FILE | FONT | FPAASM | FPABASE | FUNC | 
    HELP | 
    IGNORE | IN |
    KILL | 
    LINENO | LIST | LITERAL |
    MAKE | MAKEARGS | MENU | MOD | 
    NEXT | NEXTI | 
    OR | 
    PRINT | PSYM | PWD |
    QUIT | 
    RERUN | RUN |
    SET81 | SETENV |
    SH | SIG | SOURCE | SOURCES | SPEED | SRCLINES | STATUS | STEP | STEPI | 
    STRINGLEN | STOP | STOPI | 
    TOOLENV | TOPMARGIN | TRACE | TRACEI |
    UNBUTTON | UNDISPLAY | UNMENU | UP | USE | 
    WHATIS | WHEN | WHERE | WHEREIS | WHICH | WIDTH
;

cmdindex:
	cmdname			   { $$ = build(O_LCON, (int) findkeyword($1));}
|       '/'				{ $$ = build(O_LCON, '/'); }
|       '?'				{ $$ = build(O_LCON, '?'); }
;

cmdname:
    ALIAS | ASSIGN | BUTTON |
    CALL | CATCH | CD | CLEAR | CONT |
    DBXENV | DEBUG | DELETE | DETACH | DISPLAY | DOWN | DUMP |
    EDIT | 
    FILE | FUNC |
    HELP |
    IGNORE |
    KILL |
    LIST |
    MAKE | MENU |
    NEXT | NEXTI |
    PRINT | PWD |
    QUIT |
    RERUN | RUN |
    SET81 | SETENV |
    SH | SOURCE | SOURCES | SPEED | STATUS | STEP | STEPI | STOP | STOPI |
    TOOLENV | TRACE | TRACEI |
    UNBUTTON | UNDISPLAY | UNMENU | UP | USE |
    WHATIS | WHEN | WHERE | WHEREIS | WHICH 
;
