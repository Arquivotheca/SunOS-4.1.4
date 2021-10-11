#ifndef lint
static  char sccsid[] = "@(#)events.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Event/breakpoint managment.
 */

#include "defs.h"
#include "events.h"
#include "symbols.h"
#include "tree.h"
#include "eval.h"
#include "source.h"
#include "mappings.h"
#include "process.h"
#include "machine.h"
#include "dbxmain.h"
#include "lists.h"
#include "runtime.h"
#include "object.h"
#include "display.h"

#ifdef mc68000
#  include "cp68881.h"
#  include "fpa_inst.h"
#elif i386
#  include "fpa_inst.h"
#endif mc68000

#ifndef public
typedef struct Event *Event;
typedef struct Breakpoint *Breakpoint;

Boolean inst_tracing;
Boolean single_stepping;
Boolean isstopped;

int dirty_exit;		/* see comment above resetanybps() */

#include "symbols.h"

Symbol linesym;
Symbol procsym;
Symbol pcsym;
Symbol retaddrsym;

#define addevent(cond, cmdlist) event_alloc(nil, false, cond, cmdlist, false)
#define addtraceevent(e, cond, cmdlist) event_alloc(e, false, cond, cmdlist, false)
#define addstopevent(cond, cmdlist) event_alloc(nil, false, cond, cmdlist, true)
#define event_once(cond, cmdlist) event_alloc(nil, true, cond, cmdlist, false)
#define stop_once(cond, cmdlist) event_alloc(nil, true, cond, cmdlist, true)

struct Event {
    unsigned int id;
    Boolean temporary;
    Node condition;
    Cmdlist actions;
};

#endif

struct Breakpoint {
    Event event;
    Address bpaddr;	
    Lineno bpline;
    Cmdlist actions;
};

typedef List Eventlist;
typedef List Bplist;

#define eventlist_append(event, el)  list_append_tail(event, el)
#define eventlist_prepend(event, el) list_append_head(event, el)
#define bplist_append(bp, bl) list_append_tail(bp, bl)

private Eventlist eventlist;		/* list of active events */
private Bplist bplist;			/* list of active breakpoints */
private Integer eventid;		/* id number of next allocated event */
private Integer trid;			/* id number of next allocated trace */

typedef struct Trcmd {
    Integer trid;
    Event event;
    Cmdlist cmdlist;
} *Trcmd;

private List eachline;		/* commands to execute after each line */
private List eachinst;		/* commands to execute after each instruction */

private Breakpoint bp_alloc();

public Event newevent()
{
	Event e;

	e = new(Event);
	if (e == 0)
		fatal("No memory available. Out of swap space?");
	return(e);
}

/*
 * Initialize breakpoint information.
 */

private Symbol builtinsym(str, class, type)
String str;
Symclass class;
Symbol type;
{
    Symbol s;

    s = insert(identname(str, true));
    s->language = findlanguage(".s");
    s->class = class;
    s->type = type;
    return s;
}

public bpinit()
{
    linesym = builtinsym("$line", VAR, t_int);
    procsym = builtinsym("$proc", PROC, nil);
    pcsym = lookup(identname("$pc", true));
    if (pcsym == nil) {
	panic("can't find $pc");
    }
    retaddrsym = builtinsym("$retaddr", VAR, t_int);
    eventlist = list_alloc();
    bplist = list_alloc();
    eachline = list_alloc();
    eachinst = list_alloc();
}

/*
 * Trap an event and do the associated commands when it occurs.
 */

public Event event_alloc(evnt, istmp, econd, cmdlist, stopcmd)
Event evnt;
Boolean istmp;
Node econd;
Cmdlist cmdlist;
Boolean stopcmd;
{
    register Event e;

    if (evnt == nil) {
    	e = new(Event);
	if (e == 0)
		fatal("No memory available. Out of swap space?");
    } else {
	e = evnt;
    }
    ++eventid;
    e->id = eventid;
    e->temporary = istmp;
    e->condition = econd;
    e->actions = cmdlist;
    translate(e, stopcmd);
    /* the following pattern test is a kludge */
    if(istmp and stopcmd)
	eventlist_prepend (e, eventlist); /* must be a trace entry */
    else
	eventlist_append(e, eventlist);
    return e;
}

/*
 * Delete the event with the given id.  If "all" is true, delete all
 * trace/bpts.
 */

