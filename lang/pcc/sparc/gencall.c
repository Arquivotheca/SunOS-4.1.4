#ifndef lint
static	char sccsid[] = "@(#)gencall.c	1.1	94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include "cpass2.h"
#ifdef sparc
#include <alloca.h>
#endif

#include <sun4/trap.h>

extern int chk_ovfl;

unsigned int offsz, toff;
extern int madecall;
extern int maxargs;
NODE *double_conv();

/* round size up to whole words, which is how we do allocation */
#define WORDSIZE(p)	((p)->stn.stsize+((SZINT/SZCHAR)-1)) / (SZINT/SZCHAR)

static struct {
    int	rname;
    TWORD rtype;
} reglist[RO7-RO0], *rp = &reglist[-1];
#define RESERVE( r, t ) (++rp)->rname = r; rp->rtype = t; rbusy( r, t )
#define RDONE while( rp >= reglist ) {rfree( rp->rname, rp->rtype ); rp-=1;}

extern int dregmap,fregmap;
struct register_map { int dregs, fregs; };

int nparams; /* used by the zzzcode('a') call made by the CALL template */

static
need_save(p)
NODE *p;
{
	static char *op_tab[] = {
		".mul", ".div", ".rem", 
		".umul", ".udiv", ".urem", 
		"inline_ld_short",	"inline_ld_ushort", 
		"inline_ld_int",	"inline_ld_float", 
		"inline_ld_double",	"inline_st_short", 
		"inline_st_int",	"inline_st_float", 
		"inline_st_double",
		""};
	char **h;

	if (p->in.left->in.op == ICON && p->in.left->tn.name[0] != '_'){
		for (h = op_tab; 
		     (*h)[0] != '\0' && strcmp(p->in.left->tn.name, *h);
		     h++);
		if ((*h)[0] != '\0')
			return 0;
	}
	return 1;
}

static void
allocate_reg_save(fpnode)
    register NODE *fpnode;
{
	fpnode->tn.op = OREG;
	fpnode->tn.rval = TMPREG;
	fpnode->tn.lval = 0;
	fpnode->tn.name = tmpname;
	fpnode->tn.rall = NOPREF;
}

static struct register_map
save_regs( fploc )
    NODE* fploc;
{
    int regname;
    NODE t = *fploc;
    struct register_map map;
    
    map.dregs = dregmap;
    map.fregs = fregmap;
    
    for (regname = MIN_FVAR; regname <= MAX_FVAR; regname++){
        if (!F_AVAIL(regname, map.fregs)){
	    if (regname+1 <= MAX_FVAR && !F_AVAIL(regname+1,map.fregs)
	      && ((regname&1) == 0)
	      && (t.tn.lval % (SZDOUBLE/SZCHAR)) == 0 ) {
		/* do two at a time */
		print_str_str( "	std	", rnames[regname] );
		print_str(",");
		oregput( & t );
		print_str("\n");
		t.tn.lval += SZDOUBLE/SZCHAR;
		regname++;
	    } else {
		print_str_str( "	st	", rnames[regname] );
		print_str(",");
		oregput( & t );
		print_str("\n");
		t.tn.lval += SZFLOAT/SZCHAR;
	    }
	}
    }
    for (regname = MIN_GREG; regname <= MAX_GREG; regname++){
        if (!D_AVAIL(regname, map.dregs)){
       	    print_str_str( "	st	", rnames[regname] );
	    print_str(",");
	    oregput( & t );
	    print_str("\n");
	    t.tn.lval += SZINT/SZCHAR; 
	 }
    }    
    /* return which things we saved */
    return map;
}

