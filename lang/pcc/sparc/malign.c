#ifndef lint
static  char sccsid[] = "@(#)malign.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include "cpass2.h"

#define nncon(p) ((p)->in.op == ICON && !(p)->tn.name[0] \
	&& IN_IMMEDIATE_RANGE((p)->tn.lval))

/*
 * return 0 if address p is aligned to len
 */
static int
x_align(p, len)
NODE *p;
{
	NODE *r;

	if (len == 1)
		return 0;
	if (p->in.op == NAME ||
	    p->in.op == ICON && p->in.name[0] == '\0'){
		return (p->tn.lval % len);
	} else if (p->in.op == REG){
		return (p->tn.rval != SPREG && p->tn.rval != STKREG);
	} else if (p->in.op == PLUS || p->in.op == MINUS){
		return (x_align(p->in.left, len) || x_align(p->in.right, len));
	} else if (p->in.op == LS){
		r = p->in.right;
		if (r->tn.op == ICON && r->tn.name[0] == '\0')
			return ((1 << (r->tn.lval)) % len);
	} else if (p->in.op == MUL){
		return (x_align(p->in.left, len) && x_align(p->in.right, len));
	}
	return 1;
}

/*
 * a local routine of copying a tree
 */
static NODE *
treecopy(p)
NODE *p;
{
	NODE *q;

	ncopy( q=talloc(), p );
	switch( optype(q->in.op) ){
	case BITYPE:
		q->in.right = treecopy(p->in.right);
	case UTYPE:
		q->in.left = treecopy(p->in.left);
	}
	return (q);
}

/*
 * rewrite incr, decr, and asgops, so that
 * -- the only assignments in the tree are ASSIGN ops,
 * -- side effects are not duplicated.
 *
 * Used to rewrite ASG {MUL,DIV,MOD} trees prior
 * to transforming them into library calls.
 *
 * Extended to deal with bit fields.
 *
 * The tree is rewritten in place.
 *
 * The return value is a copy of the original operator
 * stripped of the ASG qualifier; its position in the
 * new tree depends on the details of the rewrite.
 */