public delevent(id, all)
unsigned int id;
register Boolean all;
{
    Event e;
    Breakpoint bp;
    Trcmd t;
    Boolean found;

    found = false;
    foreach (Event, e, eventlist)
	if (all or e->id == id) {
	    found = true;
	    event_delete(e->condition);
	    list_delete(list_curitem(eventlist), eventlist);
	    foreach (Breakpoint, bp, bplist)
		if (bp->event == e) {
		    list_delete(list_curitem(bplist), bplist);
		}
	    endfor
	    if (not all) {
	    	break;
	    }
	}
    endfor
    foreach (Trcmd, t, eachline)
	if (all or t->event->id == id) {
	    found = true;
	    printrmtr(t);
	    list_delete(list_curitem(eachline), eachline);
	}
    endfor
    foreach (Trcmd, t, eachinst)
	if (all or t->event->id == id) {
	    found = true;
	    printrmtr(t);
	    list_delete(list_curitem(eachinst), eachinst);
	}
    endfor
    if (list_size(eachinst) == 0) {
	inst_tracing = false;
	if (list_size(eachline) == 0) {
	    single_stepping = false;
	}
    }
    if (not found and not all) {
	warning("Event %d not found", id);
    }
}

/*
 * Translate an event into the appropriate breakpoints and actions.
 * While we're at it, turn on the breakpoints if the condition is true.
 * If stopcmd is true, which means we're stopping at a proc, set bp
 * at first source line in procedure.  (Otherwise, since trace is
 * not on, we'll stop at codeloc and be unable to print out source line.)
 */

private translate(e, stopcmd)
Event e;
{
    Symbol s;
    Node place;
    Lineno line;
    Address addr;

    checkref(e->condition);
    switch (e->condition->op) {
	case O_EQ:
	    if (e->condition->value.arg[0]->op == O_SYM) {
		s = e->condition->value.arg[0]->value.sym;
		place = e->condition->value.arg[1];
		if (s == linesym) {
		    if (place->op == O_QLINE) {
			line = place->value.arg[1]->value.lcon;
			addr = objaddr(line,
			    place->value.arg[0]->value.scon, true);
		    } else {
			eval(place);
			line = pop(long);
			addr = objaddr(line, cursource, true);
		    }
		    if (addr == NOADDR) {
			beginerrmsg();
			printf("no executable code at line ");
			prtree(place);
			enderrmsg();
		    }
		    bp_alloc(e, addr, line, e->actions);
		} else if (s == procsym) {
		    eval(place);
		    s = pop(Symbol);
		    if (stopcmd and ((addr = firstline(s)) != NOADDR)) {
			bp_alloc(e, addr, 0, e->actions);
		    } else {
			bp_alloc(e, codeloc(s), 0, e->actions);
		    }
		    /* why are we doing this??? just allocate, dont evaluate */
/*		    if (isactive(s) and pc != codeloc(program)) {
			evalcmdlist(e->actions);
		    } */
		} else if (s == pcsym) {
		    if (place->op == O_QLINE) {
			/* puke. This should be done by eval(QLINE) */
			/* who cares, the grammer makes this unreachable.. */
			line = place->value.arg[1]->value.lcon;
			addr = objaddr(line,
			    place->value.arg[0]->value.scon, true);
		    } else {
			eval(place);
			addr = pop(Address);
		    }
		    bp_alloc(e, addr, 0, e->actions);
		} else {
		    condbp(e);
		}
	    } else {
		condbp(e);
	    }
	    break;

	/*
	 * These should be handled specially.
	 * But for now I'm ignoring the problem.
	 */
	case O_LOGAND:
	case O_LOGOR:
	default:
	    condbp(e);
	    break;
    }
}

/*
 * Create a breakpoint for a condition that cannot be pinpointed
 * to happening at a particular address, but one for which we
 * must single step and check the condition after each statement.
 */

private condbp(e)
Event e;
{
    Symbol p;
    Cmdlist actions;

    p = tcontainer(e->condition);
    if (p != nil and p->class == MODUS) {
        for (p != nil; p->class == MODUS; p = p->block);
    }
    if (p == nil) {
	p = program;
    }
    actions = buildcmdlist(build(O_IF, e->condition, e->actions));
    actions = buildcmdlist(build(O_TRACEON, false, actions));
    bp_alloc(e, codeloc(p), 0, actions);
}

/*
 * Determine the deepest nested subprogram that still contains
 * all elements in the given expression.  Don't return an inline
 * block.
 */