static void
restore_regs( map, fploc  )
    struct register_map map;
    NODE * fploc;
{
    register regname;
    NODE t = *fploc;
    
    for (regname = MIN_FVAR; regname <= MAX_FVAR; regname++){
        if (!F_AVAIL(regname, map.fregs)){
	    if (regname+1 <= MAX_FVAR && !F_AVAIL(regname+1,map.fregs)
	      && ((regname&1) == 0)
	      && (t.tn.lval % (SZDOUBLE/SZCHAR)) == 0 ) {
		/* do two at a time */
		print_str("	ldd	");
		oregput( & t );
		print_str_str_nl("," , rnames[regname] );
		t.tn.lval += SZDOUBLE/SZCHAR;
		regname++;
	    } else {
		print_str("	ld	");
		oregput( & t );
		print_str_str_nl("," , rnames[regname] );
		t.tn.lval += SZFLOAT/SZCHAR;
	    }
	}
    }
    for (regname = MIN_GREG; regname <= MAX_GREG; regname++){
    	if (!D_AVAIL(regname, map.dregs)){
	    print_str( "	ld	");
	    oregput( & t );
	    print_str_str_nl("," , rnames[regname] );
	    t.tn.lval += SZINT/SZCHAR;
	}
    }
}

static int
eval_funcname( p, fpl )
    register NODE *p;
    NODE *fpl;
{
	int args;
	NODE *funcname;

	switch( p->in.left->tn.op ){
	case ICON:
	case REG:
	    funcname = NIL; break;
	default:
	    p->in.left->in.rall = MUSTDO|RG1;
	    funcname = p->in.left;
	}
	if (fpl)
	    allocate_reg_save(fpl);
	if( p->in.right ) {
		args = make_args( p->in.right, funcname );
	} else {
		args = 0;
		if (funcname != NIL)
			order( funcname, INAREG);
	}
	return args;
}

static int
call_func( p, args, fploc )
	NODE *p;
	int args;
	NODE *fploc;
{
	int m;
	struct register_map fplist;
	int gen_trap = 0;

	madecall = 1;
	if (fploc)
	    fplist = save_regs( fploc );

	/* set this to the number of out REGISTERS full of outgoing parameters */
	nparams = args / (SZINT/SZCHAR);
	if (nparams>6) nparams = 6;

	/* generate trap if integer overflow
	 * but do we care the .div and .rem overflow under the
	 * only possible case ((-MAXINT-1)/-1)??  check with the
	 * library routine.
	 */
	if (chk_ovfl && 
	    (strcmp(p->in.left->tn.name, ".mul") == 0||
	     strcmp(p->in.left->tn.name, ".umul") == 0) )
		gen_trap++;

	RDONE; /* must give back o0 before the call */
	m = match( p, INTAREG|INTBREG );
	if (gen_trap)
		printf("	tnz	%d\n", ST_RANGE_CHECK);
	popargs( args );
	if (fploc)
	    restore_regs(fplist, fploc );
	return(m != MDONE);
}

genscall( p, cookie ) register NODE *p; 
{
	/* structure valued call */
	int tempsize;
	int args;
	NODE fploc, *addr;
	NODE *temploc, *tdest;

	/*
	 * generate all arguments (yeucch),
	 */
	addr = need_save(p) ? &fploc : NIL;
	args = eval_funcname( p, addr );
	/*
	 * Allocate temp space, putting its address on the stack.
	 * We must do this just before the call, because of nested
	 * stcalls.  We must tread cautiously here, because we lack
	 * sufficient registers for all but the most trivial expressions.
	 */
    	tempsize  =  WORDSIZE(p);
	temploc = mktempaddr(tempsize, INCREF(STRTY) );
	tdest = build_tn(OREG, INCREF(STRTY), "",
		STRUCT_VAL_OFF/SZCHAR, ARGREG);
	assign_it( tdest, temploc );
	/*
	 * generate the call
	 * returned value in [%o0] or temp.
	 */
	p->in.op = UNARY STCALL;
	return( call_func( p, args, addr) );
}

gencall( p, cookie ) register NODE *p;
{
	/* generate the call given by p */
	int args;
	NODE fploc;
	NODE *addr;
	
	addr = need_save(p) ? &fploc : NIL;
	args = eval_funcname( p, addr );

	p->in.op = UNARY CALL;
	return( call_func( p, args, addr ) );
}

popargs( size ) register size; 
{
	/* pop arguments from stack */

	if (size==0) return;
	/*
	 * round size up to a multiple of sizeof(long),
	 * to compensate for the hack in gencall() above.
	 */
	SETOFF(size, (SZLONG/SZCHAR));
	toff -= size;
	/* huh? if( toff == 0 && size >= 2 ) size -= 2; */
	if ( maxargs < size ) maxargs = size;
}



