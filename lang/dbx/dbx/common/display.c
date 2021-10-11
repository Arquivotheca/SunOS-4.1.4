#ifndef lint
static	char sccsid[] = "@(#)display.c 1.1 94/10/31 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "defs.h"
#include "tree.h"
#include "machine.h"
#include "dbxmain.h"
#include "display.h"
#include "process.h"

typedef	struct	Display	*Display;
struct	Display	{
	Node	expr;			/* Expression */
	Display	next;			/* Linked list */
	char	*value;			/* Typeless buffer holding old value */
	int	number;			/* Unique number for this expr */
};

private Display	disphdr;		/* Display list header */
private Display	*bpatch = &disphdr;	/* Back patch to add next item */
private int	disp_num;		/* Current number to assign */
private char	*display_tmp = "/tmp/dbx.display.XXXXXX";
					/* Temp file to hold display results */

private Node	copy_tree();
private Boolean	cmp_tree();

#ifndef public
#define	copy_jmpbuf(from, to)	bcopy(from, to, sizeof(jmp_buf))
#endif

/*
 * Add a list of items to the display list.
 * The contents of the items in this list will be displayed at each breakpoint.
 */
public	display(n)
Node	n;
{
	Display	dp;
	Node	n1;
	Node	start;

	start = n->value.arg[0];
	if (start == nil) {
		pr_display_list();
	} else {
		for (n1 = start; n1 != nil; n1 = n1->value.arg[1]) {
			disp_num++;
			dp = new(Display);
			if (dp == 0)
				fatal("No memory available. Out of swap space?");
			dp->expr = copy_tree(n1->value.arg[0]);
			dp->number = disp_num;
			*bpatch = dp;
			bpatch = &dp->next;
		}
		pr_display(false);
	}
}

/*
 * Print the list of variables being displayed at each breakpoint.
 */
public	pr_display_list()
{
	Display dp;

	for (dp = disphdr; dp != nil; dp = dp->next) {
		if (isredirected()) {
			printf("display ");
		} else {
			printf("(%d) ", dp->number);
		}
		prtree(dp->expr);
		printf("\n");
	}
}

/*
 * Print the contents of the expressions in the display list.
 * It is possible to run into errors in printing this values.
 * When errors are found, a longjmp() is done to go back to
 * command parsing.  This messes us up here.  Therefore,
 * the longjmp buffer is saved and setjmp() is called to
 * cause the longjump() to come back here.  When we are done,
 * the longjmp buffer is restored.  
 *
 * I hate to admit that this is nested setjmp/longjmp's.
 *
 * If the 'doitanyway' flag is true, then give the backing file to 
 * the dbxtool even if it is empty.  This is for undisplay when
 * the last item is undisplayed.
 */
public	pr_display(doitanyway)
Boolean	doitanyway;
{
	Display dp;
	jmp_buf	save_env;

	if ((disphdr == nil and not doitanyway) or process == nil) {
		return;
	}
	copy_jmpbuf(env, save_env);
	dbx_start_display();
	for (dp = disphdr; dp != nil; dp = dp->next) {
		prtree(dp->expr);
		printf(" = ");
		if (setjmp(env) != 1) {
			eval(copy_tree(dp->expr));
			printval(dp->expr->nodetype);
			printf("\n");
		}
	}
	dbx_done_display();
	dbx_display(display_tmp);
	copy_jmpbuf(save_env, env);
}

/*
 * For dbxtool, direct all output to a tmp file.
 * Do nothing for tty oriented dbx.
 */
private	dbx_start_display()
{
	private	Boolean	first = true;

	if (not istool()) {
		return;
	}
	if (first) {
		mktemp(display_tmp);
		first = false;
	} else {
		unlink(display_tmp);
	}
	setout(display_tmp);
}

/*
 * For dbxtool, restore all output to stdout.
 * Do nothing for tty oriented dbx.
 */
private	dbx_done_display()
{
	if (not istool()) {
		return;
	}
	unsetout();
}

/*
 * Copy a tree.
 */
private	Node	copy_tree(p)
Node	p;
{
	Node	n;
	Integer	i;

	if (p == nil) {
		return(nil);
	}
	n = new(Node);
	if (n == 0)
		fatal("No memory available. Out of swap space?");
	*n = *p;
	for (i = 0; i < nargs(p->op); i++) {
		n->value.arg[i] = copy_tree(p->value.arg[i]);	
	}
	return(n);
}

/*
 * Remove an item from the display list.
 */
public	undisplay(n)
Node	n;
{
	Display	dp;
	Display *bp;
	Node	n1;
	Node	start;
	Node	expr;

	start = n->value.arg[0];
	for (n1 = start; n1 != nil; n1 = n1->value.arg[1]) {
		expr = n1->value.arg[0];
		bp = &disphdr;
		for (dp = disphdr; dp != nil; dp = dp->next) {
			if ((expr->op == O_LCON and
			    dp->number == expr->value.lcon) or
			    cmp_tree(dp->expr, expr)) {
				if (bpatch == &dp->next) {
					bpatch = bp;
				}
				*bp = dp->next;
				tfree(dp->expr);
				dispose(dp);
				break;
			}
			bp = &dp->next;
		}
	}
	pr_display(true);
}

/*
 * Compare two trees to see if they are equal.
 */
private Boolean	cmp_tree(p, q)
Node	p;
Node	q;
{
	int	i;

	if (p == q) {
		return(true);
	}
	if (p->op != q->op or p->nodetype->class != q->nodetype->class) {
		return(false);
	}
	if(nargs(p->op) == -1) {
            switch(p->op) {
		case O_NAME:
		    if(strcmp(idnt(p->value.name),
			      idnt(q->value.name)) != 0)
			return (false);
		    break;
		case O_SYM:
		    if(strcmp(symname(p->value.sym),
			      symname(q->value.sym)) != 0)
			return (false);
		    break;
		case O_LCON:
		    if(p->value.lcon != q->value.lcon)
			return (false);
		    break;
		case O_FCON:
		    if(p->value.fcon != q->value.fcon)
			return (false);
		    break;
		case O_SCON:
		    if(strcmp(p->value.scon,q->value.scon) != 0)
			return (false);
		    break;
	    }
	    return (true);
	} else for (i = 0; i < nargs(p->op); i++) {
		if (not cmp_tree(p->value.arg[i], q->value.arg[i])) {
			return(false);
		}
	}
	return(true);
}