public Symbol tcontainer(exp)
Node exp;
{
    Integer i;
    Symbol s, t, u, v;

    checkref(exp);
    s = nil;
    if (exp->op == O_SYM) {
	s = container(exp->value.sym);
	skipInlineBlocks(s);
    } else if (not isleaf(exp->op)) {
	for (i = 0; i < nargs(exp->op); i++) {
	    t = tcontainer(exp->value.arg[i]);
	    if (t != nil) {
		if (s == nil) {
		    s = t;
		} else {
		    u = s;
		    v = t;
		    while (u != v and u != nil) {
			u = container(u);
			skipInlineBlocks(u);
			v = container(v);
			skipInlineBlocks(v);
		    }
		    if (u == nil) {
			panic("bad ancestry for \"%s\"", symname(s));
		    } else {
			s = u;
		    }
		}
	    }
	}
    }
    return s;
}

/*
 * Determine if the given function can be executed at full speed.
 * This can only be done if there are no breakpoints within the function.
 */

public Boolean canskip(f)
Symbol f;
{
    Breakpoint p;
    Boolean ok;

    ok = true;
    foreach (Breakpoint, p, bplist)
	if (whatblock(p->bpaddr, true) == f) {
	    ok = false;
	    break;
	}
    endfor
    return ok;
}

/*
 * Print out what's currently being traced by looking at
 * the currently active events.
 *
 * Some convolution here to translate internal representation
 * of events back into something more palatable.
 */

public status_cmd()
{
    Event e;

    foreach (Event, e, eventlist)
	if (not e->temporary) {
	    printevent(e);
	}
    endfor
}

/*
 * Print an event in a form that is similar to what the user typed in.
 * This form can also be read back in.
 */
public printevent(e)
Event e;
{
    Command cmd;

    if (not isredirected()) {
	printf("(%d) ", e->id);
    }
    cmd = list_element(Command, list_head(e->actions));
    switch (cmd->op) {
    case O_PRINTCALL:				/* trace func */
	print_call(e->condition);
	break;
    case O_PRINTSRCPOS:				/* trace line, or */
	print_srcpos(e, cmd);			/* trace var at line */
	break;
    case O_PRINTINSTPOS:			/* tracei */
	print_instpos(e, cmd);
	break;
    case O_TRACEON:				/* when exp {...}, */
	print_traceon(e, cmd);			/* or trace i, or stop i */
	break;
    case O_STOPX:				/* stop in, or stop at */
	printcmd(e, cmd);
	printcond(e->condition);
	break;
    case O_IF:					/* stop ... if cond, or */
	print_condstop(e, cmd);			/* trace ... if cond */
	break;
    default:
	print_when(e);				/* when at linenum {...} */
	break;
    }
    printf("\n");
}

/*
 * Print the command for a conditional stop or trace commands such as
 *    trace main if i == 5
 *    trace at 13 if i == 6
 *    trace i at 13 if j == 5
 *    stop at 13 if i == 4
 */
private print_condstop(e, cmd)
Event e;
Command cmd;
{
    Cmdlist actlist;
    Node action;

    actlist = cmd->value.event.actions;
    action = list_element(Node, list_head(actlist));
    switch (action->op) {
    case O_PRINTCALL:				/* trace func if cond */
	print_call(e->condition);
	break;
    case O_PRINTSRCPOS:				/* trace at line if cond, or */
	print_srcpos(e, action);		/* trace var at line if cond */
	break;
    default:					/* stop at line if cond */
	printf("stop ");
        printcond(e->condition);
	break;
    }
    print_ifcond(cmd->value.event.cond);
}

/*
 * Print the command for a O_PRINTSRCPOS node
 */
private print_srcpos(e, cmd)
Event e;
Node cmd;
{
    Node p;

    printf("trace ");			/* trace var at line*/
    p = cmd->value.arg[0];
    if (p->op != O_QLINE) {
        prtree(p);
    }
    printcond(e->condition);
}
/*
 * Print the command for a O_PRINTINSTPOS node
 */
private print_instpos(e, cmd)
Event e;
Node cmd;
{
    Node p;

    printf("tracei ");			/* trace instruction */
    printcond(e->condition);
}

/*
 * Print the command for a O_PRINTCALL node
 * The conditon is of the form:
 *          O_EQ
 *         /    \
 *      $proc   funcname
 */
private print_call(cond)
Node cond;
{
    printf("trace ");
    prtree(cond->value.arg[1]);
}

/*
 * Print the command for a 'when' command.
 * Print this out in a form that is acceptable as input.
 */
private print_when(e)
Event e;
{
    printf("when ");
    printcond(e->condition);
    prcmdlist(e->actions);
}

/*
 * Print a list of commands separated by semicolons and enclosed
 * in curly braces.
 */
private prcmdlist(cmdlist)
Cmdlist	cmdlist;
{
    Command	cmd;

    printf(" { ");
    foreach (Command, cmd, cmdlist)
        printcmd(nil, cmd);
    	printf("; ");
    endfor
    printf(" }");
}

