#ifndef lint
static	char sccsid[] = "@(#)check.c 1.1 94/10/31 SMI"; /* from UCB X.X XX/XX/XX */
#endif

/*
 * Check a tree for semantic correctness.
 */

#include "defs.h"
#include "tree.h"
#include "operators.h"
#include "events.h"
#include "symbols.h"
#include "scanner.h"
#include "source.h"
#include "object.h"
#include "mappings.h"
#include "process.h"
#include "eval.h"

#define isinteger(sym) ((sym)->class == RANGE and \
		((sym)->symvalue.rangev.lower <= 0))

public Boolean is_node_addr();

/*
 * Check that the nodes in a tree have the correct arguments
 * in order to be evaluated.  Basically the error checking here
 * frees the evaluation routines from worrying about anything
 * except dynamic errors, e.g. subscript out of range.
 */

public check(p)
register Node p;
{
    Address addr;
    Symbol f;
    Name n;

    checkref(p);
    switch (p->op) {
	case O_LIST:
	    if (p->value.arg[0]->op == O_SYM) {
		f = p->value.arg[0]->value.sym;
		if (not isblock(f) or ismodule(f)) {
		    error("\"%s\" is not a procedure or function", symname(f));
		}
		addr = firstline(f);
		if (addr == NOADDR) {
		    error("\"%s\" is empty", symname(f));
		}
	    }
	    break;

	case O_CALL:
	    f = p->value.arg[0]->nodetype;
	    if (f->class != FUNC and f->class != PROC and
		rtype(f)->class != FUNCT) {
		f = nil;
		if (p->value.arg[0]->op == O_SYM) {
		    n = p->value.arg[0]->value.sym->name;
		    find (f, n) where f->class == FUNC || f->class == PROC
		    endfind(f);
		}
		if (f != nil) {
		    p->value.arg[0] = build(O_SYM, f);
		} else {
		    printf("`");
		    prtree(p->value.arg[0]);
		    fflush(stdout);
		    error("' is not a function or procedure");
		}
	    }
	    break;

	case O_TRACE:
	case O_TRACEI:
	    chktrace(p);
	    break;

	case O_STOP:
	case O_STOPI:
	    chkstop(p);
	    break;

	/*
	 * Operands to ADD and SUB must be integers, floats, or pointers.
	 */
	case O_ADD:
	case O_SUB: {
	    Symbol t1;
	    Symbol t2;

	    t1 = rtype(p->value.arg[0]->nodetype);
	    t2 = rtype(p->value.arg[1]->nodetype);
	    if (not isinteger(t1) and not isfloat(t1) and t1->class != PTR) {
		error("Inappropriate left hand operand to `%s'",
		    opname(p->op));
	    }
	    if (not isinteger(t2) and not isfloat(t2) and t2->class != PTR) {
		error("Inappropriate right hand operand to `%s'",
		    opname(p->op));
	    }
	    break;
	}

	/*
	 * Operands to MUL and DIV must be integers or floats.
	 */
	case O_MUL:
	case O_DIV: {
	    Symbol t1;
	    Symbol t2;

	    t1 = rtype(p->value.arg[0]->nodetype);
	    t2 = rtype(p->value.arg[1]->nodetype);
	    if (not isinteger(t1) and not isfloat(t1)) {
		error("Inappropriate left hand operand to `%s'",
		    opname(p->op));
	    }
	    if (not isinteger(t2) and not isfloat(t2)) {
		error("Inappropriate right hand operand to `%s'",
		    opname(p->op));
	    }
	    break;
	}

  	/*
	 * Both operands to these operators must be integers.
	 */
	case O_MOD:
	case O_BITAND:
	case O_BITOR:
	case O_BITXOR:
	case O_RSHIFT:
	/* D Teeple March 29 1988 added a few cases to fix bug 1002617 */
	/* D Teeple April 8 1988 fix for stop in to work */
	case O_LT:
	case O_LE:
	case O_GT:
	case O_GE:
	case O_EQ:
	case O_NE:
	case O_LSHIFT: {
	    Symbol t1;
	    Symbol t2;

	    t1 = rtype(p->value.arg[0]->nodetype);
	    t2 = rtype(p->value.arg[1]->nodetype);
	    if (not (isinteger(t1) or t1->class == PTR or isfloat(t1) or
	        t1->class == FUNC or t1->class == PROC)) {
		error("Left hand operand to `%s' must be integer, float or pointer",
		    opname(p->op));
	    }
	    if (not (isinteger(t2) or t2->class == PTR or isfloat(t2) or
	        t2->class == FUNC or t2->class == PROC or t2->class == PROG)) {
		error("Right hand operand to `%s' must be integer, float or pointer",
		    opname(p->op));
	    }
	    if (size(p->value.arg[0]->nodetype) > sizeof(double)) {
		error("Left operand to `%s' is too large",
		    opname(p->op));
	    }
 	    if (size(p->value.arg[1]->nodetype) > sizeof(double)) {
		error("Right Operand to `%s' is too large",
		    opname(p->op));
	    }

	    break;
	}
	/* D Teeple March 29 1988 fix bug 1002617 
	 * operands must be real
	 */
 	case O_LTF:
 	case O_LEF:
 	case O_GTF:
 	case O_GEF:
 	case O_EQF:
 	case O_NEF: {
	    Symbol t1;
	    Symbol t2;

	    t1 = rtype(p->value.arg[0]->nodetype);
	    t2 = rtype(p->value.arg[1]->nodetype);
	    if (not isfloat(t1)) {
		error("Inappropriate left hand operand to `%s'",
		    opname(p->op));
	    }
	    if (not isfloat(t2)) {
		error("Inappropriate right hand operand to `%s'",
		    opname(p->op));
	    }
	    if (size(p->value.arg[0]->nodetype) > sizeof(double) or
	        size(p->value.arg[1]->nodetype) > sizeof(double)) {
		error("Invalid operand to `%s'",
		    opname(p->op));
	    }
 	    break;
	}

	/* end of fix D Teeple March 29 1988 bug 1002617 */

  	/*
	 * The operand to these operators must be integer.
	 */
	case O_COMPL:
	case O_NOT: {
	    Symbol t1;

	    t1 = rtype(p->value.arg[0]->nodetype);
	    if (not isinteger(t1)) {
		error("Operand to `%s' must be integer", opname(p->op));
	    }
	    break;
	}

	case O_DBXENV:
	    check_dbxenv(p);

	default:
	    break;
    }
}

