#ifndef lint
static	char sccsid[] = "@(#)tree.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Parse tree management.
 */

#include "defs.h"
#include "tree.h"
#include "operators.h"
#include "eval.h"
#include "events.h"
#include "symbols.h"
#include "scanner.h"
#include "source.h"
#include "object.h"
#include "mappings.h"
#include "library.h"
#include "process.h"
#include "machine.h"

#ifndef public
#include "lists.h"
#include "pipeout.h"

typedef struct Node *Node;
typedef Node Command;
typedef List Cmdlist;

#include "operators.h"
#include "symbols.h"
#include "events.h"

#define MAXNARGS 5

struct Node {
    Operator op;
    Symbol nodetype;
    union treevalue {
	Symbol sym;
	Name name;
	long lcon;
	double fcon;
	String scon;
	Boolean bool;
	Node arg[MAXNARGS];
	struct {
	    Node cond;
	    Cmdlist actions;
	} event;
	struct {
	    Boolean inst;
	    Event event;
	    Cmdlist actions;
	} trace;
	struct {
	    Boolean source;
	    Boolean skipcalls;
	    Integer stepamt;
	} step;
	struct {
	    String mode;
	    Node beginaddr;
	    Node endaddr;
	    Integer count;
	} examine;
	struct {
	    Node place;
	    Node sig;
	} cont;
	struct {
	    String newcmd;
	    String oldcmd;
	} alias;
	struct {
	    Boolean kernel;
	    String prog;
	    String corefile;
	} debug;
	struct {
	    Seltype seltype;
	    String string;
	} button;
	struct {
	    Node fpreg;
	    int fp1;
	    int fp2;
	    int fp3;
	} set81;
	struct {
	    String envname;
	    String envvalue;
	} setenv;
	struct {
	    int var_id;
	    Node value;
	} dbxenv;
    } value;
};

#define evalcmd(cmd) eval(cmd)
#define cmdlist_append(cmd, cl) list_append_tail(cmd, cl)

#endif

#include <varargs.h>

#define nextarg(type)  va_arg( vptr, type )

private Boolean isptr();

/*
 * Build a tree.
 */

