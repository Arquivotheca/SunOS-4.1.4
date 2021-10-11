#ifndef lint
static	char sccsid[] = "@(#)local.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "cpass1.h"

extern int twopass;
extern int safe_opt_level;	/* safe optimization level, from source info */
#define SSMALL SREF

/*
 * built in function names.  This looks a lot like a
 * similar hack in pass 2, but requires some information
 * that is gone by then.
 */

static NODE *p1builtin_va_arg_incr(/*p*/);

struct builtin_func p1builtin_funcs[] = {
	"__builtin_va_arg_incr", p1builtin_va_arg_incr,
	NULL
};

/*
 * clocal(p)
 *	do local transformations on an expression tree prior to
 *	backend processing.  In one-pass operation, the major essential
 *	job is rewriting the automatic variables and arguments in terms of
 *	explicit address arithmetic.  Note that this should NOT be done
 *	if global optimization is to follow.
 *
 *	Other assorted machine-dependent tasks may be done here as well,
 *	although if they are source language independent they should be
 *	done in the backend instead.
 */
NODE *
clocal(p) NODE *p; 
{
	/* in addition, any special features (such as rewriting
	   exclusive or) are easily handled here as well */

	register struct symtab *q;
	register NODE *r;
	register o;
	register m, ml;
	int fldsize, fldoff;
	TWORD fldtype;
	TWORD ty;

	if ((ty = BTYPE(p->in.type)) == ENUMTY || ty == MOETY) {
		/*
		 * Turn enums into ints.  This used to be done in ctype(),
		 * but doing it there affects the symbol table, which
		 * we want to leave alone (e.g., for dbx)
		 */
		enumconvert(p);
	}
	switch( o = p->in.op ){

	case NAME:
		if (BTYPE(p->in.type) == TERROR) {
			/* program error, do not compile further */
			return(p);
		}
		if ((q = STP(p->tn.rval)) == NULL) {
			return(p);
		}
		/*
		 * watch for calls to functions with unpleasant
		 * properties, e.g., setjmp().  If we get one,
		 * we must turn off all optimization for this
		 * routine, including peephole optimization.
		 */
		if (q != NULL && (q->sflags &
		    (S_UNKNOWN_CONTROL_FLOW|S_MAKES_REGS_INCONSISTENT))) {
			safe_opt_level = 0;
		}
		if (twopass) {
			/*
			 * retain symbol table info for IR mapping
			 * but first, change structure parameters to
			 * structure pointer parameters.  Note that:
			 * -- a parameter has structured type only if
			 *    the symbol table says so; beware type puns.
			 * -- the same NAME node may pass through here
			 *    twice, but should only be INCREF'd once
			 *    (i.e., if the node type is STRTY or UNIONTY)
			 */
			if (q->sclass == PARAM
			    && (q->stype == STRTY || q->stype == UNIONTY)
			    && (p->in.type == STRTY || p->in.type == UNIONTY)) {
				p->tn.type = INCREF(p->in.type);
				p = buildtree(UNARY MUL, p, 0);
			}
			return(p);
		}
		switch( q->sclass ){

		case AUTO:
		case PARAM:
			/* fake up a structure reference */
#ifndef VCOUNT
			r = block( REG, NIL, NIL, PTR+STRTY, 0, 0 );
			r->tn.lval = 0;
			r->tn.rval = STKREG;
#endif
			/*
			 * structs-as-parameters are really
			 * struct pointers. So a reference to the
			 * name gets changed to an indirection
			 */
			if ( q->sclass == PARAM ){
				switch( p->tn.type ){
				case  STRTY:
				case  UNIONTY:
#ifndef VCOUNT
					p = stref( block( STREF, r, p, 0, 0, 0 ) );
#endif
					p->in.type = INCREF( p->in.type );
#ifndef VCOUNT
					p->in.left->tn.type = INCREF( p->in.left->tn.type );
#endif
					p = buildtree( UNARY MUL, p, 0 );
					/* want 2-level break here */
					goto endswitch;
				}
			}
#ifndef VCOUNT
			p = stref( block( STREF, r, p, 0, 0, 0 ) );
#endif
			break;

		case ULABEL:
		case LABEL:
		/*...but not case STATIC:, as is usually done here */
			if( q->slevel == 0 ) break;
			p->tn.lval = 0;
			p->tn.rval = -q->offset;
			break;

		case REGISTER:
			p->in.op = REG;
			p->tn.lval = 0;
			p->tn.rval = q->offset;
			/*
			 * structs-as-parameters are really
			 * struct pointers. So a reference to the
			 * name gets changed to an indirection
			 * ( like the above. Should stay parallel. )
			 */
			switch( p->tn.type ){
			case  STRTY:
			case  UNIONTY:
				p->tn.type = INCREF( p->tn.type );
				p = buildtree( UNARY MUL, p, 0 );
			}
			break;

			} /* switch(q->sclass) */
	endswitch:;
		break;

	case PCONV:
		/* do not destroy type info in STASG and STCALL */
		if ( p->in.left->in.op == STASG
		    || p->in.left->in.op == STCALL ) {
			break;
		}
		/* do pointer conversions for char and short */
		ml = p->in.left->in.type;
		if( ( ml==CHAR || ml==UCHAR || ml==SHORT || ml==USHORT ) && p->in.left->in.op != ICON ){
			p->in.op = SCONV;
			break;
		}

		/* pointers all have the same representation; the type is inherited */

	inherit:
		p->in.left->in.type = p->in.type;
		p->in.left->fn.cdim = p->fn.cdim;
		p->in.left->fn.csiz = p->fn.csiz;
		p->in.op = FREE;
		return( p->in.left );

	case SCONV:
		/* now, look for conversions downwards */

		m = p->in.type;
		ml = p->in.left->in.type;
		if( p->in.left->in.op == ICON ){
			/* simulate the conversion here */
			CONSZ val;
			if ((val=p->in.left->tn.rval) != NONAME
			&& dimtab[p->fn.csiz] < SZPOINT ){
			    /*  a relocatable icon is being shortened */
			    /* DO NOT SHORTEN IT!!		      */
			    werror("shortening &%s may lose significance",
				val<0?"(constant)":STP(val)->sname);
			    break;
			}
		shorten_int:
			val = p->in.left->tn.lval;
			switch( m ){
			case CHAR:
				p->in.left->tn.lval = (char) val;
				break;
			case UCHAR:
				p->in.left->tn.lval = (unsigned char) val;
				break;
			case USHORT:
				p->in.left->tn.lval = (unsigned short) val;
				break;
			case SHORT:
				p->in.left->tn.lval = (short)val;
				break;
			case UNSIGNED:
				p->in.left->tn.lval = (unsigned) val;
				break;
			case INT:
				p->in.left->tn.lval = (int)val;
				break;
			case DOUBLE:
			case FLOAT:
				if (ISUNSIGNED(ml)) {
					p->in.left->fpn.dval = (unsigned)val;
				} else {
					p->in.left->fpn.dval = val;
				}
				p->in.left->in.op = FCON;
				break;
			}
			goto inherit;
		} else if( p->in.left->in.op == FCON ){
			/* simulate the conversion here */
			if (m!=DOUBLE && m!=FLOAT){
				/* conversion to int-like things */
				/* convert to int first, then to actual thing */
				p->in.left->tn.lval = (CONSZ)p->in.left->fpn.dval;
				p->in.left->tn.rval = NONAME;
				p->in.left->tn.op = ICON;
				p->in.left->tn.type = INT;
				if (m != INT)
				    goto shorten_int;
				p->in.op = FREE;
				return( p->in.left );
			}
			goto inherit;
		} else if ( p->in.type == TVOID ) {
			/* clobber conversion to void */
			p->in.op = FREE;
			return( p->in.left );
		}
		break;

	case PVCONV:
	case PMCONV:
		if( p->in.right->in.op != ICON ) cerror( "bad conversion", 0);
		p->in.op = FREE;
		return( buildtree( o==PMCONV?MUL:DIV, p->in.left, p->in.right ) );

	case CALL:
	case STCALL:
#ifdef FLOATMATH
		if (FLOATMATH<=1)
		    p->in.right = floatargs_to_double(p->in.right);
		/* FALL THROUGH */
#endif FLOATMATH
	case UNARY CALL:
		if (p->in.type == FLOAT)
#ifdef FLOATMATH
		    if (FLOATMATH<=1)
#endif
			p->in.type = DOUBLE; /* no such thing as a float fn */
		p = p1builtin_call(p);
		break;
	
#ifdef FLOATMATH
	case PLUS:
	case MINUS:
	case DIV:
	case MUL:
	case ASG PLUS:
	case ASG MINUS:
	case ASG DIV:
	case ASG MUL:
		if (floatmath){
		    if (p->in.type == FLOAT && p->in.right->tn.op == FCON)
                        p->in.right->tn.type = p->in.right->fn.csiz = FLOAT;
                }
                break;
#endif FLOATMATH

	case FORCE:
		/* we do not FORCE little things. Ints or better only */
		switch ( p->in.type ){
		case CHAR:
		case SHORT:
			p->in.left = makety(p->in.left,INT,0,INT);
			p->in.type = INT;
			break;
		case UCHAR:
		case USHORT:
			p->in.left = makety(p->in.left,UNSIGNED,0,UNSIGNED);
			p->in.type = UNSIGNED;
			break;
		}
		break;

	} /* switch */
	return(p);
}