/*
 * Have a O_TRACEON node.  This may be
 *    when exp {...}
 *    trace var
 *    trace var if cond
 *    trace in func if cond
 *    stop var
 *    stop var if cond
 *    stop if cond
 */
private print_traceon(e, cmd)
Event e;
Command cmd;
{
    Command ifcmd;
    Command action;

    ifcmd = list_element(Command, list_head(cmd->value.trace.actions));
    if (ifcmd->op == O_IF) {
	action = list_element(Command, list_head(ifcmd->value.event.actions));
	switch (action->op) {
	case O_PRINTIFCHANGED:			/* trace var if cond */
	    print_cmdname(cmd, "trace");
	    printcmd(e, list_element(Command, list_head(ifcmd->value.event.actions)));
	    printcond(e->condition);
	    print_ifcond(ifcmd->value.event.cond);
	    break;
	case O_STOPIFCHANGED:			/* stop var if cond */
	    print_cmdname(cmd, "stop");
	    printcmd(e, list_element(Command, list_head(ifcmd->value.event.actions)));
	    print_ifcond(ifcmd->value.event.cond);
	    break;
	case O_STOPX:				/* stop if cond */
	    print_cmdname(cmd, "stop");
	    print_ifcond(ifcmd->value.event.cond);
	    break;
	case O_PRINTSRCPOS:			/* trace in func if cond */
	case O_PRINTINSTPOS:			/* tracei */
	    if (action->op == O_PRINTSRCPOS)
		print_cmdname(cmd, "trace");
	    else
		print_cmdname(cmd, "tracei");
	    printcond(e->condition);
	    print_ifcond(ifcmd->value.event.cond);
	    break;
	default:				/* when exp {...} */
	    printf("when ");
	    prtree(ifcmd->value.event.cond);
	    prcmdlist(ifcmd->value.event.actions);
	    break;
	}
    } else {					/* trace i, or stop i */
	printcmd(e, cmd);
    }
}

/*
 * Print the name of a stop or trace command.
 * Append an 'i' to the name if it is in instruction mode.
 */
public print_cmdname(cmd, name)
Command cmd;
char *name;
{
    printf("%s", name);
    if (cmd->value.trace.inst) {
	putchar('i');
    }
    putchar(' ');
}

/*
 * Print an 'if' condition
 */
private print_ifcond(cond)
Node cond;
{
    printf(" if ");
    prtree(cond);
}

/*
 * Print out a condition.
 */
public printcond(cond)
Node cond;
{
    Symbol s;
    Node place;

    if (cond->op == O_EQ and cond->value.arg[0]->op == O_SYM) {
	s = cond->value.arg[0]->value.sym;
	place = cond->value.arg[1];
	if (s == procsym) {
	    if (place->value.sym != program) {
		printf(" in ");
		printname(place->value.sym);
	    }
	} else if (s == linesym or s == pcsym or s == retaddrsym) {
	    printf(" at ");
	    prtree(place);
	} else {
	    printf(" when ");
	    prtree(cond);
	}
    } else {
	printf(" when ");
	prtree(cond);
    }
}

/*
 * Add a breakpoint to the list and return it.
 */

private Breakpoint bp_alloc(e, addr, line, actions)
Event e;
Address addr;
Lineno line;
Cmdlist actions;
{
    register Breakpoint p;

    p = new(Breakpoint);
    if (p == 0)
	fatal("No memory available. Out of swap space?");
    p->event = e;
    p->bpaddr = addr;
    p->bpline = line;
    p->actions = actions;
    if (tracebpts) {
	printf("new bp at 0x%x\n", addr);
    }
    bplist_append(p, bplist);
    return p;
}

/*
 * Determine if the program stopped at a known breakpoint
 * and if so do the associated commands.
 */