/* VARARGS1 */
public Node build( va_alist )
		   va_dcl
{
    register Node p, q;
    Node q1;
    Symbol t;
    Integer i;
    String str;
    Operator op;
    va_list vptr;

    va_start( vptr );
    op = nextarg( Operator );

    p = new(Node);
    if (p == 0)
	fatal("No memory available. Out of swap space?");
    p->op = op;
    p->nodetype = nil;
    switch (op) {
	case O_NAME:
	    p->value.name = nextarg(Name);
	    break;

	case O_SYM:
	case O_PRINTCALL_NOW:
	case O_PRINTCALL:
	case O_PRINTRTN:
	    p->value.sym = nextarg(Symbol);
	    break;

	case O_LCON:
	case O_TRACEOFF:
	    p->value.lcon = nextarg(long);
	    break;

	case O_FCON:
	    p->value.fcon = nextarg(double);
	    break;

	case O_SCON:
	case O_CHFILE:
	case O_EDIT:
	case O_SOURCE:
	case O_SOURCES:
	case O_FONT:
	case O_CD:
	case O_SEARCH:
	case O_UPSEARCH:
	    p->value.scon = nextarg(String);
	    if (p->value.scon == nil) {
		p->value.scon = "";
	    }
	    break;

	case O_DEBUG:
	    str = nextarg(String);
		p->value.debug.kernel = false;
	        p->value.debug.prog = str;
	        p->value.debug.corefile = nextarg(String);
		str = nextarg(String);
		if (str != nil) {
		    error("Usage: debug [program [corefile]]");
	    }
	    break;

	case O_ALIAS:
	    p->value.alias.newcmd = nextarg(String);
	    str = nextarg(String);
	    if (str != nil) {
	        p->value.alias.oldcmd = strdup(str);
	    }
	    break;

        case O_BUTTON:
        case O_MENU:
	    p->value.button.seltype = nextarg(Seltype);
	    str = nextarg(String);
	    p->value.button.string = strdup(str);
	    break;

	case O_UNBUTTON:
	case O_UNMENU:
	    str = nextarg(String);
	    p->value.scon = strdup(str);
	    break;

	case O_TYPERENAME: {
	    Symbol castto;

	    q = nextarg(Node);
	    q = build(O_RVAL, q);
	    p->value.arg[0] = q;
	    p->value.arg[1] = nextarg(Node);
	    castto = p->value.arg[1]->value.sym;
	    if (castto->class == TYPEID) {
		error("Cannot cast to a struct/union");
	    }
	    if (castto == q->nodetype) {
		*p = *q;
	    }
	    break;
	}

	case O_RVAL:
	    q = nextarg(Node);
	    if (q->op != O_SYM and q->op != O_INDIR and
	      q->op != O_INDEX and q->op != O_DOT) {
		*p = *q;
		dispose(q);
	    } else {
		p->value.arg[0] = q;
	    }
	    break;

	case O_INDEX:
	    p->value.arg[0] = nextarg(Node);
	    p->value.arg[1] = nextarg(Node);
	    t = rtype(p->value.arg[0]->nodetype);
	    if (t->class == PTR and p->value.arg[0]->op != O_RVAL) {
		p->value.arg[0] = build(O_RVAL, p->value.arg[0]);
	    }
	    break;

	case O_INDIR:
	    q = nextarg(Node);
	    q1 = nocasts(q);
	    if (q1->op == O_RVAL) {		/* remove RVAL */
		p->value.arg[0] = q;
		q = q1->value.arg[0];
		*q1 = *q;
		dispose(q);
	    } else if (q->op == O_AMPER) {	/* INDIR and AMPER cancel */
		dispose(p);
		p = q->value.arg[0];
		dispose(q);
	    } else {
		p->value.arg[0] = q;
	    }
	    break;
	
	case O_AMPER:
	    q = nextarg(Node);
	    p->value.arg[0] = q;
	    q1 = nocasts(q);
	    if (q1->op == O_INDIR or q1->op == O_RVAL) { /* remove RVAL/INDIR */
		q = q1->value.arg[0];
		*q1 = *q;
		dispose(q);
	    }
	    break;

	case O_ADDEVENT:
	case O_ONCE:
	case O_STOPONCE:
	case O_IF:
	    p->value.event.cond = nextarg(Node);
	    p->value.event.actions = nextarg(Cmdlist);
	    break;

	case O_TRACEON:
	    p->value.trace.inst = nextarg(Boolean);
	    p->value.trace.event = nil;
	    p->value.trace.actions = nextarg(Cmdlist);
	    break;

	case O_CONT:
	    p->value.cont.place = nextarg(Node);
	    p->value.cont.sig = nextarg(Node);
	    break;

	case O_STEP:
	    p->value.step.source = nextarg(Boolean);
	    p->value.step.skipcalls = nextarg(Boolean);
	    p->value.step.stepamt = nextarg(Integer);
	    break;

	case O_EXAMINE:
	    p->value.examine.mode = nextarg(String);
	    p->value.examine.beginaddr = nextarg(Node);
	    p->value.examine.endaddr = nextarg(Node);
	    p->value.examine.count = nextarg(Integer);
	    break;

	case O_SET81:
	    p->value.set81.fpreg = nextarg(Node);
	    p->value.set81.fp1 = nextarg(int);
	    p->value.set81.fp2 = nextarg(int);
	    p->value.set81.fp3 = nextarg(int);
	    break;

	case O_SETENV:
	    p->value.setenv.envname = nextarg(String);
	    p->value.setenv.envvalue = nextarg(String);
	    break;

	case O_DBXENV:
	    p->value.dbxenv.var_id = nextarg(int);
	    p->value.dbxenv.value = nextarg(Node);
	    break;

	default:
	    for (i = 0; i < nargs(op); i++) {
		p->value.arg[i] = nextarg(Node);
	    }
	    break;
    }
    check(p);
    assigntypes(p);
    if (debug_flag[1]) {
	fprintf(stderr," built %s node %d (0x%x) with ", opname(p->op), p, p );
	fprintf(stderr,"arg0 %d (0x%x), arg1 %d (0x%x)\n", p->value.arg[0],
	    p->value.arg[0], p->value.arg[1], p->value.arg[1] );
    }
    return p;
}

