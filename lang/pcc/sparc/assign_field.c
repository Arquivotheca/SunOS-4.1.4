#ifndef lint
static char sccsid[] = "@(#)assign_field.c 1.1 94/10/31 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

#include "cpass2.h"

/* export */

void assign_field( /* p, cookie */ );

/* private */

static void isolate_src(/*p, cookie, maskop, inverted*/);
static int  mkmask(/*p, cookie, shmask, toreg*/);
static void assign_field_reg(/*p, cookie*/);
static void assign_field_const(/*p, cookie*/);

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * Generate code for a bit field assignment. p is an ASSIGN op with a FLD on
 * the left, and either a REG or some sort of constant on the right. 
 *
 * Special cases result from fields found to be suitably small and suitably
 * aligned to allow the use of immediate operands. All are variations on the
 * following: 
 *
 * Algorithm:
 *	assign(dst,src,mask,shmask,fldshf) {
 *	    dst = (dst & ~shmask) | ((src<<fldshf) & shmask);
 *	    [ return src&mask; ]
 *	} 
 */

void
assign_field(p, cookie)
    NODE *p;
{
    NODE *r;

    if (p->in.op != ASSIGN || p->in.left->in.op != FLD) {
	cerror("illegal expression in assign_field");
	/* NOTREACHED */
    }
    r = p->in.right;
    if (r->in.op == REG && isareg(r->tn.rval)) {
	assign_field_reg(p, cookie);
    } else if (r->in.op == ICON) {
	assign_field_const(p, cookie);
    } else {
	cerror("illegal source in assign_field");
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * generate code to isolate the field source value ("AR")
 * in resc[0] ("A1").  maskop represents the mask operand,
 * which is either immediate or in a register.
 */
static void
isolate_src(p, cookie, maskop, inverted)
    NODE *p;
    int cookie;
    char *maskop;	/* mask operand, either immediate or reg */
    int inverted;	/* =1 if mask is inverted */
{
    int left_field;	/* =1 if field is on left end of storage unit */
    int right_field;	/* =1 if field is on right end of storage unit */

    left_field = (UPKFOFF(p->in.left->tn.rval) == 0);
    right_field = (fldshf == 0);
    if (left_field) {
	if (right_field) {
	    /*
	     * ((src<<off)&srcmask) => (src)
	     * i.e., just copy it
	     */
	    expand(p, cookie, "	mov	AR,A1\n");
	} else {
	    /*
	     * ((src<<off)&srcmask) => (src<<off);
	     * i.e., no bits on the left to mask out
	     */
	    expand(p, cookie, "	sll	AR,H,A1\n");
	}
    } else {
	if (right_field) {
	    /*
	     * ((src<<off)&srcmask) => (src&srcmask)
	     * i.e., skip the shift.
	     */
	    if (inverted) {
		expand(p, cookie, "	andn	AR,");
	    } else {
		expand(p, cookie, "	and	AR,");
	    }
	} else {
	    /*
	     * ((src<<off)&srcmask)
	     * default case, no shortcuts.
	     */
	    expand(p, cookie, "	sll	AR,H,A1\n");
	    if (inverted) {
		expand(p, cookie, "	andn	A1,");
	    } else {
		expand(p, cookie, "	and	A1,");
	    }
	}
	expand(p, cookie, maskop);
	expand(p, cookie, ",A1\n");
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * generate code to build a mask in a register.
 * shmask is the mask value, and is known to be out of
 * range for an immediate operand. toreg represents the
 * target register.
 *
 * If it is possible to load the mask in a single sethi,
 * we'll do it, even if it means loading the mask's complement
 * instead of its true value.  If we do load the complement,
 * we return 1 so that others can act accordingly.
 */
static int
mkmask(p, cookie, shmask, toreg)
    NODE *p;
    int cookie;
    int shmask;
    char *toreg;
{
    int inverted;

    inverted = 0;
    if ((shmask & 0xfff) == 0) {
	expand(p, cookie, "	sethi	%hi(M),");
    } else {
	if (!(~shmask & 0xfff)) {
	    inverted = 1;
	    expand(p, cookie, "	sethi	%hi(N),");
	} else {
	    expand(p, cookie, "	set	M,");
	}
    }
    expand(p, cookie, toreg);
    expand(p, cookie, "\n");
    return inverted;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * Assign register to bit field.  We expect 3 temps, none of which
 * can coincide with the register named by p->in.right. If a result
 * is expected, it is always returned in resc[0]. 
 */
static void
assign_field_reg(p, cookie)
    NODE *p;
    int cookie;
{
    int mask;			/* bit field mask */
    int shmask;			/* mask << fldshf */
    int inverted;		/* =1 if complement of mask value was loaded */
    int i;			/* for iterating through resc[] */
    NODE *r;			/* rhs of assignment */

    /*
     * check temp register allocation. 
     */
    r = p->in.right;
    for (i = 0; i < 3; i++) {
	if (resc[i].in.op != REG
	    || !isareg(resc[i].tn.rval)
	    || resc[i].tn.rval == r->tn.rval) {
	    cerror("illegal register allocation in assign_field");
	}
    }
    mask =  bitmask(fldsz);		/* ZX in templates */
    shmask = lshift(mask, fldshf);	/* M in templates  */
    if (IN_IMMEDIATE_RANGE(shmask)) {
	/*
	 * small shifted field: can use immediate operands for both zeroing the
	 * target and isolating the source. 
	 */
	expand(p, cookie, "	ldZLB	AL,A2\n");
	isolate_src(p, cookie, "M", 0);
	expand(p, cookie, "	andn	A2,M,A2\n");
	expand(p, cookie, "	or	A2,A1,A2\n");
	expand(p, cookie, "	stZLb	A2,AL\n");
    } else {
	/*
	 * default case: must build mask separately. 
	 */
	expand(p, cookie, "	ldZLB	AL,A3\n");
	inverted = mkmask(p, cookie, shmask, "A2");
	isolate_src(p, cookie, "A2", inverted);
	/*
	 * (dst & ~shmask)
	 */
	if (inverted) {
	    expand(p, cookie, "	and	A3,A2,A3\n");
	} else {
	    expand(p, cookie, "	andn	A3,A2,A3\n");
	}
	expand(p, cookie, "	or	A3,A1,A3\n");
	expand(p, cookie, "	stZLb	A3,AL\n");
    }
    /*
     * the result of a bitfield assignment, if any, must be truncated to the
     * width of the assigned field. 
     */
    if (!(cookie & FOREFF)) {
	if (IN_IMMEDIATE_RANGE(mask)) {
	    expand(p, cookie, "	and	AR,ZX,A1\n");
	} else {
	    expand(p, cookie, "	set	ZX,A1\n");
	    expand(p, cookie, "	and	AR,A1,A1\n");
	}
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * Assign all 0's or all 1's to bit field.  We expect 2 temps. If a result is
 * expected, it is returned in p->in.right. 
 */
static void
assign_field_const(p, cookie)
    NODE *p;
    int cookie;
{
    int mask;			/* bit field mask */
    int shmask;			/* mask << fldshf */
    int inverted;		/* =1 if complement of mask value was loaded */
    int i;			/* for iterating through resc[] */
    NODE *r;			/* rhs of assignment */
    CONSZ value;		/* value to be assigned */

    /*
     * check inputs and register allocation. 
     */
    r = p->in.right;
    value = r->tn.lval;
    if (!(value == 0 || value == -1 || value == bitmask(fldsz))) {
	cerror("illegal constant in assign_field_const");
    }
    for (i = 0; i < 2; i++) {
	if (resc[i].in.op != REG || !isareg(resc[i].tn.rval)) {
	    cerror("illegal register allocation in assign_field");
	}
    }
    mask = bitmask(fldsz);		/* ZX in templates */
    shmask = lshift(mask, fldshf);	/* M in templates  */
    expand(p, cookie, "	ldZLB	AL,A1\n");
    if (IN_IMMEDIATE_RANGE(shmask)) {
	/*
	 * small shifted field: can use immediate operands for both zeroing the
	 * target and isolating the source. 
	 */
	if (value == 0) {
	    /*
	     * (dst &= ~shmask);
	     */
	    expand(p, cookie, "	andn	A1,M,A1\n");
	} else {
	    /*
	     * (dst |= shmask);
	     */
	    expand(p, cookie, "	or	A1,M,A1\n");
	}
    } else {
	/*
	 * default case: must build mask separately. 
	 */
	inverted = mkmask(p, cookie, shmask, "A2");
	if (value == 0) {
	    /*
	     * (dst &= ~shmask)
	     */
	    if (inverted) {
		expand(p, cookie, "	and	A1,A2,A1\n");
	    } else {
		expand(p, cookie, "	andn	A1,A2,A1\n");
	    }
	} else {
	    /*
	     * (dst |= shmask)
	     */
	    if (inverted) {
		expand(p, cookie, "	orn	A1,A2,A1\n");
	    } else {
		expand(p, cookie, "	or	A1,A2,A1\n");
	    }
	}
    }
    expand(p, cookie, "	stZLb	A1,AL\n");
    /*
     * the result of a bitfield assignment, if any, must be truncated to the
     * width of the assigned field. 
     */
    if (!(cookie & FOREFF)) {
	r->tn.lval &= mask;
    }
}