private Boolean bpact0(return_ifnotfound)
Boolean return_ifnotfound;
{
    register Breakpoint p;
    Boolean found;
    jmp_buf save_env;

    List actlist;
    Cmdlist a;

    found = false;

#ifdef OLD
    foreach (Breakpoint, p, bplist)
	if (p->bpaddr == pc) {
	    if (tracebpts) {
		printf("breakpoint found at location 0x%x\n", pc);
	    }
	    found = true;
	    if (p->event->temporary) {
		delevent(p->event->id, false);
	    }
	    batchcmd = true;
	    setcurfunc(whatblock(pc, true));
	    copy_jmpbuf(env, save_env);
	    if (setjmp(env) == 0) {
		evalcmdlist(p->actions);
	    }
	    copy_jmpbuf(save_env, env);
	}
    endfor
#endif


    /* The following has to be done is two steps because execution of 
    /* 'evalcmdlist()' might recursively call this routine, in which case
    /* we'll end up deleting bkpt's which are aliased, in the previous
    /* invokation of this frame.		--- ivan
    /* See bug 1012552
    /*........................................................................*/

    actlist = list_alloc();

	/* Find a match, delete all the temporary events while collecting
	/* the commands to be evaluated.
	/*....................................................................*/
	foreach (Breakpoint, p, bplist)
	    if (p->bpaddr == pc) {
		if (tracebpts) {
		    printf("breakpoint found at pc == 0x%x (event %d)\n", pc, p->event->id);
		}
		found = true;
		list_append_tail(p->actions, actlist);
		if (p->event->temporary) {
		    delevent(p->event->id, false);
		}
	    }
	endfor

	/* Do the collected commands
	/*....................................................................*/
	foreach (Cmdlist, a, actlist)
		batchcmd = true;
		setcurfunc(whatblock(pc, true));
		copy_jmpbuf(env, save_env);
		if (setjmp(env) == 0) {
		    evalcmdlist(a);
		}
		copy_jmpbuf(save_env, env);
		list_delete (list_curitem(actlist), actlist);
	endfor

    dispose(actlist);

    if ( is_call_retaddr(pc) ) {
	restoretty( 0 );
	longjmp(env, CALL_RETURN);
    }
    return found;
}

private bpact2(return_ifnotfound, found)
Boolean return_ifnotfound;
Boolean found;
{
    if (not isstopped or multi_stepping or (return_ifnotfound and not found)) {
        fflush(stdout);
        return found;
    }
    printstatus();
    /* NOTREACHED */
}

/* bpact is split into two pieces:  bpact0 runs the breakpoint actions\
 * returning true if an action was found.  bpact2(ret_if_found, found)\
 * implements the flow of control aspects at the end of bpact.\
 * Now, printnews can run the bpactions, then the tracecmds, and still\
 * get back to do the return or printstatus or whatever.
 */

public Boolean bpact(return_ifnotfound)
Boolean return_ifnotfound;
{
    return bpact2(return_ifnotfound, bpact0());
}

/*
 * Begin single stepping and executing the given commands after each step.
 * If the first argument is true step by instructions, otherwise
 * step by source lines.
 *
 * We automatically set a breakpoint at the end of the current procedure
 * to turn off the given tracing.
 */

public traceon(inst, event, cmdlist)
Boolean inst;
Event event;
Cmdlist cmdlist;
{
    register Trcmd trcmd;
    Node until;
    Cmdlist actions;
    Address ret;

    trcmd = new(Trcmd);
    if (trcmd == 0)
	fatal("No memory available. Out of swap space?");
    ++trid;
    trcmd->trid = trid;
    trcmd->event = event;
    trcmd->cmdlist = cmdlist;
    single_stepping = true;
    if (inst) {
	inst_tracing = true;
	list_append_tail(trcmd, eachinst);
    } else {
	list_append_tail(trcmd, eachline);
    }
    ret = return_addr();
	if(ret != 0) {
		Symbol rets = whatblock(ret, true);
		if(rets != nil && streq(symname(rets), "_start"))
			ret = get_exit_addr();
	}
    if (ret == 0) {
	ret = get_exit_addr();
    }
    if (ret != 0) {
	if (linelookup(ret) == 0) {
	    ret = srcaddr(ret);		/* get addr of next source line */
	}
	until = build(O_EQ, build(O_SYM, pcsym), build(O_LCON, ret));
	actions = buildcmdlist(build(O_TRACEOFF, trcmd->trid));
	event_once(until, actions);
    }
    if (tracebpts) {
	if (event != nil) {
	    printf("adding trace %d for event %d\n",
	      trcmd->trid, event->id);
	}
    }
}

/*
 * Turn off some kind of tracing.
 * Strictly an internal command, this cannot be invoked by the user.
 */

private int deleted_tbps[50];

public traceoff(id)
Integer id;
{
    register Trcmd t;
    register Boolean found;
    int i;

    found = false;
    foreach (Trcmd, t, eachline)
	if (t->trid == id) {
	    printrmtr(t);
	    list_delete(list_curitem(eachline), eachline);
	    found = true;
	    break;
	}
    endfor
    if (not found) {
	foreach (Trcmd, t, eachinst)
	    if (t->event->id == id) {
		printrmtr(t);
		list_delete(list_curitem(eachinst), eachinst);
		found = true;
		break;
	    }
	endfor
	if (not found) {
	    for (i = 0; i < 25; i++) {
		if (deleted_tbps[i] == id) {
		    deleted_tbps[i] = -1;
		    found = true;
		    break;
		}
	    }
	    if (not found) {
	    	panic("missing trid %d", id);
	    }
	}
    }
    if (list_size(eachinst) == 0) {
	inst_tracing = false;
	if (list_size(eachline) == 0) {
	    single_stepping = false;
	}
    }
}