/*
 * Create a command list from a single command.
 */

public Cmdlist buildcmdlist(cmd)
Command cmd;
{
    Cmdlist cmdlist;

    cmdlist = list_alloc();
    cmdlist_append(cmd, cmdlist);
    return cmdlist;
}

/*
 * Create a "concrete" version of a node.
 * This is necessary when the type of the node contains
 * an unresolved type reference.
 */

public Node concrete(p)
Node p;
{
    findtype(p->nodetype);
    return build(O_INDIR, p);
}

/*
 * Print out a command.
 */

public printcmd(e, cmd)
Event e;
Command cmd;
{
    register Integer i;
    register Command c;
    register Node p;
    Node place;
    Node sig;

    switch (cmd->op) {
	case O_PRINTIFCHANGED:
	case O_PRINTSRCPOS:
	case O_STOPIFCHANGED:
	case O_TRACEON:
	case O_ASSIGN:
	case O_CALLCMD:
	case O_SEARCH:
	case O_UPSEARCH:
	    break;

	case O_STEP:
	    if (cmd->value.step.skipcalls) {
		printf("next");
	    } else {
		printf("step");
	    }
	    if (not cmd->value.step.source) {
		printf("i");
	    }
	    if (cmd->value.step.stepamt != 0) {
		printf(" %d", cmd->value.step.stepamt);
	    }
	    break;

	default:
	    printf("%s", opname(cmd->op));
	    if (nargs(cmd->op) != 0) {
		printf(" ");
	    }
	    break;
    }
    switch (cmd->op) {
	case O_PRINTCALL_NOW:
	case O_PRINTCALL:
	case O_PRINTRTN:
	    printf("%s", symname(cmd->value.sym));
	    break;

	case O_PRINTSRCPOS:
	    p = cmd->value.arg[0];
	    if (p != nil and p->op != O_QLINE) {
		printf("trace ");
		prtree(p);
	    }
	    break;

	case O_CHFILE:
	case O_EDIT:
	case O_SOURCE:
	case O_CD:
	case O_FONT:
	    printf(" %s", cmd->value.scon);
	    break;

	case O_WHERE:
	    if (cmd->value.arg[0]->value.lcon > 0) {
		printf("%d", cmd->value.arg[0]->value.lcon);
	    }
	    break;

	case O_DELETE:
	    for (p = cmd->value.arg[0]; p != nil; p = p->value.arg[1]) {
		prtree(p->value.arg[0]);
		printf(" ");
	    }
	    break;

	case O_CATCH:
	case O_IGNORE:
	    for (p = cmd->value.arg[0]; p != nil; p = p->value.arg[1]) {
		printf("%s ", signo_to_name(p->value.arg[0]->value.lcon));
	    }
	    break;

	case O_TRACEOFF:
	case O_HELP:
	    printf("%d", cmd->value.arg[0]->value.lcon);
	    break;

	case O_ADDEVENT:
	case O_ONCE:
	case O_STOPONCE:
	case O_IF:
	    printf(" ");
	    prtree(cmd->value.event.cond);
	    printf(" { ");
	    foreach (Command, c, cmd->value.event.actions)
		printcmd(e, c);
		if (not list_islast()) {
		    printf(";");
		}
	    endfor
	    printf(" }");
	    break;

	case O_TRACEON:
	    print_tracestop(e, cmd);
	    break;

	case O_EXAMINE:
	    prtree(cmd->value.examine.beginaddr);
	    if (cmd->value.examine.endaddr != nil) {
		printf(",");
		prtree(cmd->value.examine.endaddr);
	    }
	    printf("/");
	    if (cmd->value.examine.count > 1) {
		printf("%d", cmd->value.examine.count);
	    }
	    printf("%s", cmd->value.examine.mode);
	    break;

	case O_ASSIGN:
	    printf("assign ");
	    prtree(cmd);
	    break;

	case O_CALLCMD:
	    printf("call ");
	    prtree(cmd);
	    break;

	case O_CONT:
	    place = cmd->value.cont.place;
	    sig = cmd->value.cont.sig;
	    if (place != nil) {
		printf("at ");
		prtree(place);
	    }
	    if (sig != nil) {
		printf(" sig %s", signo_to_name(sig->value.lcon));
	    }
	    break;

	case O_LIST: 
	    if (cmd->value.arg[0]->op == O_SYM) {
		prtree(cmd->value.arg[0]);
	    } else {
		prtree(cmd->value.arg[0]);
		printf(", ");
		prtree(cmd->value.arg[1]);
	    }
	    break;

	case O_SEARCH:
	case O_UPSEARCH:
	    printf("%s%s%s", opname(cmd->op), cmd->value.scon, opname(cmd->op));
	    break;

	case O_ALIAS:
	    printf("%s \"%s\"", cmd->value.alias.newcmd, 
		cmd->value.alias.oldcmd);
	    break;

	case O_BUTTON:
	case O_MENU:
	    pr_seltype(cmd->value.button.seltype);
	    printf(" \"%s\"", cmd->value.button.string);
	    break;

	case O_UNBUTTON:
	case O_UNMENU:
	    printf(" \"%s\"", cmd->value.button.string);
	    break;
	
	case O_DEBUG:
	    if (cmd->value.debug.prog != nil) {
	        printf("%s ", cmd->value.debug.prog);
	    }
	    if (cmd->value.debug.corefile != nil) {
	        printf("%s ", cmd->value.debug.corefile);
	    }
	    break;
	
	case O_SET81:
	    printf("%s = 0x%08x 0x%08x 0x%08x", 
	      symname(cmd->value.set81.fpreg->value.sym),
	      cmd->value.set81.fp1, cmd->value.set81.fp2, cmd->value.set81.fp3);
	    break;

	case O_SETENV:
	    printf("%s \"%s\"", cmd->value.setenv.envname, 
	      cmd->value.setenv.envvalue);
	    break;

	case O_DBXENV:
	    print_dbxenv_cmd(cmd);
	    break;

	default:
	    if (nargs(cmd->op) != 0) {
		i = 0;
		for (;;) {
		    prtree(cmd->value.arg[i]);
		    ++i;
		if (i >= nargs(cmd->op)) break;
		    printf(" ");
		}
	    }
	    break;
    }
}