NODE *
rw_asgop(p)
	NODE *p;
{
	NODE *l, *q, *r;
	NODE *ll, *lr;
	int op, simple, postinc;
	NODE *deassigned_op= NULL;

	postinc = 0;
	switch (p->in.op) {
	case INCR:
		op = PLUS;
		postinc = 1;
		break;
	case DECR:
		op = MINUS;
		postinc = 1;
		break;
	default:
		if (!asgop(p->in.op)) {
			cerror("expected ASG op, got op=%d\n", p->in.op);
			/*NOTREACHED*/
		}
		op = NOASG p->in.op;
		break;
	}
	/*
	 * probe for FLD and UNARY MUL
	 */
	l = p->in.left;
	q = NULL;
	if (l->in.op == FLD) {
		q = l;
		l = l->in.left;
	}
	if (l->in.op == UNARY MUL){
		q = l;
		l = l->in.left;
	}
	/*
	 * classify the rewriting job, based on the
	 * complexity of the destination address.
	 */
	switch (l->in.op) {
	case NAME:
	case OREG:
	case REG:
	case ICON:
		simple = 1;
		break;
	case PLUS:
	case MINUS:
		ll = l->in.left;
		lr = l->in.right;
		simple = ((ll->in.op == REG || nncon(ll))
		       && (lr->in.op == REG || nncon(lr)));
		break;
	default:
		simple = 0;
		break;
	}
	/*
	 * rewrite (*p) in place as a series of assignments
	 * connected by COMOP ops.  Result is in p->in.right.
	 */
	if (!postinc) {
		NODE *tmp;
		NODE *assigntmp;
		if (simple) {
			/*
			 * simple cases, rewrite as x = x op y
			 */
			l = treecopy(p->in.left);
			p->in.right = build_in(op, p->in.type, l, p->in.right);
			p->in.op = ASSIGN;
			return(p->in.right);
		} 
		/*
		 * tmp = (...);
		 */
		tmp = build_tn(OREG, l->in.type, NULL, (CONSZ)0, 0);
		mktempnode(tmp, 1);
		assigntmp = build_in(ASSIGN, l->in.type, tmp, l);
		/*
		 * *tmp = *tmp op y;  [Note *tmp may be covered by a FLD]
		 */
		q->in.left = treecopy(tmp);
		l = p->in.left;
		r = treecopy(l);
		r = build_in(op, p->in.type, r, p->in.right);
		deassigned_op = r;
		r = build_in(ASSIGN, p->in.type, l, r);
		p->in.op = COMOP;
		p->in.left = assigntmp;
		p->in.right = r;
	} else {
		/*
		 * Postincrement/postdecrement ops.
		 *
		 * Starting with (left) {++|--} (right), we have two cases:
		 *
		 * 1. In the "simple" case, the original value of the left
		 *    operand is copied into a temp, and the left operand
		 *    may be referenced a second time to complete the side
		 *    effect:
		 *
		 *	(tmp1=(left), (left)=tmp1 {+|-} (right), tmp1)
		 *
		 * 2. In the non-simple case, the address of the left
		 *    operand must be evaluated into a temporary, because
		 *    the operand itself may have side effects:
		 *
		 *    (tmp1=&(left), tmp2=*tmp1, *tmp1=tmp2{+|-}(right), tmp2)
		 */
		NODE *tmp1, *tmp2;
		NODE *assign1, *assign2;
		TWORD t;
		if (simple) {
			/*
			 * assign1: tmp1 = (left);
			 */
			l = p->in.left;
			t = l->in.type;
			tmp1 = build_tn(OREG, t, NULL, (CONSZ)0, 0);
			mktempnode(tmp1, 1);
			assign1 = build_in(ASSIGN, t, tmp1, l);
			/*
			 * assign2: (left) = tmp1 {+|-} (right);
			 */
			r = p->in.right;
			r = build_in(op, t, treecopy(tmp1), r);
			deassigned_op = r;
			assign2 = build_in(ASSIGN, t, treecopy(l), r);
			/*
			 * rewrite (*p) in place as:
			 * ( (assign1, assign2) , tmp1 )
			 */
			p->in.left = build_in(COMOP, t, assign1, assign2);
			p->in.right = treecopy(tmp1);
			p->in.op = COMOP;
		} else {
			/*
			 * assign1: tmp1 = &(left);
			 */
			t = l->in.type; /* (l) == address of lhs */
			tmp1 = build_tn(OREG, t, NULL, (CONSZ)0, 0);
			mktempnode(tmp1, 1);
			assign1 = build_in(ASSIGN, t, tmp1, l);
			/*
			 * assign2: tmp2 = (*tmp1);
			 */
			l = p->in.left;	/* (l) == value of lhs (may be FLD) */
			t = l->in.type;
			q->in.left = treecopy(tmp1);
			r = treecopy(l);
			tmp2 = build_tn(OREG, t, NULL, (CONSZ)0, 0);
			mktempnode(tmp2, szty(l));
			assign2 = build_in(ASSIGN, t, tmp2, r);
			/*
			 * rewrite (*p) in place as:
			 * ((assign1), (assign2), *tmp1=tmp2{+|-}(right), tmp2)
			 */
			r = p->in.right;
			r = build_in(op, t, treecopy(tmp2), r);
			deassigned_op = r;
			r = build_in(ASSIGN, t, l, r);
			l = build_in(COMOP, t, assign1, assign2);
			p->in.op = COMOP;
			p->in.left = build_in(COMOP, t, l, r);
			p->in.right = treecopy(tmp2);
		}
	}
	return(deassigned_op);
}

/*
 * change NAME to (CALL ld_xx ICON)
 * change (U* (...)) to (CALL ld_xx (...))
 */
