#ifndef lint
static	char sccsid[] = "@(#)operators.c 1.1 94/10/31 SMI"; /* from UCB X.X XX/XX/XX */
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Tree node classes.
 */

#include "defs.h"
#include "operators.h"

#ifndef public
/*
 * Precedence values.  Higher precedence has a higher value.
 */
enum Precedence { 
	PREC_NONE,
	PREC_COMMA,		/* , */
	PREC_ASSIGN,		/* = += -= etc */
	PREC_QUEST,		/* ?: */
	PREC_LOGOR,		/* || */
	PREC_LOGAND,		/* && */
	PREC_BITOR,		/* bitwise(|) */
	PREC_BITXOR,		/* bitwise(^) */
	PREC_BITAND,		/* bitwise(&) */
	PREC_EQ,		/* == != */
	PREC_COMP,		/* < <= > >= */
	PREC_SHIFT,		/* << >> */
	PREC_ADD,		/* + sub(-) */
	PREC_MUL,		/* multiply(*) / % */
	PREC_NOT,		/* ! ~ ++ -- unary(-) (cast) indir(*) */
			        /* addr(&) sizeof */
	PREC_CALL,		/* () [] -> . */
	PREC_LEAF,		/* leaf nodes like constants and symbols */
};
typedef enum Precedence Precedence;

typedef struct {
    char numargs;
    char opflags;
    String opstring;
    Precedence prec;
} Opinfo;

typedef enum {
    O_NOP,
    O_NAME, O_SYM, O_LCON, O_FCON, O_SCON,
    O_RVAL, O_INDEX, O_INDIR, O_DOT, O_AMPER,
    O_COMMA,

    O_ITOF, O_FTOI, O_ADD, O_ADDF, O_SUB, O_SUBF, O_NEG, O_NEGF,
    O_MUL, O_MULF, O_DIVF, O_DIV, O_MOD,

    O_BITAND, O_BITOR, O_BITXOR, O_RSHIFT, O_LSHIFT, O_COMPL,

    O_LOGAND, O_LOGOR,

    O_LT, O_LTF, O_LE, O_LEF, O_GT, O_GTF, O_GE, O_GEF,
    O_EQ, O_EQF, O_NE, O_NEF, O_NOT, O_SIZEOF, O_QUEST,

    O_ALIAS,		/* rename a command */
    O_ASSIGN,		/* assign a value to a program variable */
    O_BOTMARGIN,	/* Bottom margin in src subwindow */
    O_BUTTON,		/* Define a dbxtool button */
    O_CALL,		/* call a procedure in the program */
    O_CATCH,		/* catch a signal before program does */
    O_CD,		/* Change the current directory */
    O_CHFILE,		/* change (or print) the current source file */
    O_CLEAR,		/* Clear all breakpoints at a given line */
    O_CMDLINES,		/* # of lines in cmd subwindow */
    O_CONT,		/* continue execution */
    O_DBXENV,		/* Control dbx environment */
    O_DEBUG,		/* invoke a dbx internal debugging routine */
    O_DELETE,		/* remove a trace/stop */
    O_DETACH,		/* Detach the child process */
    O_DISPLAY,		/* display items at each breakpoint */
    O_DISPLINES,	/* # of lines in display subwindow */
    O_DOWN,		/* Move down the call stack */
    O_DUMP,		/* dump out variables */
    O_EDIT,		/* edit a file (or function) */
    O_FONT,		/* Font in dbxtool */
    O_FUNC,		/* set the current function */
    O_HELP,		/* print a synopsis of debugger commands */
    O_IGNORE,		/* let program catch signal */
    O_KILL,		/* kill the debugee */
    O_LIST,		/* list source lines */
    O_MAKE,		/* Re-make the program */
    O_MENU,		/* Add an entry to dbxtool's menu */
    O_PRDBXENV,		/* Print the dbxenv variables */
    O_PRINT,		/* print the values of a list of expressions */
    O_PRTOOLENV,	/* Print the toolenv variables */
    O_PSYM,		/* print symbol information */
    O_PWD,		/* Print the current Working Directory */
    O_QUIT,		/* Quit, exit dbx */
    O_RERUN,		/* Re-run the program with the current args */
    O_RUN,		/* start up program */
    O_SEARCH,		/* Search for a string */
    O_SET81,		/* Set a 68881 register to a bit pattern */
    O_SETENV,		/* Set an environment variable */
    O_SH,		/* Go to the shell and execute a command */
    O_SOURCE,		/* read commands from a file */
    O_SOURCES,		/* display/control the sources selection list */
    O_SRCLINES,		/* # of lines in src subwindow */
    O_STATUS,		/* display currently active trace/stop's */
    O_STEP,		/* execute a single line */
    O_STOP,		/* stop on an event */
    O_STOPI,		/* stop on an event at an instruction boundary */
    O_TOPMARGIN,	/* Top margin in src subwindow */
    O_TRACE,		/* trace something on an event */
    O_TRACEI,		/* trace at the instruction level */
    O_UNBUTTON,		/* Remove a dbxtool button */
    O_UNDISPLAY,	/* Remove an item from the display list */
    O_UNMENU,		/* Remove an entry from dbxtool's menu */
    O_UP,		/* Move up the call stack */
    O_UPSEARCH,		/* Search for a string in an upwards direction */
    O_USE,		/* Define the path for source files */
    O_WHATIS,		/* print the declaration of a variable */
    O_WHERE,		/* print a stack trace */
    O_WHEREIS,		/* print all the symbols with the given name */
    O_WHICH,		/* print out full qualification of a symbol */
    O_WIDTH,		/* Width of dbxtool in chars */
    O_EXAMINE,		/* examine program instructions/data */

    O_ADDEVENT,		/* add an event */
    O_ENDX,		/* end of program reached */
    O_IF,		/* if first arg is true, do commands in second arg */
    O_ONCE,		/* add a "one-time" event, delete when first reached */
    O_STOPONCE,		/* add a "one-time" stop event, for trace-entry */
    O_PRINTCALL_NOW,	/* print out the current procedure and its arguments */
    O_PRINTCALL,	/* print out the current procedure and its arguments */
    O_PRINTIFCHANGED,	/* print the value of the argument if it has changed */
    O_PRINTRTN,		/* print out the routine and value that just returned */
    O_PRINTSRCPOS,	/* print out the current source position */
    O_PRINTINSTPOS,	/* print out the current instruction position */
    O_QLINE,		/* filename, line number */
    O_STOPIFCHANGED,	/* stop if the value of the argument has changed */
    O_STOPX,		/* stop execution */
    O_TRACEON,		/* begin tracing source line, variable, or all lines */
    O_TRACEOFF,		/* end tracing source line, variable, or all lines */

    O_TYPERENAME,	/* state the type of an expression */
    O_CALLCMD,		/* CALL command (no return value requested) */

    O_LASTOP
} Operator;

