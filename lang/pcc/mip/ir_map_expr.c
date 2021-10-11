#ifndef lint
static	char sccsid[] = "@(#)ir_map_expr.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

# include "ir_c.h"

/*
 * this module contains the main control routines for
 * PCC => IR mapping.  Much of the code here is adapted from
 * similar routines in the second pass of PCC; see reader.c
 * for the originals.
 */

/* from common.c */
extern void pcc_freenode(/*p*/);	/* free a single PCC node */
extern void pcc_tfree(/*p*/);		/* free an entire PCC tree */

/* from trees.c */
extern PCC_NODE *block(/*op, l, r, tword, dimoff, sizoff */);

/* from local.c */
extern PCC_NODE *p1tmpalloc(/*p*/);	/* allocate a tmp of same type as p */
extern PCC_NODE *pcc_tcopy(/*p*/);	/* recursively copy a PCC tree */

/* from ir_misc */

extern TYPE inttype;
extern TYPE uinttype;

/* from ir_alloc */

extern IR_NODE	*new_triple(/*op,arg1,arg2,type*/);

/* from ir_map */

extern TYPE	ir_type(/*p*/);
extern IR_NODE	*ir_temp(/*p*/);
extern IR_NODE	*ir_leaf(/*p, cookie*/);
extern IR_NODE	*ir_addrof(/*p*/);
extern IR_NODE	*ir_assign(/*p, cookie*/);
extern IR_NODE	*ir_stasg(/*p, cookie*/);
extern IR_NODE	*ir_starg(/*p, cookie*/);
extern IR_NODE	*ir_fval(/*p*/);
extern IR_NODE	*ir_asgop(/*p, ir_opno, cookie*/);
extern IR_NODE	*ir_incr(/*p, ir_opno, cookie*/);
extern IR_NODE	*ir_gencall(/*p, cookie*/);

extern void	ir_pass(/*s*/);
extern void	ir_cbranch(/*p, true_lab, false_lab*/);
extern void	ir_labeldef(/*lab*/);
extern void	ir_bitfield(/*p*/);

/* export */

IR_NODE *ir_map_expr(/*p, cookie*/);
void	ir_compile(/*p*/);
int	ir_deltest(/*p*/);

/* forward */