/*
 * Print out a trace/stop command name.
 */

private print_tracestop(e, cmd)
Event e;
Command cmd;
{
    register Command c, ifcmd, stopcmd;
    Boolean done;

    done = false;
    ifcmd = list_element(Command, list_head(cmd->value.trace.actions));
    checkref(ifcmd);
    if (ifcmd->op == O_IF) {
	stopcmd = list_element(Command, list_head(ifcmd->value.event.actions));
	checkref(stopcmd);
	if (stopcmd->op == O_STOPX) {
	    print_cmdname(cmd, "stop");
	    printf(" if ");
	    prtree(ifcmd->value.event.cond);
	    done = true;
	}
    } else if (ifcmd->op == O_STOPIFCHANGED) {
	print_cmdname(cmd, "stop");
	printf(" ");
	prtree(ifcmd->value.arg[0]);
	done = true;
    }
    if (not done) {
	print_cmdname(cmd, "trace");
	foreach (Command, c, cmd->value.trace.actions)
	    printcmd(e, c);
	    if (not list_islast()) {
		printf(";");
	    }
	endfor
	printcond(e->condition);
    }
}

/*
 * Print out a tree.
 */

public prtree(p)
register Node p;
{
    register Node q;
    Operator op;
    Symbol s;

    if (p != nil) {
	op = p->op;
	if (ord(op) > ord(O_LASTOP)) {
	    panic("bad op %d in prtree", p->op);
	}
	switch (op) {
	    case O_NAME:
		printf("%s", idnt(p->value.name));
		break;

	    case O_SYM:
		printname(p->value.sym);
		break;

	    case O_QLINE:
		if (nlhdr.nfiles > 1) {
		    prtree(p->value.arg[0]);
		    printf(":");
		}
		prtree(p->value.arg[1]);
		break;

	    case O_LCON:
		if (compatible(p->nodetype, t_char)) {
		    printf("'%c'", p->value.lcon);
		} else {
		    printf("%d", p->value.lcon);
		}
		break;

	    case O_FCON:
		printf("%.18g", p->value.fcon);
		break;

	    case O_SCON:
		printf("\"%s\"", p->value.scon);
		break;

	    case O_INDEX:
		if (strip_) {				/* fortran */
		    for (q = p->value.arg[0]; q!=nil; q = q->value.arg[0]) {
			if (q->op != O_INDEX) {
			    prtree_parens(op, q);/* print the array name */
			    break;
			}
		    }
		    printf("[");
		    prtree(p->value.arg[1]);
		    for (q = p->value.arg[0]; q!=nil; q = q->value.arg[0]) {
			if (q->op == O_INDEX) {
		    	    printf(", ");
			    prtree(q->value.arg[1]);
			} else {
			    printf("]");
			    break;
			}
		    }
		} else {				/* C */
		    prtree_parens(op, p->value.arg[0]);
		    printf("[");
		    prtree(p->value.arg[1]);
		    printf("]");
		}
		break;

	    case O_COMMA:
		prtree(p->value.arg[0]);
		if (p->value.arg[1] != nil) {
		    printf(", ");
		    prtree(p->value.arg[1]);
		}
		break;

	    case O_RVAL:
		q = p->value.arg[0];
		while (q->op == O_INDIR) {
			q = q->value.arg[0];
		}
		if (q->op == O_SYM) {
		    if (isreg(q->value.sym)) {
			printf("*");
		    }
		}
		prtree(p->value.arg[0]);
		break;

	    case O_ITOF:
	    case O_FTOI:
		prtree(p->value.arg[0]);
		break;

	    case O_CALL:
	    case O_CALLCMD:
		prtree_parens(op, p->value.arg[0]);
		printf("(");
		if (p->value.arg[1]!= nil) {
		    prtree(p->value.arg[1]);
		}
		printf(")");
		break;

	    case O_DOT:
		q = p->value.arg[0];
		if (q->op == O_INDIR) {
		    while (q->op == O_INDIR) {
		        q = q->value.arg[0];
		    }
		    if (q->op == O_SYM && isreg(q->value.sym)) {
			q = p->value.arg[0];
		    } else {
			q = p->value.arg[0]->value.arg[0];
		    }
		    prtree_parens(op, q);
		    printf("->%s", symname(p->value.arg[1]->value.sym));
		} else {
		    prtree_parens(op, q);
		    s = rtype(q->nodetype);
		    if (s != nil and s->class == PTR) {
		    	printf("->%s", symname(p->value.arg[1]->value.sym));
		    } else {
		    	printf(".%s", symname(p->value.arg[1]->value.sym));
		    }
		}
		break;

	    case O_ASSIGN:
		prtree(p->value.arg[0]);
		printf("%s", opname(op));
		prtree(p->value.arg[1]);
		break;

	    case O_TYPERENAME:
		/* should we print cast here in C syntax? */
		prcast(p->value.arg[1]);
		prtree_parens(op, p->value.arg[0]);
		break;

	    case O_SIZEOF:
	        printf("%s", opname(p->op));
		if (p->value.arg[0]->op == O_SYM and 
		  not isvariable(p->value.arg[0]->value.sym)) {
		    prcast(p->value.arg[0]);
		} else {
	            printf("(");
	            prtree(p->value.arg[0]);
	            printf(")");
		}
	        break;

	    case O_QUEST:
		printf("(");
		prtree(p->value.arg[0]);
		printf(")");
		printf(" ? ");
		prtree(p->value.arg[1]);
		printf(" : ");
		prtree(p->value.arg[2]);
		break;

	    default:
		switch (degree(op)) {
		    case BINARY:
			prtree_parens(op, p->value.arg[0]);
			printf("%s", opname(op));
			prtree_parens(op, p->value.arg[1]);
			break;

		    case UNARY:
			printf("%s", opname(op));
			prtree_parens(op, p->value.arg[0]);
			break;

		    default:
			error("error in parse tree (check syntax)");
		}
		break;
	}
    }
}

