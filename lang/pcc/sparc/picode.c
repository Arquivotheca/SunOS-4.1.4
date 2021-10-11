#ifndef lint
static	char sccsid[] = "@(#)picode.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "cpass2.h"

/*
 * given the components of a relocatable expression (<symbol>+<offset>),
 * return an equivalent expression, with the symbol replaced by
 * an indirect memory reference *(<basereg>+<symbol>), where <basereg>
 * is the 'pic' base register.
 */

static NODE *
base_plus_offset(name, offset, ptr_t)
	char *name;
	OFFSZ offset;
	TWORD ptr_t;
{
	NODE *q,*r;

	q = build_tn(REG, ptr_t, "", 0, BASEREG);
	r = build_tn(ICON, INT, name, 0, 0);
	q = build_in(PLUS, ptr_t, q, r);
	q = build_in(UNARY MUL, DECREF(ptr_t), q, NULL);
	if (offset != 0) {
		r = build_tn(ICON, INT, "", offset, 0);
		q = build_in(PLUS, DECREF(ptr_t), q, r);
	}
	return(q);
}

/*
 * Traverse an expression tree and rewrite leaf nodes (NAME, ICON, FCON) 
 * with relocatable symbol references, so that all such references
 * are replaced by indirect storage references through a global
 * linkage table.  BASEREG is a macro for a register reserved
 * for addressing this table.
 *
 * The point of all this is to make all nonlocal storage references
 * position-independent.  Note that direct procedure calls are already
 * self-relative, so we leave them alone.
 */
void
picode(p)
	NODE *p;
{
	int d;
	int o;
	TWORD t;
	NODE *q;
	char *lab;

	if((o = p->in.op) == INIT) {
		return;
	}
	switch (optype(o)) {
	case BITYPE:
		picode(p->in.right);
		if (pcc_callop(o) && p->in.left->in.op == ICON) {
			/* direct procedure call; leave the name alone */
			break;
		}
		picode(p->in.left);
		break;
	case UTYPE:
		if (pcc_callop(o) && p->in.left->in.op == ICON) {
			/* direct procedure call; leave the name alone */
			break;
		}
		picode(p->in.left);
		break;
	case LTYPE:
		t = p->in.type;
		switch (o) {
		case FCON:
			/*
			 * floating point constants are generated
			 * in data space, and must be treated the
			 * same as global names.
			 */
			d = fgetlab(p);
			lab = malloc(20);
			if (lab == NULL) {
				fatal("picode: out of memory");
				/*NOTREACHED*/
			}
			(void)sprintf(lab, "L%d", d);
			p->in.op = NAME;
			p->tn.name = lab;
			p->tn.lval = 0;
			p->tn.rval = 0;
			picode(p);
			break;
		case NAME:
			/*
			 * relocatable storage reference
			 */
			if (p->tn.name[0] != '\0') {
				q = base_plus_offset(p->tn.name, p->tn.lval,
					INCREF(INCREF(t)));
				p->in.op = UNARY MUL;
				p->in.left = q;
				p->in.right = NULL;
			}
			break;
		case ICON:
			/*
			 * relocatable address constant
			 */
			if (p->tn.name[0] != '\0') {
				q = base_plus_offset(p->tn.name, p->tn.lval,
					INCREF(t));
				*p = *q;
				q->in.op = FREE;
			}
			break;
		}
	}
}
