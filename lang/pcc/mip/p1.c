#ifndef lint
static	char sccsid[] = "@(#)p1.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * this file contains code that is common to all Sun C compilers,
 * but not common to other programs that live parasitically off
 * the C grammar, scanner, and symbol table.  Most of it used to
 * be maintained separately in three separate versions of local.c.
 */

#include "cpass1.h"
#include "pragma.h"

extern int twopass;
static OFFSZ p1tmpoff = 0;	/* # storage units allocated to temporaries */
extern void module_ident();	/* emits assembler -- in local.c */

#ifdef FLOATMATH
/*
 * convert float value parameters to double.
 * This has to be done before code generation time,
 * if C is to use the same code generator as FORTRAN
 * and Pascal.
 */

NODE *
floatargs_to_double(p)
	register NODE *p;
{
	if(p->in.op == CM) {
		p->in.right = floatargs_to_double(p->in.right);
		p->in.left = floatargs_to_double(p->in.left);
		return p;
	}
	/* assume (FLOATMATH <= 1) */
	if (p->in.type == FLOAT) {
		return makety(p, DOUBLE, 0, DOUBLE);
	}
	return p;
}
#endif FLOATMATH

/*
 * allocate automatic storage and create a tree for a temporary
 * copy of p.  This is done by generating a symbol and entering
 * it in the symbol table with storage class AUTO.  clocal() then
 * decides whether to turn it into a tree of addressing primitives
 * or leave the node as a NAME (which is what iropt prefers)
 */
NODE *
p1tmpalloc(p)
    register NODE *p;
{
    OFFSZ off;
    static tmpno;
    char name[30];

    p = block(NAME, NIL, NIL, p->in.type, p->fn.cdim, p->fn.csiz);
    sprintf(name, "#tmp%d", tmpno++);
    p->tn.rval = lookup(hash(name),0);
    off = autooff;
    defid(p, AUTO);
    SETOFF( autooff, SZINT );
    p1tmpoff += (autooff - off);
    bccode();	 /* sync with backend */
    return clocal(p);
}

/*
 * reclaim auto storage allocated to temporaries;
 * this is called after the end of the current
 * statement (see ecomp, trees.c).
 *
 * WARNING: don't do this if you're going to run iropt.
 * Your temps will get mixed up with local variables in
 * nested blocks, and unpleasant things will happen.
 */
void
p1tmpfree()
{
    if (p1tmpoff && !twopass) {
	autooff -= p1tmpoff;
	bccode();	/* sync with back end */
	p1tmpoff = 0;
    }
}

/*
 * The following atrocity is called from tymatch() when it has been
 * painted into the following corner: an expression of the form
 * x <op>= y, where one of the following is true:
 *	1. <op> = "/" or "%" and y is of a larger type than x
 *	2. y has a floating type, and x does not.
 *	3. x has type 'float' and y has a type whose values cannot always
 *	   be represented exactly in float.
 * In these cases, it is incorrect to truncate y to the type of x.
 * If x is sufficiently nice, rewrite as
 *	x = x op y
 * and do the usual arithmetic conversions; otherwise rewrite as
 *	(temp = &x, *temp = *temp op y)
 * This does a lot of crap that should be done in the back end,
 * except that by then the usual idiotic arithmetic conversions
 * will have botched the job irrecoverably.
 */
NODE *
rewrite_asgop(p)
    register NODE *p;
{
    register NODE *l, *tmp, *q;
    int simple;

    l = p->in.left;
    if (l->in.op == FLD) {
	q = l;
	l = l->in.left;
    }
    if (l->in.op == UNARY MUL) {
	q = l;
	l = l->in.left;
    }
    if (l->in.op == PLUS && nncon(l->in.right) && l->in.left->in.op == REG) {
	simple = 1;
    } else {
	simple = (optype(l->in.op) == LTYPE);
    }
    if (simple) {
	/*
	 * easy case: rewrite as x = x op y
	 */
	p->in.op = NOASG p->in.op;
	l = pcc_tcopy(p->in.left);
	p = tymatch(p);
	p = makety(p, l->in.type, l->fn.cdim, l->fn.csiz);
	return block(ASSIGN, l, p, p->in.type, p->fn.cdim, p->fn.csiz);
    } else {
	/*
	 * ugly case: rewrite, more or less, as
	 * (tmp = &x, *tmp = *tmp op y)
	 * but remember there might be a FLD in there someplace.
	 */
	tmp = p1tmpalloc(l);
	tmp = block(ASSIGN, tmp, l, l->in.type, l->fn.cdim, l->fn.csiz);
	q->in.left = pcc_tcopy(tmp->in.left);
	p->in.op = NOASG p->in.op;
	l = pcc_tcopy(p->in.left);
	p = tymatch(p);
	p = makety(p, l->in.type, l->fn.cdim, l->fn.csiz);
	p = block(ASSIGN, l, p, p->in.type, p->fn.cdim, p->fn.csiz);
	return block(COMOP, tmp, p, p->in.type, p->fn.cdim, p->fn.csiz);
    } /* 1e+308 on the vomit meter */
}