# define DELAYS 40	/* max # of delayable side effects in a statement */
static int	ir_delay0(/*p*/);
static int	ir_delay1(/*p, cookie*/);
static void	ir_delay2(/*p, deltrees, deli*/);

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static struct node_map {
    int pcc_opno;
    IR_OP ir_opno; 
} node_map_init_tab[] = {
    { PCC_NAME,			IR_ERR },
    { PCC_STRING,		IR_ERR },
    { PCC_REG,			IR_ERR },
    { PCC_OREG,			IR_ERR },
    { PCC_ICON,			IR_ERR },
    { PCC_FCON,			IR_ERR },
    { PCC_UNARY_MINUS,		IR_NEG },
    { PCC_UNARY_MUL,		IR_IFETCH },
    { PCC_UNARY_CALL,		IR_FCALL },
    { PCC_UNARY_FORTCALL,	IR_FCALL },
    { PCC_NOT,			IR_NOT },
    { PCC_COMPL,		IR_COMPL },
    { PCC_FORCE,		IR_FVAL },
    { PCC_INIT,			IR_ERR },
    { PCC_PCONV,		IR_PCONV },
    { PCC_SCONV,		IR_CONV },
    { PCC_PLUS,			IR_PLUS },
    { PCC_ASG_PLUS,		IR_PLUS },
    { PCC_MINUS,		IR_MINUS },
    { PCC_ASG_MINUS,		IR_MINUS },
    { PCC_MUL,			IR_MULT },
    { PCC_ASG_MUL,		IR_MULT },
    { PCC_AND,			IR_AND },
    { PCC_ASG_AND,		IR_AND },
    { PCC_QUEST,		IR_ERR },
    { PCC_COLON,		IR_ERR },
    { PCC_ANDAND,		IR_ERR },
    { PCC_OROR,			IR_ERR },
    { PCC_CM,			IR_ERR },
    { PCC_COMOP,		IR_ERR },
    { PCC_ASSIGN,		IR_ASSIGN },
    { PCC_DIV,			IR_DIV },
    { PCC_ASG_DIV,		IR_DIV },
    { PCC_MOD,			IR_REMAINDER },
    { PCC_ASG_MOD,		IR_REMAINDER },
    { PCC_LS,			IR_LSHIFT },
    { PCC_ASG_LS,		IR_LSHIFT },
    { PCC_RS,			IR_RSHIFT },
    { PCC_ASG_RS,		IR_RSHIFT },
    { PCC_OR,			IR_OR },
    { PCC_ASG_OR,		IR_OR },
    { PCC_ER,			IR_XOR },
    { PCC_ASG_ER,		IR_XOR },
    { PCC_INCR,			IR_PLUS },
    { PCC_DECR,			IR_MINUS },
    { PCC_CALL,			IR_FCALL },
    { PCC_FORTCALL,		IR_FCALL },
    { PCC_EQ,			IR_EQ },
    { PCC_NE,			IR_NE },
    { PCC_LE,			IR_LE },
    { PCC_LT,			IR_LT },
    { PCC_GE,			IR_GE },
    { PCC_GT,			IR_GT },
    { PCC_UGT,			IR_GT },
    { PCC_UGE,			IR_GE },
    { PCC_ULT,			IR_LT },
    { PCC_ULE,			IR_LE },
    { PCC_CBRANCH,		IR_CBRANCH },
    { PCC_FLD,			IR_ERR },
    { PCC_GOTO,			IR_GOTO },
    { PCC_STASG,		IR_ASSIGN },
    { PCC_STARG,		IR_ERR },
    { PCC_STCALL,		IR_FCALL },
    { PCC_UNARY_STCALL,		IR_FCALL },
    { PCC_CHK,			IR_ERR },
    { PCC_FABS,			IR_ERR },
    { PCC_FCOS,			IR_ERR },
    { PCC_FSIN,			IR_ERR },
    { PCC_FTAN,			IR_ERR },
    { PCC_FACOS,		IR_ERR },
    { PCC_FASIN,		IR_ERR },
    { PCC_FATAN,		IR_ERR },
    { PCC_FCOSH,		IR_ERR },
    { PCC_FSINH,		IR_ERR },
    { PCC_FTANH,		IR_ERR },
    { PCC_FEXP,			IR_ERR },
    { PCC_F10TOX,		IR_ERR },
    { PCC_F2TOX,		IR_ERR },
    { PCC_FLOGN,		IR_ERR },
    { PCC_FLOG10,		IR_ERR },
    { PCC_FLOG2,		IR_ERR },
    { PCC_FSQR,			IR_ERR },
    { PCC_FSQRT,		IR_ERR },
    { PCC_FAINT,		IR_ERR },
    { PCC_FANINT,		IR_ERR },
    { PCC_FNINT,		IR_ERR },
    { -1 }
};

static struct node_map node_map_tab[PCC_DSIZE]; 

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