incref(p)
    register NODE *p;
{
    /* change (p) into &(p) */
    register NODE *l, *r;
    int val;
    switch (p->in.op){
    case UNARY MUL:
	/* *(l) => (l) */
	l = p->in.left;
	*p = *l;
	l->in.op = FREE;
	break;
    case OREG:
	/* OREG => (+ REG ICON/REG) */
	l = talloc();
	l->tn.op = REG;
	r = talloc();
	val = p->tn.rval;
	if (R2TEST(val)){
	    l->tn.rval = R2UPK1(val);
	    r->tn.op = REG;
	    r->tn.rval = R2UPK2(val);
	} else {
	    l->tn.rval = val;
	    r->tn.op = ICON;
	    r->tn.lval = p->tn.lval;
	    r->tn.name = p->tn.name;
	}
	p->in.op = PLUS;
	p->in.type = INCREF( p->in.type );
	p->in.left = l;
	p->in.right = r;
	l->tn.type = p->in.type;
	r->tn.type = INT;
	break;
    case NAME:
	/* NAME => ICON */
	p->tn.op = ICON;
	p->tn.type = INCREF( p->tn.type );
	break;
    default:
	cerror("Address of expression");
    }
}


extern int dregmap;

typedef struct argstruct { NODE *p; int off, size; } ARG;
#define ADDARG( argpp, argp, argoff, argsize )  \
	(argpp)->p = argp; \
	(argpp)->size = argsize; \
	(argpp)++->off = argoff++
#define NEW( n ) (ARG *) alloca( n * sizeof( ARG ) )
#define LAST_OUT RO5
#define ARGOUT( offset, size ) ( ((offset)+(size)) <= (LAST_OUT+1) )


#define COSTOF(p) ((p)->in.su.d)

static int
compare_costs( first, second )
    register ARG *first, *second;
{
    if (COSTOF(first->p) < COSTOF(second->p) ) return 1;
    else if (COSTOF(first->p) > COSTOF(second->p) ) return -1;
    else return 0;
}

static void
enlist( p, offset, size, callp, stackp, argp ) 
    NODE *p;
    int offset;
    int size;
    ARG **callp;
    ARG **stackp;
    ARG **argp;
{
    /*  put the argument into the appropriate list */
    if ( !SUGT( fregs, p->in.su ) ){ 
    	/* looks like a procedure call to me */
    	ADDARG( *callp, p, offset, size );
    } else if ( ARGOUT( offset, size ) ){
    	/* normal register parameter */
    	ADDARG( *argp, p, offset, size );
    } else {
    	/* memory-bound */
    	ADDARG( *stackp, p, offset, size );
    }
}

int
grab_temp_reg( type )
    TWORD type;
{
    /* want to make a 1-word temporary OREG or TEMP */
    register regno;
    
    if (ISFLOATING(type)) return -1; /* don't try to stash floats in regs */
    NEXTD( regno, dregmap );
    if ( regno >= MIN_DVAR ) { /* registers allocated backwards */
	SETD( dregmap, regno );
    } else {
	regno = -1;
    }
    return regno;
}

static
assign_it( lhs, rhs )
    NODE *lhs, *rhs;
{
    /*
     * get the value of rhs into the location lhs. Build and evaluate
     * an ASSIGN node.
     */
    register NODE *a = talloc();
    a->in.op = ASSIGN;
    a->in.rall = NOPREF;
    a->in.name = "";
    a->in.left = lhs;
    a->in.right = rhs;
    a->in.type = rhs->in.type;
    sucomp( a );
    order( a, FOREFF );
}

static NODE *
assign_to_temp( a )
    ARG *a;
{
    NODE *t = a->p;
    int regname;
    regname = grab_temp_reg( t->in.type);
    if (regname == -1){
	/* no regs available */
	order( t, INTEMP );
    } else {
	/* use that register */
	t->in.rall = MUSTDO | regname;
	order( t, INAREG|INBREG );
    }
    t->tn.su.d = a->size;
    t->tn.su.f = 0;
    t->tn.su.calls = 0;
    return t;
}