/*
 * If breakpoints are being traced, note that a Trcmd is being deleted.
 * Keep track of those deleted in case there's a traceoff still lying
 * around.
 */

private printrmtr(t)
Trcmd t;
{
    static int once = 0;
    register int i;

    if (once == 0) {
	once++;
	for (i = 0; i < 25; i++) {
	    deleted_tbps[i] = -1;
	}
    }
    for (i = 0; i < 25; i++) {
	if (deleted_tbps[i] == -1) {
	    break;
	}
    }
    if (i == 25) {
	i = 0;
    }
    deleted_tbps[i] = t->trid;
    if (tracebpts) {
	printf("removing trace %d", t->trid);
	if (t->event != nil) {
	    printf(" for event %d", t->event->id);
	}
	printf("\n");
    }
}

/*
 * Print out news during single step tracing.
 */

public printnews()
{
    register Trcmd t;
    register Breakpoint p;
    Boolean bpfound, found;
    Command c;

    /* do bpact0 first: check for and print results of previous execution,
     * before allowing the [trace line] to say we are at next line.
     * really, [trace line] or [trace in func] should always be *last*
     * even amoung the trcmds.
     * [trace var in func] should precede printing the next line.
     */
    bpfound = bpact0();
    found = false;
    foreach (Breakpoint, p, bplist)
	if (p->bpaddr == pc) {
	    foreach (Command, c, p->actions)
		if (c->op == O_TRACEOFF) {
		    found = true;
		}
	    endfor
	}
    endfor
    if (found == false) {
    	foreach (Trcmd, t, eachline)
 	    evalcmdlist(t->cmdlist);
    	endfor
    	foreach (Trcmd, t, eachinst)
	    evalcmdlist(t->cmdlist);
    	endfor
    }
    bpact2(false,bpfound);
}

/*
 * A procedure call/return has occurred while single-stepping,
 * note it if we're tracing lines.
 */

private Boolean chklist();

public callnews(iscall)
Boolean iscall;
{
    if (not chklist(eachline, iscall)) {
	chklist(eachinst, iscall);
    }
}

private Boolean chklist(list, iscall)
List list;
Boolean iscall;
{
    register Trcmd t;
    register Command cmd;

    setcurfunc(whatblock(pc, true));
    foreach (Trcmd, t, list)
	foreach (Command, cmd, t->cmdlist)
	    if (cmd->op == O_PRINTSRCPOS and
	      (cmd->value.arg[0] == nil or cmd->value.arg[0]->op == O_QLINE)) {
		if (iscall) {
		    printentry(curfunc);
		} else {
		    printexit(curfunc);
		}
		return true;
	    }
	endfor
    endfor
    return false;
}

/*
 * When tracing variables we keep a copy of their most recent value
 * and compare it to the current one each time a breakpoint occurs.
 * MAXTRSIZE is the maximum size variable we allow.
 * changed this restriction Feb 8 1988 DT, now you can use memory
 * to heart's delight until you run out of stack
 */

/*#define MAXTRSIZE 512*/

/*
 * List of variables being watched.
 */

typedef struct Trinfo *Trinfo;

struct Trinfo {
    Node variable;
    Symbol trblock;
    char *trvalue;
};

private List trinfolist;

/*
 * Find the trace information record associated with the given record.
 * If there isn't one then create it and add it to the list.
 */

private Trinfo findtrinfo(p)
Node p;
{
    register Trinfo tp;
    Boolean isnew;

    isnew = true;
    if (trinfolist == nil) {
	trinfolist = list_alloc();
    } else {
	foreach (Trinfo, tp, trinfolist)
	    if (tp->variable == p) {
		isnew = false;
		break;
	    }
	endfor
    }
    if (isnew) {
	if (tracebpts) {
	    printf("adding trinfo for \"");
	    prtree(p);
	    printf("\"\n");
	}
	tp = new(Trinfo);
	if (tp == 0)
		fatal("No memory available. Out of swap space?");
	tp->variable = p;
	tp->trvalue = nil;
	list_append_tail(tp, trinfolist);
    }
    return tp;
}

/*
 * See if the value of a variable if it has changed since the
 * last time we checked.  If "printing" is true, print the value;
 * otherwise stop (e.g. "stop var").
 * modified Feb 8 1988 to remove MAXTRSIZE implementation restriction
 * DT
 */