/*
 * do_pragma(pragma_id, args)
 *	Process a #pragma directive.  pragma_id is a value returned
 *	by lookup_pragma() (below).  args is a tree of the same form
 *	as a function argument list, with the restriction that only
 *	names and integer constant arguments are permitted.  Names
 *	may be in a separate name space from program identifiers,
 *	and thus may be used as pragma keywords.
 *
 * -- #pragma unknown_control_flow(name {, name })
 *
 *	Specifies a list of routines that violate the usual control
 *	flow properties of procedure calls.  For example, the statement
 *	following a call to setjmp() can be reached from an arbitrary call
 *	to any other routine.  The statement is reached by a call to longjmp().
 *	Since such routines render standard flowgraph analysis invalid,
 *	routines that call them cannot be safely optimized; hence, they
 *	are compiled with the optimizer disabled.
 *
 * -- #pragma makes_regs_inconsistent(name {, name })
 *
 *	Specifies a list of routines with the unpleasant habit of
 *	making their caller's register variables inconsistent with
 *	the values they would have in automatic storage (hence, a
 *	strong hint to the optimizer not to put variables in registers)
 *	The most commonly cited example is setjmp() again, although
 *	others are rumored to exist in the unix kernel.
 */
static void mark_unsafe_calls();

void
do_pragma(pragma_id, args)
	int pragma_id;
	NODE *args;
{
	NODE *p,*q;
	struct symtab *sym;

	switch(pragma_id) {
	case PR_MAKES_REGS_INCONSISTENT:
		mark_unsafe_calls(args, S_MAKES_REGS_INCONSISTENT);
		break;
	case PR_UNKNOWN_CONTROL_FLOW:
		mark_unsafe_calls(args, S_UNKNOWN_CONTROL_FLOW);
		break;
	case PR_IDENT:
		module_ident(args);
		break;
	default:
		break;
	}
	pcc_tfree(args);
}

/*
 * traverse arg list, and verify that each node is a declared
 * function name.  Mark each function as having an unpleasant
 * property.
 */
static void
mark_unsafe_calls(p, sflag)
	NODE *p;
	int sflag;
{
	struct symtab *sym;

	while(p != NULL && p->in.op == CM ) {
		mark_unsafe_calls(p->in.right, sflag);
		p = p->in.left;
	}
	if (p != NULL && p->in.op == NAME) {
		sym = STP(p->tn.rval);
		if (sym != NULL && ISFTN(sym->stype)) {
			sym->sflags |= sflag;
			return;
		}
	}
	werror("function name expected");
}

struct pragma {
	char *name;
	int key;
};

struct pragma pragma_table [] = {
	"unknown_control_flow",		PR_UNKNOWN_CONTROL_FLOW,
	"makes_regs_inconsistent",	PR_MAKES_REGS_INCONSISTENT,
	"ident",			PR_IDENT,
	NULL
};

int
lookup_pragma(name)
	char *name;
{
	struct pragma *p;

	if (name != NULL) {
		for (p = pragma_table; p->name; p++) {
			if (!strcmp(p->name, name)) {
				return p->key;
			}
		}
		werror("pragma %s not recognized", name);
	}
	return 0;
}

static p1builtin_must_init = 1;

static void
p1builtin_init()
{
	int i;
	for (i = 0; p1builtin_funcs[i].name != NULL; i++) {
		p1builtin_funcs[i].name = hash(p1builtin_funcs[i].name);
	}
}

/*
 * rewrite builtin functions.  This looks much like a similar facility
 * in pass 2 of the SPARC compiler, but is used for situations requiring
 * information not available in pass 2.
 */

NODE *
p1builtin_call(p)
	NODE *p;		/* CALL, STCALL, UNARY CALL, UNARY STCALL */
{
	struct symtab *funcid;	/* indentifier of called function */
	NODE *lp;		/* for tree probing */
	int i;			/* to scan list of builtins */

	if (callop(p->in.op) && p->in.left != NULL) {
		/*
		 * have call, probe for function identifier
		 */
		lp = p->in.left;
		funcid = NULL;
		if (lp->in.op == ICON) {
			funcid = STP(lp->tn.rval);
		} else if (lp->in.op == UNARY AND
		    && lp->in.left != NULL
		    && lp->in.left->in.op == NAME) {
			lp = lp->in.left;
			funcid = STP(lp->tn.rval);
		}
		if (funcid != NULL) {
			if (p1builtin_must_init) {
				p1builtin_init();
				p1builtin_must_init = 0;
			}
			for (i = 0; p1builtin_funcs[i].name != NULL; i++) {
				if (p1builtin_funcs[i].name == funcid->sname) {
					return (*p1builtin_funcs[i].rewrite)(p);
				}
			}
		}
	}
	return(p);
}