/*
 * Front end to prtree() that parenthesizes things.
 * Given a parent and child operator.
 * There is an ugly special case here for the RVAL node.
 * If an RVAL is over an INDIR, then the RVAL has the 
 * precedence of an INDIR.  If an RVAL is over a SYM
 * that is a register variable, then it also has the
 * precedence of an INDIR.
 * Type conversion nodes also get in the way.
 * The expression "(i + j)/k" yields
 *		/
 *        ITOF     ITOF
 *         +         k
 *       i   j 
 */
private prtree_parens(parentop, childnode)
Operator parentop;
Node	childnode;
{
    Operator childop;
    Node kid;

    kid = childnode;
    childop = childnode->op;
    while (childop == O_ITOF or childop == O_FTOI) {
	childnode = childnode->value.arg[0];
	childop = childnode->op;
    }
    if (childop == O_RVAL) {
	if (childnode->value.arg[0]->op == O_INDIR) {
	    childop = O_INDIR;
	} else if (childnode->value.arg[0]->op == O_SYM and
	  isreg(childnode->value.arg[0]->value.sym)) {
	    childop = O_INDIR;
	}
    }
    if (opprec(parentop) > opprec(childop)) {
	printf("(");
	prtree(kid);
	printf(")");
    } else {
	prtree(kid);
    }
}