public seeifchanged(p, printing)
Node p;
Boolean printing;
{
    register Trinfo tp;
    register int n;
/*    char buff[MAXTRSIZE]; DT Feb 8 1988 removed */
    char *buff;
    static Lineno prevline;
    register int i, *pbuff;
    int regno;
    Symbol s;

    tp = findtrinfo(p);
    s = p->nodetype;
    n = size(s);
    /* variable sized buffer DT Feb 8 1988 */
    buff = newarr(char, n);
    if (buff == 0)
	fatal("No memory available. Out of swap space?");
    if (p->op == O_SYM and isreg(p->value.sym)) {
	regno = p->value.sym->symvalue.offset;
#ifdef mc68000
	if (is68881reg(regno)) {
	    findregvar_68881(p->value.sym, nil, (double *) &buff[0]);
	} else if (isfpareg(regno)) {
	    findregvar_fpa(p->value.sym, nil, (double *) &buff[0]);
	} else
#elif i386
#ifdef WEITEK_1167
	if (isfpareg(regno)) {
	    findregvar_fpa(p->value.sym, nil, (double *) &buff[0]);
	} else
#endif WEITEK_1167
#endif mc68000
	{
	    pbuff = (int *)&buff[0];
	    i = findregvar(p->value.sym, nil);
	    if (n == sizeof(char)) {
	        *(char *)pbuff = i;
	    } else if (n == sizeof(short)) {
	        *(short *)pbuff = i;
	    } else {
	        *pbuff = i;
	    }
	}
    } else {
    	dread(buff, lval(p), n);
	if (read_err_flag) {
	    printf("can't read traced var \"");
	    prtree(p);
	    printf("\" - bad address 0x%x\n", lval(p));
	    if (not printing) {
		isstopped = true;
	    }
	    return;
	}
    }
    if (isbitfield(s)) {
	pbuff = (int *)&buff[0];
	if (n == 1) {
	    push(char, *(char *)pbuff);	/* push raw byte(s) on the stack */
	    push_aligned_bitfield(s);	/* extract bitfield */
	    *(char *)pbuff = pop(char);	/* put back in buff */
	} else if (n == 2) {
	    push(short, *(short *)pbuff);
	    push_aligned_bitfield(s);
	    *(short *)pbuff = pop(short);
	} else {
	    push(long, *pbuff);
	    push_aligned_bitfield(s);
	    *pbuff = pop(long);
	}
    }
    if (tp->trvalue == nil) {
	tp->trvalue = newarr(char, n);
	if (tp->trvalue == 0)
		fatal("No memory available. Out of swap space?");
	mov(buff, tp->trvalue, n);
	if (printing) {
	    mov(buff, sp, n);
	    sp += n;
	    push_double(rtype(s), n);
	    printf("initially (at line %d):\t", brkline);
	    prtree(p);
	    printf(" = ");
	    printval(s);
	    printf("\n");
	} else {
	    isstopped = true;
	}
    } else if (cmp(tp->trvalue, buff, n) != 0) {
	mov(buff, tp->trvalue, n);
	if (printing) {
	    mov(buff, sp, n);
	    sp += n;
	    push_double(rtype(s), n);
	    printf("after line %d:\t", prevline);
	    prtree(p);
	    printf(" = ");
	    printval(s);
	    printf("\n");
	} else {
	    isstopped = true;
	}
    }
    prevline = brkline;
    /* DT Feb 8 1988 - get rid of temporary buffer */
    dispose(buff);
}

/*
 * Free the tracing table.
 */

public trfree()
{
    register Trinfo tp;

    foreach (Trinfo, tp, trinfolist)
	dispose(tp->trvalue);
	dispose(tp);
	list_delete(list_curitem(trinfolist), trinfolist);
    endfor
}

/*
 * Fix up breakpoint information before continuing execution.
 *
 * It's necessary to destroy events and breakpoints that were created
 * temporarily and still exist because the program terminated abnormally.
 */

public fixbps()
{
    register Event e;
    register Trcmd t;

    single_stepping = false;
    inst_tracing = false;
    trfree();
    foreach (Event, e, eventlist)
	if (e->temporary) {
	    delevent(e->id, false);
	}
    endfor
    foreach (Trcmd, t, eachline)
	printrmtr(t);
	list_delete(list_curitem(eachline), eachline);
    endfor
    foreach (Trcmd, t, eachinst)
	printrmtr(t);
	list_delete(list_curitem(eachinst), eachinst);
    endfor
}

/*
 * Set all breakpoints in object code.
 */