/*
 * Operator flags and predicates.
 */

#define null 0
#define LEAF 01
#define UNARY 02
#define BINARY 04
#define REALOP 020
#define INTOP 040

#define isbitset(a, m)	((a&m) == m)
#define isleaf(o)	isbitset(opinfo[ord(o)].opflags, LEAF)
#define isunary(o)	isbitset(opinfo[ord(o)].opflags, UNARY)
#define isbinary(o)	isbitset(opinfo[ord(o)].opflags, BINARY)
#define isreal(o)	isbitset(opinfo[ord(o)].opflags, REALOP)
#define isint(o)	isbitset(opinfo[ord(o)].opflags, INTOP)
#define opname(o)	(opinfo[ord(o)].opstring)
#define opprec(o)	ord((opinfo[ord(o)].prec))

#define degree(o)	(opinfo[ord(o)].opflags&(LEAF|UNARY|BINARY))
#define nargs(o)	(opinfo[ord(o)].numargs)

#endif

/*
 * Operator information structure.
 */

public Opinfo opinfo[] ={
/* O_NOP */		0,	null,		"nop",		PREC_NONE,
/* O_NAME */		-1,	LEAF,		"name",		PREC_NONE,
/* O_SYM */		-1,	LEAF,		"sym",		PREC_LEAF,
/* O_LCON */		-1,	LEAF,		"lcon",		PREC_LEAF,
/* O_FCON */		-1,	LEAF,		"fcon",		PREC_LEAF,
/* O_SCON */		-1,	LEAF,		"scon",		PREC_LEAF,
/* O_RVAL */		1,	UNARY,		"rval",		PREC_LEAF,
/* O_INDEX */		2,	null,		"index",	PREC_CALL,
/* O_INDIR */		1,	UNARY,		"*",		PREC_NOT,
/* O_DOT */		2,	null,		".",		PREC_CALL,
/* O_AMPER */		1,	UNARY,		"&",		PREC_NOT,
/* O_COMMA */		2,	BINARY,		",",		PREC_NONE,
/* O_ITOF */		1,	UNARY|INTOP,	"itof",		PREC_LEAF, 
/* O_FTOI */		1,	UNARY|REALOP,	"ftoi",		PREC_LEAF, 
/* O_ADD */		2,	BINARY|INTOP,	"+",		PREC_ADD,
/* O_ADDF */		2,	BINARY|REALOP,	"+",		PREC_ADD,
/* O_SUB */		2,	BINARY|INTOP,	"-",		PREC_ADD,
/* O_SUBF */		2,	BINARY|REALOP,	"-",		PREC_ADD,
/* O_NEG */		1,	UNARY|INTOP,	"-",		PREC_NOT,
/* O_NEGF */		1,	UNARY|REALOP,	"-",		PREC_NOT,
/* O_MUL */		2,	BINARY|INTOP,	"*",		PREC_MUL,
/* O_MULF */		2,	BINARY|REALOP,	"*",		PREC_MUL,
/* O_DIVF */		2,	BINARY|REALOP,	"/",		PREC_MUL,
/* O_DIV */		2,	BINARY|INTOP,	" div ",	PREC_MUL,
/* O_MOD */		2,	BINARY|INTOP,	" mod ",	PREC_MUL,
/* O_BITAND */		2,	BINARY|INTOP,	" & ",		PREC_BITAND,
/* O_BITOR */		2,	BINARY|INTOP,	" | ",		PREC_BITOR,
/* O_BITXOR */		2,	BINARY|INTOP,	" ^ ",		PREC_BITXOR,
/* O_RSHIFT */		2,	BINARY|INTOP,	" >> ",		PREC_SHIFT,
/* O_LSHIFT */		2,	BINARY|INTOP,	" << ",		PREC_SHIFT,
/* O_COMPL */		1,	UNARY|INTOP,	"~",		PREC_NOT,
/* O_LOGAND */		2,	BINARY|INTOP,	" && ",		PREC_LOGAND,
/* O_LOGOR */		2,	BINARY|INTOP,	" || ",		PREC_LOGOR,
/* O_LT */		2,	BINARY|INTOP,	" < ",		PREC_COMP,
/* O_LTF */		2,	BINARY|REALOP,	" < ",		PREC_COMP,
/* O_LE */		2,	BINARY|INTOP,	" <= ",		PREC_COMP,
/* O_LEF */		2,	BINARY|REALOP,	" <= ",		PREC_COMP,
/* O_GT */		2,	BINARY|INTOP,	" > ",		PREC_COMP,
/* O_GTF */		2,	BINARY|REALOP,	" > ",		PREC_COMP,
/* O_GE */		2,	BINARY|INTOP,	" >= ",		PREC_COMP,
/* O_GEF */		2,	BINARY|REALOP,	" >= ",		PREC_COMP,
/* O_EQ */		2,	BINARY|INTOP,	" == ",		PREC_EQ, 
/* O_EQF */		2,	BINARY|REALOP,	" == ",		PREC_EQ,
/* O_NE */		2,	BINARY|INTOP,	" != ",		PREC_EQ,
/* O_NEF */		2,	BINARY|REALOP,	" != ",		PREC_EQ,
/* O_NOT */		1,	UNARY|INTOP,	"!",		PREC_NOT,
/* O_SIZEOF */		1,	null,		"sizeof",	PREC_NOT,
/* O_QUEST */		3,	UNARY,		"?:",		PREC_QUEST,

/* O_ALIAS */		2,	null,		"alias",	PREC_NONE,
/* O_ASSIGN */		2,	null,		" = ",		PREC_NONE,
/* O_BOTMARGIN */	1,	null,		"toolenv botmargin", PREC_NONE,
/* O_BUTTON */		2,	null,		"button",	PREC_NONE,
/* O_CALL */		2,	null,		"call",		PREC_CALL,
/* O_CATCH */		1,	null,		"catch",	PREC_NONE,
/* O_CD */		0,	null,		"cd",		PREC_NONE,
/* O_CHFILE */		0,	null,		"file",		PREC_NONE,
/* O_CLEAR */		1,	null,		"clear",	PREC_NONE,
/* O_CMDLINES */	1,	null,		"toolenv cmdlines", PREC_NONE,
/* O_CONT */		1,	null,		"cont",		PREC_NONE,
/* O_DBXENV */		1,	null,		"dbxenv",	PREC_NONE,
/* O_DEBUG */		3,	null,		"debug",	PREC_NONE,
/* O_DELETE */		1,	null,		"delete",	PREC_NONE,
/* O_DETACH */		0,	null,		"detach",	PREC_NONE,
/* O_DISPLAY */		1, 	null,		"display",	PREC_NONE,
/* O_DISPLINES */	1, 	null,		"toolenv displines", PREC_NONE,
/* O_DOWN */		1, 	null,		"down",		PREC_NONE,
/* O_DUMP */		1,	null,		"dump",		PREC_NONE,
/* O_EDIT */		0,	null,		"edit",		PREC_NONE,
/* O_FONT */		1,	null,		"toolenv font",	PREC_NONE,
/* O_FUNC */		1,	null,		"func",		PREC_NONE,
/* O_HELP */		1,	null,		"help",		PREC_NONE,
/* O_IGNORE */		1,	null,		"ignore",	PREC_NONE,
/* O_KILL */		0,	null,		"kill",		PREC_NONE,
/* O_LIST */		2,	null,		"list",		PREC_NONE,
/* O_MAKE */		0,	null,		"make",		PREC_NONE,
/* O_MENU */		2,	null,		"menu",		PREC_NONE,
/* O_PRDBXENV */	0,	null,		"dbxenv",	PREC_NONE,
/* O_PRINT */		1,	null,		"print",	PREC_NONE,
/* O_PRTOOLENV */	0,	null,		"toolenv",	PREC_NONE,
/* O_PSYM */		1,	null,		"psym",		PREC_NONE,
/* O_PWD */		0,	null,		"pwd",		PREC_NONE,
/* O_QUIT */		0,	null,		"quit",		PREC_NONE,
/* O_RERUN */		0,	null,		"rerun",	PREC_NONE,
/* O_RUN */		0,	null,		"run",		PREC_NONE,
/* O_SEARCH */		1,  	null,		"/",		PREC_NONE,
/* O_SET81 */		4,  	null,		"set81",	PREC_NONE,
/* O_SETENV */		2,  	null,		"setenv",	PREC_NONE,
/* O_SH */		1,	null,		"sh",		PREC_NONE,
/* O_SOURCE */		0,	null,		"source",	PREC_NONE,
/* O_SOURCES */		0,	null,		"modules",	PREC_NONE,
/* O_SRCLINES */	1,	null,		"toolenv srclines", PREC_NONE,
/* O_STATUS */		0,	null,		"status",	PREC_NONE,
/* O_STEP */		0,	null,		"step",		PREC_NONE,
/* O_STOP */		3,	null,		"stop",		PREC_NONE,
/* O_STOPI */		3,	null,		"stopi",	PREC_NONE,
/* O_TOPMARGIN */	1,	null,		"toolenv topmargin", PREC_NONE,
/* O_TRACE */		3,	null,		"trace",	PREC_NONE,
/* O_TRACEI */		3,	null,		"tracei",	PREC_NONE,
/* O_UNBUTTON */	2,	null,		"unbutton",	PREC_NONE,
/* O_UNDISPLAY */	1, 	null,		"undisplay",	PREC_NONE,
/* O_UNMENU */		2,	null,		"unmenu",	PREC_NONE,
/* O_UP */		1, 	null,		"up",		PREC_NONE,
/* O_UPSEARCH */	1, 	null,		"?",		PREC_NONE,
/* O_USE */		1,	null,		"use",		PREC_NONE,
/* O_WHATIS */		1,	null,		"whatis",	PREC_NONE,
/* O_WHERE */		1,	null,		"where",	PREC_NONE,
/* O_WHEREIS */		1,	null,		"whereis",	PREC_NONE,
/* O_WHICH */		1,	null,		"which",	PREC_NONE,
/* O_WIDTH */		1,	null,		"toolenv width",PREC_NONE,
/* O_EXAMINE */		0,	null,		"",		PREC_NONE,

/* O_ADDEVENT */	0,	null,		"when",		PREC_NONE,
/* O_ENDX */		0,	null,		"endx",		PREC_NONE,
/* O_IF */		0,	null,		"if",		PREC_NONE,
/* O_ONCE */		0,	null,		"once",		PREC_NONE,
/* O_STOPONCE */	0,	null,		"stoponce",	PREC_NONE,
/* O_PRINTCALL */	1,	null,		"printcall",	PREC_NONE,
/* O_PRINTCALL_NOW */	1,	null,		"printcallnow",	PREC_NONE,
/* O_PRINTIFCHANGED */	1,	null,		"printifchanged", PREC_NONE,
/* O_PRINTRTN */	1,	null,		"printrtn",	PREC_NONE,
/* O_PRINTSRCPOS */	1,	null,		"printsrcpos",	PREC_NONE,
/* O_PRINTINSTPOS */	1,	null,		"printinstpos",	PREC_NONE,
/* O_QLINE */		2,	null,		"qline",	PREC_NONE,
/* O_STOPIFCHANGED */	1,	null,		"stopifchanged", PREC_NONE,
/* O_STOPX */		0,	null,		"stop",		PREC_NONE,
/* O_TRACEON */		1,	null,		"traceon",	PREC_NONE,
/* O_TRACEOFF */	1,	null,		"traceoff",	PREC_NONE,
/* O_TYPERENAME */	2,	UNARY,		"(typecast)",	PREC_NOT,
/* O_CALLCMD */		2,	null,		"call",		PREC_NONE,
};

