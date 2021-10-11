%{
/*
 *	@(#)cb_ex_scan.l 1.1 94/10/31 SMI
 *		Scanner for the code browser extensibility compiler
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


#ifndef array_h_INCLUDED
#include "array.h"
#endif

#ifndef CB_BOOT
#ifndef cb_cpp_protocol_h_INCLUDED
#include "cb_cpp_protocol.h"
#endif
#endif

#ifndef CB_BOOT
#ifndef cb_enter_h_INCLUDED
#include "cb_enter.h"
#endif
#endif

#ifndef cb_extend_h_INCLUDED
#include "cb_extend.h"
#endif

#ifndef cb_libc_h_INCLUDED
#include "cb_libc.h"
#endif

#ifndef cb_string_h_INCLUDED
#include "cb_string.h"
#endif

#include "cb_ex_parse.h"


	char		yyfile[1024];

#ifdef lint
yywrap()
{
	(void)yyoutput(yyinput());
	yyunput(10);
	return 1;
}
#else
#define yywrap() 1

#undef output
#define output(c)
#endif

#ifndef CB_BOOT
#undef input
#define input() cb_ex_input()
#endif

#define RETURN(val) yylval.lineno = yylineno; \
			return(val)

#define yylex YYLEX


static	int	cb_ex_hidden = 0;
%}

%%
"("				{ RETURN(EX_LP); }
")"				{ RETURN(EX_RP); }
"+"				{ RETURN(EX_PLUS); }
"."				{ RETURN(EX_DOT); }
"="				{ RETURN(EX_EQ); }
"{"				{ RETURN(EX_LB); }
"|"				{ RETURN(EX_OR); }
"}"				{ RETURN(EX_RB); }

"color"				{ RETURN(EX_COLOR); }
"dash-dotted"			{ RETURN(EX_RENDER_DASH_DOTTED); }
"dashed"			{ RETURN(EX_RENDER_DASHED); }
"default-equiv-language"	{ RETURN(EX_DEFAULT_EQUIV_LANGUAGE); }
"def-focus-unit"		{ RETURN(EX_DEF_FOCUS_UNIT); }
"dotted"			{ RETURN(EX_RENDER_DOTTED); }
"equiv"				{ RETURN(EX_EQUIV); }
"ex-file"			{ RETURN(EX_EX_FILE); }
"graph"				{ RETURN(EX_GRAPH); }
"grep-able"			{ RETURN(EX_GREP_ABLE); }
"help"				{ RETURN(EX_HELP); }
"inf"				{ RETURN(EX_INF); }
"insert-menu-items"		{ RETURN(EX_INSERT_MENU); }
"invisible"			{ RETURN(EX_INVISIBLE); }
"language"			{ RETURN(EX_LANGUAGE); }
"make-absolute"			{ RETURN(EX_MAKE_ABSOLUTE); }
"menu"				{ RETURN(EX_MENU); }
"no-border"			{ RETURN(EX_RENDER_NO_BORDER); }
"node"				{ RETURN(EX_NODE); }
"none"				{ RETURN(EX_NONE); }
"oval"				{ RETURN(EX_RENDER_OVAL); }
"properties"			{ RETURN(EX_PROPERTIES); }
"ref-focus-unit"		{ RETURN(EX_REF_FOCUS_UNIT); }
"render"			{ RETURN(EX_RENDER); }
"secondary"			{ RETURN(EX_SECONDARY); }
"solid"				{ RETURN(EX_RENDER_SOLID); }
"square"			{ RETURN(EX_RENDER_SQUARE); }
"start"				{ RETURN(EX_START); }
"stop"				{ RETURN(EX_STOP); }
"tags"				{ RETURN(EX_TAGS); }
"tags_h"			{ RETURN(EX_TAGS_H); }
"use-refd"			{ RETURN(EX_USE_REFD); }
"version"			{ RETURN(EX_VERSION); }
"weight"			{ RETURN(EX_WEIGHT); }
"zero"				{ RETURN(EX_ZERO); }

[a-zA-Z_][a-zA-Z_0-9]*		{ 
		yylval.symbol.lineno = yylineno;
		yylval.symbol.hidden = cb_ex_hidden;
		yylval.symbol.string = cb_string_unique(yytext);
		return(EX_SYMBOL);
		}

'[^']*'				{
		goto string;
		}
\"[^\"]*\"			{
		char	*cdp;
		char	*csp;
		int	value = 0;
	    string:

		yylval.symbol.lineno = yylineno;
		yylval.symbol.hidden = cb_ex_hidden;
		yytext[strlen(yytext)-1] = 0;
		for (cdp = yytext, csp = yytext+1; *csp != 0; ) {
			if (*csp != '\\') {
				*cdp++ = *csp++;
			} else {
				if (isdigit(*++csp)) {
					value = *csp - '0';
					if (isdigit(*++csp)) {
						value *= 8;
						value += *csp - '0';
						if (isdigit(*++csp)) {
							value *= 8;
							value += *csp - '0';
							csp++;
						}
					}
				}
				*cdp++ = value;
			}
		}
		*cdp = 0;
		yylval.symbol.string = cb_string_unique(yytext);
		return(EX_STRING);
		}

[0-9]+[0-9\.]*			{
		yylval.integer.lineno = yylineno;
		yylval.symbol.hidden = cb_ex_hidden;
		if (sscanf(yytext, "%d", &yylval.integer.value) != 1) {
			cb_ex_fatal("`%s' is not a number\n", yytext);
		}
		return(EX_NUMBER);
		}

#.*$				{
		int	level;

		if (sscanf(yytext, "# %d %s %d", &yylineno, yyfile, &level) == 3) {
#ifndef CB_BOOT
			switch (level) {
			case 1:
				cb_enter_push_file(yyfile, 1);
				break;
			case 2:
				cb_enter_pop_file();
				break;
			}
		} else {
			cb_enter_push_file(yyfile, 1);
#endif
		}
		yylineno--;
		return(yylex());
		}

%%

#ifndef CB_BOOT

#define ex_getchar() ((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==EOF?0:yytchar)

static char
cb_ex_plain_input()
{
	return ex_getchar();
}

cb_ex_input()
{
	char	chr;

	cb_ex_hidden = cb_hidden;

	while ((chr = ex_getchar()) == CB_CHR_PREFIX) {
		if (cb_cbextend_cpp_protocol()) {
			return CB_CHR_PREFIX;
		}
	}
	if (chr == '\n') {
		yylineno++;
	}

	return chr;
}

/*
 *	cb_cbextend_get_cpp_id()
 *		Read one identifier from the cpp byte stream
 */
static char *
cb_cbextend_get_cpp_id()
{
	char	*result;

	cbgetid(cb_ex_plain_input, , result);
	return result;
}

/*
 *	cb_cbextend_lxtitle()
 */
void
cb_cbextend_lxtitle()
{
	int	level = 0;
	char	*p = yytext;

	while ((*p++ = cb_ex_plain_input()) != '\n');
	*p = 0;
	(void)sscanf(yytext, "# %d %s %d", &yylineno, yyfile, &level);
	switch (level) {
	case 0:
		break;
	case 1:
		cb_enter_push_file(yyfile, 1);
		break;
	case 2:
		cb_enter_pop_file();
		break;
	}
}

/*
 *	cb_cbextend_cpp_protocol()
 *		This function is called from cbextend to handle cpp
 *		protocol information.  TRUE is returned whenever
 *		a control-A was in the input stream.
 */
int
cb_cbextend_cpp_protocol()
{
	char		chr;

	chr = cb_ex_plain_input();
	switch (chr) {
	case CB_CHR_SAYLINE:
		cb_cbextend_lxtitle();
		return FALSE;
	default:
		return cb_cpp_protocol(chr,
				       cb_ex_plain_input,
				       cb_cbextend_get_cpp_id,
				       (Cb_cpp_get_block_fn)NULL,
				       stdin);
	}
}

#endif