public setallbps()
{
    register Breakpoint p;

    if (dirty_exit) {
	dirty_exit = 0;
	resetanybps();
    }
    foreach (Breakpoint, p, bplist)
	setbp(p->bpaddr);
    endfor
}

/*
 * Undo damage done by "setallbps".
 */

public unsetallbps()
{
    register Breakpoint p;

    foreach (Breakpoint, p, bplist)
	unsetbp(p->bpaddr);
    endfor
}

/*
 * A break point as been deleted.
 * If it corresponds to a "stop in" or "stop at"
 * then tell dbxtool about it.
 */
private event_delete(cond)
Node	cond;
{
    Node place;
    Symbol sym;
    Symbol func;
    String bpfile;
    Address addr;
    Integer lineno;

    if (cond->op == O_EQ and cond->value.arg[0]->op == O_SYM) {
	sym = cond->value.arg[0]->value.sym;
	place = cond->value.arg[1];
	if (sym == linesym) {
	    if (place->op == O_QLINE) {
		bpfile = place->value.arg[0]->value.scon;
		lineno = place->value.arg[1]->value.lcon;
		addr = objaddr(lineno, bpfile, true);
		lineno = srcline(addr);
		dbx_breakpoint(bpfile, lineno, false);
	    }
	} else if (sym == procsym) {
	    if (place->value.sym != program) {
		func = place->value.sym;
		addr = firstline(func);
		if (addr == NOADDR) {
			addr = codeloc(func);
		}
		bpfile = srcfilename(addr);
		lineno = linelookup(addr);
		dbx_breakpoint(bpfile, lineno, false);
	    }
	}
    }
}

/*
 * Clear all the breakpoints at given line numbers
 * If bp was "stopi" and we stopped there and user just
 * said "clear", then we have to clear the bp that is at the "PC".
 * Same if we did, e.g., "stop in printf" and printf has no -g.
 */
bp_clear(p)
Node p;
{
    Event e;
    Symbol sym;
    Symbol func;
    Node cond;
    Node place;
    Lineno lineno;
    Address addr, nog_addr;
    char *bpfile;

    nog_addr = 0;
    if( p->op == O_QLINE ) {
	bpfile = p->value.arg[0]->value.scon;
	lineno = p->value.arg[1]->value.lcon;

	if(  offset_findline != 0  or
	     ( lineno == 0  and (bpfile == nil or *bpfile == 0) )
	  ) {
	    /* clear bp "here", where here had no -g */
	    nog_addr = pc;
	}
    }

    if( nog_addr == 0 )
	next_executable(p);

    foreach (Event, e, eventlist)
	    cond = e->condition;
	    if (cond->op == O_EQ and cond->value.arg[0]->op == O_SYM) {
		place = cond->value.arg[1];
		sym = cond->value.arg[0]->value.sym;
		if (sym == linesym) {
		    if (place->op == O_QLINE) {
		        bpfile = place->value.arg[0]->value.scon;
		 	lineno = place->value.arg[1]->value.lcon;
			if (same_file(bpfile, p->value.arg[0]->value.scon) and
			    lineno == p->value.arg[1]->value.lcon) {
			    delevent(e->id, false);
			}
		    }
		} else if (sym == procsym) {
		    if (place->value.sym != program) {
			func = place->value.sym;
			addr = firstline(func);
			if (addr == NOADDR) {
				addr = codeloc(func);
			}
			lineno = linelookup(addr);
			if (lineno == p->value.arg[1]->value.lcon) {
			    delevent(e->id, false);
			}
		    }
		} else if (sym == pcsym) {
		    if( nog_addr == (Address) place->value.arg[0] ) {
			delevent(e->id, false);
		    }
		}
	    }
    endfor
}



/*
 * dumpeventlist -- just like printevent, but print out an internal
 * form.  For dbx-debugging only -- called from dbxdebug
 */
public  dumpeventlist ( matchid, verbose )
int matchid;
Boolean verbose;
{
    Event e;

    foreach (Event, e, eventlist)

	printf( "(Event id %d) ", e->id);
	printf( "\ttemporary = %s\n", e->temporary ? "true" : "false" );
	printf( "\t\tcondition = 0x%x\n", e->condition );
	printf( "\t\tactions = 0x%x\n", e->actions );

	if (not e->temporary) {
	    printevent( e );
	    dumpevent( e, matchid, verbose );
	}
    endfor
}


/*
 * Print an event in a form that is nothing like what the user typed in.
 */
private  dumpevent(e, matchid, verbose )
Event e;
int matchid;
Boolean verbose;
{
    Command cmd;

    if( matchid  and  matchid != e->id ) return;

    dumptree( e->condition );
    printf("\n");
}