andable( p ) NODE *p; 
{
	return(1);  /* all names can have & taken on them */
}

cendarg()
{ /* at the end of the arguments of a ftn, set the automatic offset */
	autooff = AUTOINIT;
}

cisreg( t )
	TWORD t; 
{
	/* is an automatic variable of type t OK for a register variable */
	switch (t) {
	case DOUBLE:
	case FLOAT:
	case INT:
	case UNSIGNED:
	case SHORT:
	case USHORT:
	case CHAR:
	case UCHAR:	
		return(1);
	default:
		return( ISPTR(t) );
	}
}

/*
 * Given an integral constant, what is the smallest type
 * such that the constant's value is invariant under the
 * "usual arithmetic conversions"?  This information is
 * useful for machines that have byte and short compare
 * instructions.  It isn't very useful for those that don't.
 */

TWORD
icontype(u, lval)
	int u;
	CONSZ lval;
{
	if (u) {
		return(UNSIGNED);
	} else {
		return(INT);
	}
}

/*ARGSUSED*/
NODE *
offcon( off, t, d, s ) OFFSZ off; TWORD t; 
{

	/* return a node, for structure references, which is suitable for
	   being added to a pointer of type t, in order to be off bits offset
	   into a structure */

	/* t, d, and s are the type, dimension offset, and sizeoffset */
	/* in general they  are necessary for offcon, but not on H'well */

	return bcon(off/SZCHAR);
}