/*
 * Check arguments to a trace command.
 */

private chktrace(p)
Node p;
{
    Node exp, place;

    exp = p->value.arg[0];
    place = p->value.arg[1];
    if (exp == nil && p->op == O_TRACE) {
	chkblock(place);
    } else if (exp == nil && p->op == O_TRACEI) {
	if (place == nil) {
	    error("missing address to tracei");
	}
	if (place->op == O_SYM)
	    chkblock(place);
	else
	    chkaddr(place);
    } else if (exp->op == O_LCON or exp->op == O_QLINE) {
	if (place != nil) {
	    error("unexpected \"at\" or \"in\"");
	}
	if (p->op == O_TRACE) {
	    chkline(exp);
	} else {
	    chkaddr(exp);
	}
    } else if (place != nil and (place->op == O_QLINE or place->op == O_LCON)) {
	if (p->op == O_TRACE) {
	    chkline(place);
	} else {
	    chkaddr(place);
	}
    } else {
	if (exp->op != O_RVAL and exp->op != O_SYM and exp->op != O_CALL) {
	    error("can't trace expressions");
	}
	if (is_node_addr(place)) {
	    chkaddr(place);
	}
	else {
	    chkblock(place);
	}
    }
}

/*
 * Check arguments to a stop command.
 */

private chkstop(p)
Node p;
{
    Node exp, place, cond;

    exp = p->value.arg[0];
    place = p->value.arg[1];
    cond = p->value.arg[2];
    if (exp != nil) {
 	if (exp->op != O_RVAL and exp->op != O_SYM and exp->op != O_LCON) {
	    beginerrmsg();
	    printf("expected variable, found ");
	    prtree(exp);
	    enderrmsg();
	}
	chkblock(place);
    } else if (cond != nil and (place == nil or place->op == O_SYM)) {
	chkblock(place);
    } else if (place->op == O_SYM) {
	chkblock(place);
    } else {
	if (p->op == O_STOP) {
	    chkline(place);
	} else {
	    chkaddr(place);
	}
    }
}

/*
 * Check to see that the given node specifies some subprogram.
 * Nil is ok since that means the entire program.
 * If it is a function variable, this too is ok since it will be
 * handled in the routines called by trace() and stop().
 */

private chkblock(b)
Node b;
{
    if (b != nil) {
	if (b->op != O_SYM) {
	    beginerrmsg();
	    printf("expected subprogram, found ");
	    prtree(b);
	    enderrmsg();
	} else if ((not (isblock(b->value.sym) or ismodule(b->value.sym))) and
	  (b->value.sym->class != FVAR)) {
	    error("\"%s\" is not a subprogram", symname(b->value.sym));
	}
    }
}

/*
 * Check to make sure a node corresponds to a source line.
 */

private chkline(p)
Node p;
{
    if (p == nil) {
	error("missing line");
    } else if (p->op != O_QLINE and p->op != O_LCON) {
	error("expected source line number, found \"%t\"", p);
    }
}


/* use this to determine if a node is an address expression; typically
 * this means that the production address was used in commands.y.
 * and you want to allow a subset of the expressions possible.
 */
public Boolean is_node_addr(p)
Node p;
{
    if (p == nil) 
	return false;
    switch (p->op) {
	case O_LCON:
	case O_ADD:
	case O_AMPER:
	case O_SUB:
	case O_MUL:
		return true;
	break;
    }
    return false;
}


/*
 * Check to make sure a node corresponds to an address.
 */

private chkaddr(p)
Node p;
{
    if (p == nil) {
	error("missing address");
    } else {
	if (!is_node_addr(p)) {
	    beginerrmsg();
	    printf("expected address, found \"");
	    prtree(p);
	    printf("\"");
	    enderrmsg();
	}
    }
}