mk_irdope()
{
    register struct node_map *p;

    if (node_map_tab[PCC_NAME].pcc_opno != PCC_NAME) {
	for( p = node_map_init_tab; p->pcc_opno >= 0; ++p ) {
	    node_map_tab[p->pcc_opno] = *p;
	}
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * map a PCC expression tree to IR.  If cookie == FOR_VALUE, the
 * result of the translation must generate the value of the
 * expression.  Otherwise, the expression may be evaluated
 * for effect.  In particular, since assignment operators in IR
 * do not have values, result == VALUE means that an explicit
 * temporary must be generated as part of the assignment. (bletch)
 *
 * The parameter is called "cookie" for historical reasons.
 */
IR_NODE *
ir_map_expr(p, cookie)
    register PCC_NODE *p;
    COOKIE cookie;
{
    TYPE t;
    int op;
    PCC_NODE *lp, *rp;
    IR_NODE *ir_lp, *ir_rp;
    IR_NODE *result;
    IR_NODE *ir_tmp;
    struct node_map *map;
    int true_lab, false_lab, join_lab;

    if (ir_debug > 2) {
	printf("ir_map_expr called on: \n");
	e1pr(p);
    }
    op = p->in.op;
    if (cookie == FOR_ADDRESS
      && pcc_optype(op) != PCC_LTYPE && op != PCC_UNARY_MUL) {
	cerror("cannot take address of op %s", pcc_opname(op));
    }
    map = &node_map_tab[op];
    switch(op) {
    case PCC_AUTO:
    case PCC_REG:
    case PCC_FCON:
    case PCC_NAME:
    case PCC_ICON:
	/* leaf nodes */
	switch(cookie) {
	case FOR_EFFECT:
	    result = NULL;
	    pcc_freenode(p);
	    break;
	case FOR_VALUE:
	case FOR_ADDRESS:
	case FOR_ASSIGN:
	    result = ir_leaf(p, cookie);
	    break;
	default:
	    goto undefined_goal;
	}
	break;

    case PCC_FLD:
	switch(cookie) {
	case FOR_VALUE:
	case FOR_ASSIGN:
	    t = ir_type(p);
	    /* crush cans before recycling */
	    ir_bitfield(p);	/* changes p->in.op */
	    result = ir_map_expr(p, cookie);
	    if (cookie == FOR_VALUE) {
		result = new_triple(IR_CONV, result, NULL, t);
	    }
	    break;
	case FOR_EFFECT:
	    result = ir_map_expr(p->in.left, cookie);
	    pcc_freenode(p);
	    break;
	default:
	    goto undefined_goal;
	}
	break;

    case PCC_UNARY_MUL:
	lp = p->in.left;
	switch(cookie) {
	case FOR_ADDRESS:
	    result = ir_map_expr( lp, FOR_VALUE );
	    pcc_freenode(p);
	    break;
	case FOR_VALUE:
	    ir_lp = ir_map_expr( lp, FOR_VALUE );
	    result = new_triple(IR_IFETCH, ir_lp, NULL, ir_type(p));
	    pcc_freenode(p);
	    break;
	case FOR_EFFECT:
	    result = ir_map_expr( lp, cookie );
	    pcc_freenode(p);
	    break;
	default:
	    goto undefined_goal;
	}
	break;

    case PCC_UNARY_MINUS:	/* IR_NEG */
    case PCC_COMPL:		/* IR_NOT */
    case PCC_SCONV:		/* IR_CONV */
    case PCC_PCONV:		/* IR_PCONV */
	/* simple unary ops */
	switch(cookie) {
	case FOR_VALUE:
	case FOR_EFFECT:
	    ir_lp = ir_map_expr( p->in.left, cookie );
	    if (cookie == FOR_VALUE) {
		result = new_triple(map->ir_opno, ir_lp, NULL, ir_type(p));
	    } else {
		result = NULL;
	    }
	    pcc_freenode(p);
	    break;
	default:
	    goto undefined_goal;
	}
	break;

    case PCC_FORCE:		/* IR_FVAL */
	/* return function result */
	switch(cookie) {
	case FOR_VALUE:
	case FOR_EFFECT:
	    result = ir_fval(p, cookie);
	    pcc_freenode(p);
	    break;
	default:
	    goto undefined_goal;
	}
	break;

    case PCC_PLUS:		/*IR_PLUS */
    case PCC_MINUS:		/*IR_MINUS */
    case PCC_MUL:		/*IR_MULT */
    case PCC_AND:		/*IR_AND */
    case PCC_DIV:		/*IR_DIV */
    case PCC_MOD:		/*IR_REMAINDER */
    case PCC_LS:		/*IR_LSHIFT */
    case PCC_RS:		/*IR_RSHIFT */
    case PCC_OR:		/*IR_OR */
    case PCC_ER:		/*IR_XOR */
    case PCC_EQ:		/*IR_EQ */
    case PCC_NE:		/*IR_NE */
    case PCC_LE:		/*IR_LE */
    case PCC_LT:		/*IR_LT */
    case PCC_GE:		/*IR_GE */
    case PCC_GT:		/*IR_GT */
    case PCC_UGT:		/*IR_GT */
    case PCC_UGE:		/*IR_GE */
    case PCC_ULT:		/*IR_LT */
    case PCC_ULE:		/*IR_LE */
	/* simple binary ops */
	switch(cookie) {
	case FOR_VALUE:
	case FOR_EFFECT:
	    ir_lp = ir_map_expr( p->in.left, cookie );
	    ir_rp = ir_map_expr( p->in.right, cookie );
	    if (cookie == FOR_VALUE) {
		result = new_triple(map->ir_opno, ir_lp, ir_rp, ir_type(p));
	    } else {
		result = NULL;
	    }
	    pcc_freenode(p);
	    break;
	default:
	    goto undefined_goal;
	}
	break;

    case PCC_ASSIGN:
	/* normal assignments */
	result = ir_assign(p, cookie);
	break;

    case PCC_STASG:
	/* structure assignments */
	result = ir_stasg(p, cookie);
	break;

    case PCC_STARG:
	/* structure arguments */
	result = ir_starg(p, cookie);
	break;

    case PCC_ASG_PLUS:		/* IR_PLUS */
    case PCC_ASG_MINUS:		/* IR_MINUS */
    case PCC_ASG_MUL:		/* IR_MULT */
    case PCC_ASG_AND:		/* IR_AND */
    case PCC_ASG_DIV:		/* IR_DIV */
    case PCC_ASG_MOD:		/* IR_REMAINDER */
    case PCC_ASG_LS:		/* IR_LSHIFT */
    case PCC_ASG_RS:		/* IR_RSHIFT */
    case PCC_ASG_OR:		/* IR_OR */
    case PCC_ASG_ER:		/* IR_XOR */
	/* arithmetic/logical ops with assignment */
	result = ir_asgop(p, map->ir_opno, cookie);
	break;

    case PCC_INCR:		/* IR_PLUS */
    case PCC_DECR:		/* IR_MINUS */
	/* post increment/decrement */
	result = ir_incr(p, map->ir_opno, cookie);
	break;

    case PCC_COMOP:
	/* sequence; evaluate subtrees in order */
	ir_compile(p->in.left);
	if (cookie == FOR_VALUE) {
	    /* value required, compile rhs and assign to temp */
	    t = ir_type(p->in.right);
	    ir_tmp = ir_temp(p->in.right);
	    result = ir_map_expr( p->in.right, FOR_VALUE );
	    (void)new_triple(IR_ASSIGN, ir_tmp, result, t);
	    result = ir_tmp;
	} else {
	    result = ir_map_expr(p->in.right, cookie);
	}
	pcc_freenode(p);
	break;

    case PCC_CBRANCH:
	ir_cbranch(p->in.left, LABFALLTHRU, p->in.right->tn.lval);
	pcc_freenode(p->in.right);
	pcc_freenode(p);
	result = NULL;
	break;

    case PCC_QUEST:
	/* rhs is PCC_COLON, both sides of which should agree in type */
	lp = p->in.left;
	rp = p->in.right;
	ir_cbranch( lp, LABFALLTHRU, false_lab=getlab() );
	if (rp->in.type == PCC_TVOID) {
	    /* compile true case, discarding result */
	    (void)ir_map_expr( rp->in.left, FOR_EFFECT );
	    ir_branch(join_lab = getlab());
	    ir_labeldef( false_lab );
	    /* compile false case, discarding result */
	    (void)ir_map_expr( rp->in.right, FOR_EFFECT );
	    result = NULL;
	} else {
	    /* compile value of true case and assign to a temp */
	    t = ir_type(rp);
	    ir_tmp = ir_temp(rp);
	    result = ir_map_expr( rp->in.left, FOR_VALUE );
	    (void)new_triple(IR_ASSIGN, ir_tmp, result, t);
	    ir_branch(join_lab = getlab());
	    ir_labeldef( false_lab );
	    /* compile value of false case and assign to the same temp */
	    result = ir_map_expr( rp->in.right, FOR_VALUE );
	    (void)new_triple(IR_ASSIGN, ir_tmp, result, t);
	    /* result is temp */
	    result = ir_tmp;
	}
	ir_labeldef( join_lab );
	pcc_freenode(rp);
	pcc_freenode(p);
	break;

    case PCC_ANDAND:
    case PCC_OROR:
    case PCC_NOT:
	/* if here, must be a logical operator for 0-1 value */
	ir_tmp = ir_temp(p);
	t = ir_type(p);
	ir_cbranch( p, LABFALLTHRU, false_lab=getlab() );
	/* compile temp := 1 */
	(void)new_triple(IR_ASSIGN, ir_tmp, ileaf(1), t);
	ir_branch(join_lab = getlab());
	ir_labeldef( false_lab );
	/* compile temp := 0 */
	(void)new_triple(IR_ASSIGN, ir_tmp, ileaf(0), t);
	ir_labeldef( join_lab );
	/* result is temp */
	result = ir_tmp;
	break;

    case PCC_UNARY_CALL:
    case PCC_UNARY_FORTCALL:
    case PCC_CALL:
    case PCC_FORTCALL:
    case PCC_STCALL:
    case PCC_UNARY_STCALL:
	result = ir_gencall(p, cookie);
	break;


    default:
	cerror("op '%s' not implemented yet", pcc_opname(op));
	/*NOTREACHED*/

    undefined_goal:
	cerror("goal %d not defined for op %s", (int)cookie, pcc_opname(op));
	/*NOTREACHED*/
    }
    return result;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * Main routine for PCC=>IR mapping.  A ripoff of delay()
 * from the second pass of PCC (original sources in reader.c).
 * Search for delayable post-increment and post-decrement
 * side effect operators, and delay them until the end
 * of the current statement.  Note: don't delay ++ and --
 * within calls, or things like getchar (in their macro forms)
 * will start behaving strangely
 */
void
ir_compile( p )
    register PCC_NODE *p;
{
    register i;
    register PCC_NODE *q;
    PCC_NODE *deltrees[DELAYS];	/* list of delayed side effects */
    int	deli;			/* # of delayed side effects */

    if (ir_debug > 1) {
	printf("ir_compile called on:\n");
	e1pr(p);
    }

    /* look for hard-to-delay ++ and --, and create 'delay slots'
       using COMOP */

    (void)ir_delay0( p );

    /* look for visible COMOPS, and rewrite repeatedly */

    while( ir_delay1( p, FOR_EFFECT ) ) { /* VOID */ }

    /* look for visible, delayable ++ and -- */

    deli = 0;
    ir_delay2( p, deltrees, &deli );
    (void)ir_map_expr( p, FOR_EFFECT );  /* do what is left */
    for( i = 0; i<deli; ++i ) {
	/* compile delayed side effects */
	q = deltrees[i];
	(void)ir_map_expr( q, FOR_EFFECT );
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * rewrite expression (...) as (tmp = (...), tmp).
 * This creates an opportunity for ir_delay2() to extract
 * a post-increment side effect.
 */
static PCC_NODE *
ir_delay_slot(p)
    register PCC_NODE *p;
{
    PCC_NODE *tmp;
    tmp = p1tmpalloc(p);
    p = block(PCC_ASSIGN, tmp, p, p->fn.type, p->fn.cdim, p->fn.csiz);
    p = block(PCC_COMOP, p, pcc_tcopy(tmp), p->fn.type, p->fn.cdim, p->fn.csiz);
    return p;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static PCC_NODE *
ir_delay_args(p)
    register PCC_NODE *p;
{
    if (p->in.op == PCC_CM) {
	p->in.left = ir_delay_args(p->in.left);
	if (ir_delay0(p->in.right)) {
	    p->in.right = ir_delay_slot(p->in.right);
	}
	return p;
    }
    return (ir_delay0(p) ? ir_delay_slot(p) : p);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * returns 1 if an expression contains a post-increment or post-decrement
 * side effect with no place to delay the side effect.  If a suitable
 * place is found along the way,  make a "delay slot" using COMOP
 * and a temporary, and return 0.
 *
 * The intent is to rewrite things like
 *	while (*p++ && *q++) ...
 * as
 *	while ((tmp = *p++, tmp) && (tmp = *q++, tmp)) ...
 *
 * Such constructs will generate code patterns that are easy to
 * recognize as opportunities for machine idioms, in a peephole
 * optimizer with data flow analysis.  The code will also use fewer
 * temporaries than would be required if we were to translate the
 * post-increment and post-decrement operators directly.
 */
static int
ir_delay0( p )
    register PCC_NODE *p;
{
    /*
     * Look for the cases that ir_delay2() can't deal
     * with.  Examine the expression for post-increment
     * side effects, and if any are found, rewrite as
     * (tmp = (...), tmp) (thus creating a "delay slot")
     */
    switch( p->in.op ){
    case PCC_CALL:
    case PCC_STCALL:
    case PCC_FORTCALL:
	if ( ir_delay0(p->in.left) ) {
	    p->in.left = ir_delay_slot(p->in.left);
	}
	p->in.right = ir_delay_args(p->in.right);
	return 0;
    case PCC_UNARY_CALL:
    case PCC_UNARY_STCALL:
    case PCC_UNARY_FORTCALL:
	if ( ir_delay0(p->in.left) ) {
	    p->in.left = ir_delay_slot(p->in.left);
	}
	return 0;
    case PCC_COMOP:
	if ( ir_delay0(p->in.right) ) {
	    p->in.right = ir_delay_slot(p->in.right);
	}
	return 0;
    case PCC_QUEST:
	if ( ir_delay0(p->in.left) ) {
	    p->in.left = ir_delay_slot(p->in.left);
	}
	p = p->in.right;
	/* FALL THROUGH */
    case PCC_EQ:
    case PCC_NE:
    case PCC_LE:
    case PCC_LT:
    case PCC_GE:
    case PCC_GT:
    case PCC_UGT:
    case PCC_UGE:
    case PCC_ULT:
    case PCC_ULE:
    case PCC_ANDAND:
    case PCC_OROR:
	if ( ir_delay0(p->in.left) ) {
	    p->in.left = ir_delay_slot(p->in.left);
	}
	if ( ir_delay0(p->in.right) ) {
	    p->in.right = ir_delay_slot(p->in.right);
	}
	return 0;
    case PCC_FORCE:
    case PCC_NOT:
    case PCC_CBRANCH:
	if ( ir_delay0(p->in.left) ) {
	    p->in.left = ir_delay_slot(p->in.left);
	}
	return 0;
    case PCC_INCR:
    case PCC_DECR:
	return pcc_optype(p->in.left->in.op) == PCC_LTYPE;
    case PCC_STASG:
    case PCC_STARG:
	/* avoid making structured temporaries */
	return 0;
    default:
	break;
    }
    /*
     * for the other ops, propagate the information upwards
     */
    switch(pcc_optype( p->in.op )) {
    case PCC_LTYPE:
	return 0;
    case PCC_UTYPE:
	return ir_delay0(p->in.left);
	break;
    case PCC_BITYPE:
	/* yes, evaluate both sides */
	return ir_delay0(p->in.left) | ir_delay0(p->in.right);
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int
ir_delay1( p, cookie )
    register PCC_NODE *p;
    COOKIE cookie;
{
    /* look for COMOPS */
    register o, ty;
    PCC_NODE *q;

    o = p->in.op;
    ty = pcc_optype( o );
    if( ty == PCC_LTYPE )
	return( 0 );
    if( ty == PCC_UTYPE )
	return( ir_delay1( p->in.left, FOR_VALUE ) );
    switch( o ){
    case PCC_QUEST:
    case PCC_ANDAND:
    case PCC_OROR:
	/* don't look on RHS */
	return( ir_delay1(p->in.left, FOR_VALUE ) );
    case PCC_COMOP:
	/* completely evaluate the LHS */
	ir_compile( p->in.left );
	if (cookie == FOR_VALUE) {
	    /*
	     * assign the RHS into a temporary,
	     * and rewrite the COMOP as a reference to
	     * the same temporary.
	     */
	    q = p1tmpalloc(p->in.right);
	    q = block(PCC_ASSIGN, q, p->in.right,
		p->fn.type, p->fn.cdim, p->fn.csiz);
	    *p = *(q->in.left);
	    ir_compile( q );
	} else {
	    /*
	     * RHS replaces root
	     */
	    q = p->in.right;
	    *p = *q;
	    pcc_freenode(q);
	}
	return( 1 );
    }
    return( ir_delay1(p->in.left, FOR_VALUE)
	|| ir_delay1(p->in.right, FOR_VALUE) );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * look for delayable ++ and -- operators
 */
static void
ir_delay2( p, deltrees, deli )
    register PCC_NODE *p;
    PCC_NODE *deltrees[];
    int *deli;
{
    register o, ty;
    PCC_NODE *q;

    o = p->in.op;
    ty = pcc_optype( o );
    switch( o ){
    case PCC_NOT:
    case PCC_FORCE:
    case PCC_QUEST:
    case PCC_ANDAND:
    case PCC_OROR:
    case PCC_CALL:
    case PCC_UNARY_CALL:
    case PCC_STCALL:
    case PCC_UNARY_STCALL:
    case PCC_FORTCALL:
    case PCC_UNARY_FORTCALL:
    case PCC_COMOP:
    case PCC_CBRANCH:
	/*
	 * don't delay past a conditional context,
	 * or inside of a call
	 */
	return;
    case PCC_INCR:
    case PCC_DECR:
	if(ir_deltest(p->in.left) && *deli < DELAYS ) {
	    /* stash away the side effect */
	    deltrees[(*deli)++] = pcc_tcopy(p);
	    q = p->in.left;
	    pcc_freenode(p->in.right);	/* zap constant */
	    *p = *q;			/* hoist the left operand */
	    pcc_freenode(q);		/* and discard the original */
	    return;
	}
	break;
    }
    if( ty == PCC_BITYPE ) ir_delay2( p->in.right, deltrees, deli );
    if( ty != PCC_LTYPE ) ir_delay2( p->in.left, deltrees, deli );
}

/*
 * return 1 if this operand is a local variable
 */
static int
pcc_islocal(p)
    PCC_NODE *p;
{
    if (p->in.op == PCC_NAME) {
	struct pcc_symtab *sym;
	sym = PCC_STP(p->tn.rval);
	if (sym != NULL) {
	    switch(pcc_sclass(sym)) {
	    case PCC_AUTO:
	    case PCC_PARAM:
	    case PCC_REGISTER:
		return 1;
	    }
	}
    }
    return 0;
}

/*
 * return 1 if we believe it is economical to delay
 * postincrement side effects on this operand
 */
int
ir_deltest(p)
    PCC_NODE *p;
{
#   if TARGET == SPARC
	return pcc_islocal(p);
#   endif
#   if TARGET == MC68000
	/* incrementing memory is relatively cheap on the 68k */
	if (pcc_optype(p->in.op) == PCC_LTYPE)
	    return 1;
	if (p->in.op == PCC_UNARY_MUL)
	    return pcc_islocal(p->in.left);
	return 0;
#   endif
}