/*
 * Given a node, skip all casts (TYPERENAME's) and return a
 * non-cast node.
 */
public Node nocasts(p)
Node p;
{
    while (p->op == O_TYPERENAME) {
	p = p->value.arg[0];
    }
    return(p);
}

/*
 * Free storage associated with a tree.
 */

public tfree(p)
Node p;
{
    Integer i;

    if (p == nil) {
	return;
    }
    switch (p->op) {
	case O_QLINE:
	    dispose(p->value.arg[0]->value.scon);
	    dispose(p->value.arg[0]);
	    tfree(p->value.arg[1]);
	    break;

	case O_SCON:
	    unmkstring(p->nodetype);
	    dispose(p->nodetype);
	    dispose(p->value.scon);
	    break;

	default:
	    for (i = 0; i < nargs(p->op); i++) {
		tfree(p->value.arg[i]);
	    }
	    break;
    }
    dispose(p);
}

/*
 * A recursive tree search routine to test if two trees 
 * are equivalent.
 */

public Boolean tr_equal(t1, t2)
register Node t1;
register Node t2;
{
    register Boolean b;

    if (t1 == nil and t2 == nil) {
	b = true;
    } else if (t1 == nil or t2 == nil) {
	b = false;
    } else if (t1->op != t2->op or degree(t1->op) != degree(t2->op)) {
	b = false;
    } else {
	switch (degree(t1->op)) {
	    case LEAF:
		switch (t1->op) {
		    case O_NAME:
			b = (Boolean) (t1->value.name == t2->value.name);
			break;

		    case O_SYM:
			b = (Boolean) (t1->value.sym == t2->value.sym);
			break;

		    case O_LCON:
			b = (Boolean) (t1->value.lcon == t2->value.lcon);
			break;

		    case O_FCON:
			b = (Boolean) (t1->value.fcon == t2->value.fcon);
			break;

		    case O_SCON:
			b = (Boolean) (t1->value.scon == t2->value.scon);
			break;

		    default:
			panic("tr_equal: leaf %d\n", t1->op);
		}
		/*NOTREACHED*/

	    case BINARY:
		if (not tr_equal(t1->value.arg[0], t2->value.arg[0])) {
		    b = false;
		} else {
		    b = tr_equal(t1->value.arg[1], t2->value.arg[1]);
		}
		break;

	    case UNARY:
		b = tr_equal(t1->value.arg[0], t2->value.arg[0]);
		break;

	    default:
		panic("tr_equal: bad degree for op %d\n", t1->op);
	}
    }
    return b;
}

/*
 * Put an indirection in front of an expression for an EXAMINE node.
 */
public Node examine_indir(p)
Node p;
{
    if (p->op == O_AMPER) {
	p->op = O_RVAL;
    } else {
	p = build(O_INDIR, p);
    }
    return(p);
}