static void
assign_to_reg_dest( a ) 
   ARG *a;
{
    register NODE *p = a->p;
    if (callop(p->in.op)){
    	order(p, INTAREG|INTBREG );
    }
    if (!(p->tn.op == REG && p->tn.rval == a->off)){
    	p->tn.rall = MUSTDO|a->off;
    	order(p, INAREG);
    }
    reclaim( p, RNULL, FOREFF );
    RESERVE( a->off, p->in.type);
}

static NODE *
assign_to_stack_dest( a )
    ARG *a;
{
    register NODE *q = talloc();
    NODE *r = talloc();
    q->tn.op = OREG;
    q->tn.type = a->p->in.type;
    q->tn.rval = SPREG;
    q->tn.name = "";
    q->tn.rall = NOPREF;
    q->tn.lval = (a->off-RO0) * (SZINT/SZCHAR) + (ARGINIT/SZCHAR);
    *r = *q;
    assign_it( q, a->p );
    return r;
}

#define STACK_IT( a ) \
{ NODE * q;    \
	q  = assign_to_stack_dest( a ); \
	/* if its half-in-half-out of the registers, we'll correct later */ \
	if (ARGOUT( a->off, 1 ) ){ \
	    q->in.type = UNSIGNED; /* i.e. register */ \
	    ADDARG( nextarg, q, a->off, a->size); \
	}else {\
	    q->in.op = FREE;\
	} \
}

static void
double_arg( p )
    NODE *p;
{
    NODE *q = talloc();
    *q = *p;
    p->in.op = SCONV;
    p->in.type = DOUBLE;
    p->in.left = q;
    sucomp( p );
}

static void
de_struct_arg( p )
    register NODE *p;
{
    register NODE *t;
    int size_in_words;
    /* allocate temporary area for the copy we're going to make */
    size_in_words =  WORDSIZE(p);
    t = mktempaddr( size_in_words, INCREF(STRTY));
    /* now change p into an STASG node, of type pointer-to-struct */
    p->in.right = p->in.left;
    p->in.type = INCREF(STRTY);
    p->in.left = t;
    p->in.op = STASG;
    /* put off doing the actual work for now */
    sucomp( p );
}
    
static int
count_an_arg( p, op, callp, stackp, argp )
    NODE *p;
    int  *op;
    ARG **callp, **stackp, **argp;
{
    TWORD t;
    int size; /* size of the argument: units is SZINT, a.k.a. register size */
    int cumsize; /* cumulative size: units is SZCHAR */
    
    /*
     * This routine is used for traversing the argument list in order
     * and computing the offsets, repairing the types, and triage. The
     * only reason order is important is for computing the offset.
     */
    if (p->in.op == CM){
    	cumsize = count_an_arg( p->in.left, op, callp, stackp, argp );
    	p->in.op = FREE;
    	p = p->in.right;
    } else {
       cumsize = 0;
    }
    t = p->in.type;
    if ( p->in.op == STARG ) {
    	de_struct_arg( p );
    	size = SZPOINT/SZINT; /* really pass a pointer */
    } else {
    	size = szty( t );
    }
    /* now put the argument into the appropriate list */
    enlist( p, *op, size, callp, stackp, argp );
    
    /* adjust arg offset */
    *op += size;
    return cumsize + (size * SZINT/SZCHAR);
}