static void
malign_ld(p)
NODE *p;
{
	struct ld_op { int ty; char *name };
	static struct ld_op ld_ops[] = {
		SHORT,	"inline_ld_short",
		USHORT,	"inline_ld_ushort",
		INT,	"inline_ld_int",
		UNSIGNED, "inline_ld_int",
		FLOAT,	"inline_ld_float",
		DOUBLE,	"inline_ld_double",
		-1
	};
	TWORD ty;
	struct ld_op *h;
	char *name;
	NODE *param;

	ty = p->in.type;

	/* find the proper routine name */
	for (h = ld_ops; h->ty != ty && h->ty != -1; h++);
	if (h->ty == -1)
		if (ISPTR(ty))
			name = "inline_ld_int";
		else
			cerror("ty not in ld_op table\n");
	else
		name = h->name;

	/* make the address as parameter */
	if (p->in.op == NAME){
		param = build_tn(ICON, INCREF(ty), p->in.name, p->tn.lval, p->tn.rval);
	} else { /* p->in.op == U* */
		param = p->in.left;
	}
	p->in.op = CALL;
	p->in.right = param;
	p->in.left = build_tn (ICON, INCREF( FTN + ty ), name, 0, 0);
}

/*
 * change (= NAME (...)) to (CALL st_xx (CM ICON (...)))
 * change (= (U* (.A.)) (.B.)) to (CALL st_xx (CM (.A.) (.B.)))
 */
static void
malign_st(p)
NODE *p;
{
	struct st_op { int ty; char *name };
	static struct st_op st_ops[] = {
		SHORT,	"inline_st_short",
		USHORT,	"inline_st_ushort",
		INT,	"inline_st_int",
		UNSIGNED, "inline_st_int",
		FLOAT,	"inline_st_float",
		DOUBLE,	"inline_st_double",
		-1
	};
	TWORD ty;
	struct st_op *h;
	char *name;
	NODE *param;

	/* find the proper routine name */
	ty = p->in.type;
	for (h = st_ops; h->ty != ty && h->ty != -1; h++);
	if (h->ty == -1)
		if (ISPTR(ty))
			name = "inline_st_int";
		else
			cerror("ty not in st_op table\n");
	else
		name = h->name;

	/* param is the address tree */
	param = p->in.left;

	/* do necessary tree rewrite */
	if (param->in.op == NAME){
		param->in.op = ICON;
		param->in.type = INCREF(param->in.type);
	} else { /* param->in.op == U* */
		param->in.op = FREE; /* free node U* */
		param = param->in.left;
	}

	p->in.op = CALL;
	p->in.right = build_in (CM, INT, p->in.right, param);
	p->in.left = build_tn (ICON, INCREF( FTN + ty ), name, 0, 0);
}

void
malign_ld_st(p)
register NODE *p;
{
	int o;

	o = p->in.op;
	if (o == STASG) {
		/*
		 * misaligned structure assignments are handled
		 * in struct.c; here we only deal with the operands
		 * (below)
		 */
	} else if (asgop(o)){
		NODE *q;

		q = p->in.left;
		/* we don't care bit-field */
		if (q->in.op != FLD){
			/* if we find (U* (NAME)) or (U* (mis-align)) or
			   (mis-align-NAME) */
			if (q->in.op == UNARY MUL &&
			    (q->in.left->in.op == NAME && tlen(q) > 1 ||
			    x_align(q->in.left, tlen(q))) ||
			    q->in.op == NAME && x_align(q, tlen(q))){
				if (o == ASSIGN) {
					malign_st(p);
				} else {
					(void)rw_asgop(p);
					malign_ld_st(p);
					return;
				}
			}
		}
	} else if (o == UNARY MUL){
		int op = p->in.left->in.op;
		if (op != STASG && op != STCALL && op != UNARY STCALL &&
		    (op == NAME && tlen(p) > 1 || 
		    x_align(p->in.left, tlen(p))))
			malign_ld(p);
	} else if (o == NAME){
		if (x_align(p, tlen(p)))
			malign_ld(p);
	} else if (o == FLD){
		return;
	}
	switch (optype(p->in.op)){
	case BITYPE:
		malign_ld_st(p->in.right);
	case UTYPE:
		malign_ld_st(p->in.left);
	}
}