char *
exname( p ) char *p; 
{
	/* make a name look like an external name in the local machine */

#ifndef FLEXNAMES
	static char text[NCHNAM+1];
#else
	static char text[BUFSIZ+1];
#endif

	register i;

	text[0] = '_';
#ifndef FLEXNAMES
	for( i=1; *p&&i<NCHNAM; ++i ){
#else
	for( i=1; *p; ++i ){
#endif
		text[i] = *p++;
		}

	text[i] = '\0';
#ifndef FLEXNAMES
	text[NCHNAM] = '\0';  /* truncate */
#endif

	return( text );
}

ctype( type )
{ /* map types which are not defined on the local machine */
	switch( BTYPE(type) ){

	case LONG:
		MODTYPE(type,INT);
		break;

	case ULONG:
		MODTYPE(type,UNSIGNED);
		break;

		}
	return( type );
}

noinit( t ) 
{ /* curid is a variable which is defined but
	is not initialized (and not a function );
	This routine returns the stroage class for an uninitialized declaration */

	return(EXTERN);

}

commdec( id )
{ /* make a common declaration for id, if reasonable */
	register struct symtab *q;
	OFFSZ sz, tsize();
	char *name,*place;

#ifndef VCOUNT
	q = STP(id);
	name = exname( q->sname );
	sz = tsize( q->stype, q->dimoff, q->sizoff )/SZCHAR;
	/*
	 * If size is zero, as in "struct _iobuf  _iob[];" treat it
	 * as an external declaration, i.e., force an ld-time message
	 * if no real definition is given.  If size is nonzero, semantics
	 * are those of FORTRAN common.
	 */
	if (sz != 0) {
	    place = (q->sflags&SSMALL)? "sdata" : "data";
	    putprintf("	.common	%s,0x%lx,\"%s\"\n", name, sz, place);
	}
#endif
}

isitlong( cb, ce )
{ /* is lastcon to be long or short */
	/* cb is the first character of the representation, ce the last */

	if( ce == 'l' || ce == 'L' ||
		lastcon >= (1L << (SZINT-1) ) ) return (1);
	return(0);
}


isitfloat( s ) char *s; 
{
	double atof();
	dcon = atof(s);
	return( FCON );
}

#ifndef ONEPASS
tlen(p) NODE *p; 
{
	switch(p->in.type) {
		case CHAR:
		case UCHAR:
			return(1);
			
		case SHORT:
		case USHORT:
			return(2);
			
		case DOUBLE:
			return(8);
			
		default:
			return(4);
		}
}
#endif


fixdef( p )
	register struct symtab *p;
{
	extern int xdebug;
#ifdef SMALL_DATA
	if (p->slevel == 0 || p->sclass == STATIC || p->sclass == EXTERN ){
	    if ( !xdebug && !ISARY(p->stype) && !ISFTN(p->stype))
		p->sflags |= SSMALL;
	}
#endif SMALL_DATA
	outstab(p);
}

NAMEHACK( p )
	register NODE *p;
{
	if (/* p->tn.rval <= 0 || */
	(p->tn.rval != NONAME && p->tn.rval >= 0 && (STP(p->tn.rval)->sflags&SSMALL))){
#ifdef ONEPASS
		return 1;
#else
		printf("1\t");
#endif
	} else {
#ifdef ONEPASS
		return 0;
#else
		printf("0\t");
#endif
	}
}

#ifndef ONEPASS
PRNAME( p )
	register NODE *p;
{
	register struct symtab *q;
	if( p->tn.rval == NONAME ) {
		;
	} else if( p->tn.rval >= 0 ){
		q = STP(p->tn.rval);
		if ( q->sclass == STATIC && q->slevel >0){
			printf(LABFMT, q->offset);
		} else {
			printf("%s", exname(q->sname) );
		}
	} else { /* label */
		printf( LABFMT, -p->tn.rval );
	}
	putchar('\n');
}
#endif ONEPASS

allocate_static( s, sz )
	struct symtab *s;
	int sz;
{
	int  curloc;
	char label[20];

#ifndef VCOUNT
	/*
	 * we cannot use reserve because it does not
	 * let us force alignment.
	 */
#ifdef SMALL_DATA
	curloc = locctr((s->sflags&SSMALL)? DATA : BSS);
#else 
	curloc = locctr(BSS);
#endif
	defalign(talign(s->stype, s->sizoff));
	if ( s->slevel >1){
		sprintf(label, LABFMT, s->offset);
		putprintf("%s:	.skip	%d\n", label, sz);
	} else {
		putprintf("%s:	.skip	%d\n", exname(s->sname), sz);
	}
	(void)locctr(curloc);
#endif
}

/*
 * emit an .ident "string" directive to the assembler,
 * given a STRING node with the string pointer stashed
 * in the tn.lval field.
 */

void
module_ident(p)
	NODE *p;
{
	if (p == NULL || p->in.op != STRING) {
		werror("string expected");
	} else {
		putprintf("	.ident	\"%s\"\n", (char*)p->tn.lval);
	}
}

/*
 * p1builtin_va_arg_incr(p): given a call to
 *	__builtin_va_arg_incr(p);
 * where p is a pointer of some type, rewrite the call as an INCR
 * expression, incrementing p by the size of the referenced object,
 * and adjusting for the idiosyncracies of the sparc calling sequence.
 */
static NODE *
p1builtin_va_arg_incr(p)
	NODE *p;
{
	NODE *argp;	/* argument of __builtin_va_arg_incr() */
	NODE *incnode;	/* for generating INCR tree */
	TWORD t;	/* argument type */
	int d, s;	/* dimoff, sizoff components of type descriptor */
	int incrsize;	/* size of argument, rounded to multiple of SZINT */

	argp = p->in.right;
	if (argp == NULL) {
		uerror("missing required argument to va_arg");
		return(p);
	}
	if (notlval(argp)) {
		uerror("illegal argument to va_arg");
		return(p);
	}
	t = argp->in.type;
	d = argp->fn.cdim;
	s = argp->fn.csiz;
	if (!ISPTR(t)) {
		uerror("argument to __builtin_va_arg_incr() must be pointer");
		return(p);
	}
	switch(DECREF(t)) {
	case STRTY:
	case UNIONTY:
		/*
		 * On SPARC, there is an extra level of indirection
		 * for structured arguments.  So we return a tree of
		 * the form:
		 *	U* PTR strty
		 *	    INCR PTR PTR strty
		 *		(argp) PTR PTR strty
		 *		ICON #sizeof(argp) int
		 */
		t = INCREF(t);
		argp->in.type = t;
		incnode = block(INCR, pcc_tcopy(argp),
				bcon(tsize(t,d,s)/SZCHAR), t, d, s);	
		t = DECREF(t);
		incnode = block(UNARY MUL, incnode, NIL, t, d, s);
		break;
	default:
		/*
		 * Objects smaller than an int (including tiny structs) are
		 * right-justified.  So there are two cases:
		 *
		 *	small:  [advance by a multiple of sizeof(int), back up]
		 *	    MINUS PTR t
		 *		INCR PTR int
		 *		    (argp) PTR int
		 *		    ICON #sizeof(int)
		 *		ICON #sizeof(t)
		 *
		 *	big:  [advance by a multiple of sizeof(short)]
		 *	    INCR PTR strty
		 *		(argp) PTR strty
		 *		ICON #sizeof(argp) int
		 */
		incrsize = tsize(DECREF(t),d,s);
		if (incrsize < SZINT) {
			incnode = block(ASG PLUS, pcc_tcopy(argp),
				bcon(SZINT/SZCHAR), INCREF(INT), 0, INT);
			incnode = block(MINUS, incnode,
				bcon(incrsize/SZCHAR), t, d, s);
		} else {
			incnode = block(INCR, pcc_tcopy(argp),
				bcon(incrsize/SZCHAR), t, d, s);
		}
		break;
	}
	pcc_tfree(p);
	return (incnode);
}