static int
make_args(p, fn )
    NODE *p, *fn;
{
    NODE *q;
    ARG *calls, *stack, *args;
    ARG *nextcall, *nextstack, *nextarg;
    int argoff;
    int nargs, mostex, dreg_avail;
    int argsize;
    /*
     * count how many total args there are. Rather than figuring out
     * how many can go in each catagory, allocate that much space in
     * all for each of them.
     */
    if (p == 0 ){
        return 0; /* nothing to do */
    }    
    q = p;
    nargs = 1;
    while ( q->in.op == CM){
	nargs += 1;
	q = q->in.left;
    }
    calls = NEW( nargs );
    nextcall = calls;
    stack =  NEW( nargs );
    nextstack = stack;
    args = NEW( 2*nargs );
    nextarg = args;
    
    /*
     * pass over the whole argument list:
     * 1) fix up argument types, such as structs and floats by value
     * 2) assign offsets
     * 3) assign to the appropriate list.
     * Because of the way the recursive data structure is built, 
     * the first argument is furthest down the chain. So we do this
     * with a recursive procedure.
     */
    argoff = RO0;
    argsize = count_an_arg( p, &argoff, &nextcall, &nextstack, &nextarg );
    /*
     * sort the lists as most-expensive-first. for the call list it probably
     * won't make any difference, and in the stack case we only really want
     * to know how expensive the most expensive non-call argument is.
     */
    if (nextcall - calls > 0 )
        qsort( calls, nextcall-calls, sizeof (ARG), compare_costs );
    if (nextstack - stack > 0 ){
        qsort( stack, nextstack-stack, sizeof (ARG), compare_costs );
        mostex = COSTOF(stack->p);
    } else
        mostex = 0;
    if (nextarg - args > 0 ){
        qsort( args, nextarg-args, sizeof (ARG), compare_costs );
        if (COSTOF(args->p) > mostex)
            mostex = COSTOF(args->p);
    }
    if ((fn != NIL) && (COSTOF(fn) > mostex))
    	mostex = COSTOF(fn);
        
    /*
     * the first list we look at is the call list. For all
     * except the last of its elements, we
     * will assign the evaluated object into a temporary, either
     * a local register or an allocated temporary. We cannot evaluate
     * stack-bound arguments directly onto the stack at this point, since
     * that area will be wanted by other calls. Locals, I hope, are
     * ok, since there is no expression complex enough to require all the
     * outs PLUS locals. <<FUTURE BUG HERE>>!
     */
    
    if ( calls < nextcall ){
	while ( calls < nextcall - 1){
	    q = assign_to_temp( calls );
	    /* now put it on the correct list for its ultimate disposition */
	    enlist( q, calls->off, calls->size, 0, &nextstack, &nextarg );
	    calls += 1;
	}
	/*
	 * the last calling argument does not have to be evaluated
	 * into a temp if we don't need its destination for anything else
	 */
	if (!ARGOUT(calls->off, calls->size)  /* destination is on stack, thus not needed */
	  ||( count_free_dregs() - calls->size >= mostex )){
	    /* ...or we won't need that register anyway */
	    if (ARGOUT(calls->off, calls->size ) )
		assign_to_reg_dest( calls );
	    else
		STACK_IT( calls );
	 } else {
	     /* humbug... */
	     q = assign_to_temp( calls );
	     enlist( q, calls->off, calls->size, 0, &nextstack, &nextarg );
	}
    }
    /* 
     * if we must evaluate the name of the function we're calling (into RG1),
     * do so.
     */
    if (fn != NIL )
        order(fn, INAREG);
    /*
     * the next list to look at is the stack-bound arguments. These can
     * always be evaluated immediately, since they will not be taking up any
     * needed resources.
     */
	while ( stack < nextstack ){
	    STACK_IT( stack );
	    stack += 1;
	}
    /*
     * Finally, the register-bound arguments. These are the most subtle.
     * We can evaluate each into its proper register AS LONG AS that
     * register is not needed for the evaluation of further arguments.
     * We sorted this list by cost so that (1) the most expensive 
     * arguments are evaluated when the greatest number of register
     * should be avaliable, and (2) its easy to tell -- by just looking
     * at the cost of the next-in-line -- how many registers will be
     * needed. Even though we may have added arguments to our list after
     * having sorted it, we know each of these is really cheap, since
     * they're already in temps. When we find something that must be
     * evaluated to a temporary location, we'll do so, then put it at
     * the end of our list.
     */
    dreg_avail = count_free_dregs();
    while (args < nextarg ){
    	mostex = (args==(nextarg-1)) ? 0 : COSTOF((args+1)->p); /* cost of next element */
    	if (mostex + args->size <= dreg_avail){
    	    /* evaluate into place */
    	    assign_to_reg_dest( args );
    	    dreg_avail -= args->size;
    	} else {
    	    /* won't fit yet */
    	    q = assign_to_temp( args );
    	    ADDARG( nextarg, q, args->off, args->size );
    	}
    	args += 1;
    }
    /*
     * all done evaluating. do some cleanup
     */
    return argsize;
}

