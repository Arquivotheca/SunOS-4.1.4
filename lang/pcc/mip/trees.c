#ifndef lint
static	char sccsid[] = "@(#)trees.c 1.1 94/10/31 SMI"; /* from S5R2 1.4 */
#endif

# if TARGET == I386
# include "mfile1.h"
# else
# include "cpass1.h"
# endif
# include "messages.h"

# ifdef BROWSER
# include <cb_sun_c_tags.h>
# include <cb_ccom.h>
# include <cb_enter.h>
# endif

	    /* corrections when in violation of lint */

/*
 * Guy's changes to merge with system V lint (12 Apr 88):
 *
 * The main differences pertain to the error messsage printing.  Some
 * minor "lint" cleanup was done (casts, eliminating unused variables,
 * etc.).  Some minor "lint" changes from S5 were introduced (warning that
 * "pointer casts may be troublesome" if the "-p" flag is passed to the S5
 * version, some fixes to eliminate bogus "statement not reached"
 * messages).  I also got rid of some #ifdef FLEXNAMES code, since I don't
 * think we care about supporting non-FLEXNAMES systems.
 */

/*	some special actions, used in finding the type of nodes */
# define NCVT 01
# define PUN 02
# define TYPL 04
# define TYPR 010
# define TYMATCH 040
# define LVAL 0100
# define CVTO 0200
# define CVTL 0400
# define CVTR 01000
# define PTMATCH 02000
# define OTHER 04000
# define NCVTR 010000
# define RINT 020000
# define ELINT 040000	/* not to be confused with ifdef LINT ... */
# define UCONV 0100000
# define MISMATCH 0200000

/*
 * Avoid spitting out type-mismatch messages about type TERROR
 */
#define TYPEO( type, message ) {if (BTYPE(type)!= TERROR) UERROR(message);}
#define TYPEO2( type1, type2, message ) {if (BTYPE(type1)!= TERROR && BTYPE(type2)!= TERROR) UERROR(message);}

/* node conventions:

	NAME:	rval>0 is stab index for external
		rval<0 is -inlabel number
		lval is offset in bits
	ICON:	lval has the value
		rval has the STAB index, or - label number,
			if a name whose address is in the constant
		rval = NONAME means no name
	REG:	rval is reg. identification cookie

	*/

int bdebug = 0;
extern ddebug;
extern int twopass;

NODE * uconv();

NODE *
buildtree( o, l, r ) register NODE *l, *r; {
	register NODE *p, *q;
	register actions;
	register opty;
	register struct symtab *sp;
	register NODE *lr, *ll;
	extern int eprint();

# ifndef BUG1
	if( bdebug ) printf( "buildtree( %s, 0x%x, 0x%x )\n", opst[o], l, r );
# endif
	opty = optype(o);
	switch(opty) {
	case BITYPE:
		if (r == NULL) {
			cerror("missing right operand of op '%s'", opst[o]);
			}
		/*FALL THROUGH*/
	case UTYPE:
		if (l == NULL) {
			cerror("missing left operand of op '%s'", opst[o]);
			}
		break;
		}

	/* check for constants */

	if( opty == UTYPE && l->in.op == ICON ){
		switch( o ){
		case NOT:
			/* "constant argument to NOT"  */
			if( hflag ) WERROR( MESSAGE( 22 ) );
		case UNARY MINUS:
		case COMPL:
			if( conval( l, o, l, 0 ) ) return(l);
			break;
			}
		}

	else if( o==UNARY MINUS && l->in.op==FCON ){
		l->fpn.dval = -l->fpn.dval;
		return(l);
		}

	else if( o==QUEST && l->in.op==ICON ) {
		l->in.op = FREE;
		r->in.op = FREE;
		if( l->tn.lval ){
			tfree( r->in.right );
			return( r->in.left );
			}
		else {
			tfree( r->in.left );
			return( r->in.right );
			}
		}

	else if( (o==ANDAND || o==OROR) && (l->in.op==ICON||r->in.op==ICON) ) goto ccwarn;

	else if( opty == BITYPE && l->in.op == ICON && r->in.op == ICON ){
		switch( o ){
		case ULT:
		case UGT:
		case ULE:
		case UGE:
		case LT:
		case GT:
		case LE:
		case GE:
		case EQ:
		case NE:
		case ANDAND:
		case OROR:
		case CBRANCH:
		ccwarn:
			/* "constant in conditional context" */
			if( hflag ) WERROR( MESSAGE( 24 ) );

		case PLUS:
		case MINUS:
		case MUL:
		case DIV:
		case MOD:
		case AND:
		case OR:
		case ER:
		case LS:
		case RS:
			if( conval( l, o, r, 0 ) ) {
				r->in.op = FREE;
				return(l);
				}
			break;
			}
		}
	else if( opty == BITYPE && (l->in.op==FCON||l->in.op==ICON) &&
		(r->in.op==FCON||r->in.op==ICON) ){
		switch(o){
		case PLUS:
		case MINUS:
		case MUL:
		case DIV:
			if( l->in.op == ICON ){
				l->fpn.dval = l->tn.lval;
				}
			if( r->in.op == ICON ){
				r->fpn.dval = r->tn.lval;
				}
			l->in.op = FCON;
			l->in.type = l->fn.csiz = DOUBLE;
			r->in.op = FREE;
			switch(o){
			case PLUS:
				l->fpn.dval += r->fpn.dval;
				return(l);
			case MINUS:
				l->fpn.dval -= r->fpn.dval;
				return(l);
			case MUL:
				l->fpn.dval *= r->fpn.dval;
				return(l);
			case DIV:
				/* "division by 0." */
				if( r->fpn.dval == 0 ) UERROR( MESSAGE( 32 ) );
				else l->fpn.dval /= r->fpn.dval;
				return(l);
				}
			}
		}

	/* its real; we must make a new node */

	p = block( o, l, r, INT, 0, INT );

	actions = opact(p);

	if( actions&LVAL ){ /* check left descendent */
		if( notlval(p->in.left) ) {
			/* "illegal lhs of assignment operator" */
			TYPEO(p->in.left->in.type, MESSAGE( 62 ) );
			}
		}

	if( actions & NCVTR ){
		p->in.left = pconvert( p->in.left );
		}
	else if( actions & MISMATCH ){
		/* don't default type of p, avoid further type complaints */
		p->in.type = p->fn.cdim = p->fn.csiz = TERROR;
		}
	else if( !(actions & NCVT ) ){
		switch( opty ){

		case BITYPE:
			p->in.right = pconvert( p->in.right );
		case UTYPE:
			p->in.left = pconvert( p->in.left );

			}
		}

	if( (actions&PUN) && (o!=CAST||cflag) ){
		chkpun(p);
		}

#ifndef VAX
	if (actions & RINT)
		p->in.right = makety(p->in.right, INT, 0, INT );
	if (actions & ELINT)
		p->in.left = makety(p->in.left, INT, 0, INT );
#endif

	if( actions & (TYPL|TYPR) ){

		q = (actions&TYPL) ? p->in.left : p->in.right;

		p->in.type = q->in.type;
		p->fn.cdim = q->fn.cdim;
		p->fn.csiz = q->fn.csiz;
		}

	if( actions & CVTL ) p = convert( p, CVTL );
	if( actions & CVTR ) p = convert( p, CVTR );
	if( actions & TYMATCH ) p = tymatch(p);
	if( actions & PTMATCH ) p = ptmatch(p);
	if( actions & UCONV ) p = uconv(p);

	if( actions & OTHER ){
		l = p->in.left;
		r = p->in.right;

		switch(o){

		case NAME:
			sp = STP(idname);
			if (sp == NULL) {
				cerror("buildtree: null symbol");
				}
			if( sp->stype == UNDEF ){
				/* "%s undefined" */
				UERROR( MESSAGE( 4 ), sp->sname );
				/* make p look reasonable */
				p->in.type = p->fn.cdim = p->fn.csiz = TERROR;
				p->tn.rval = idname;
				p->tn.lval = 0;
				defid( p, SNULL );
				break;
				}
			p->in.type = sp->stype;
			p->fn.cdim = sp->dimoff;
			p->fn.csiz = sp->sizoff;
			p->tn.lval = 0;
			p->tn.rval = idname;
			/* special case: MOETY is really an ICON... */
			if( p->in.type == MOETY ){
				p->tn.rval = NONAME;
				p->tn.lval = sp->offset;
				p->fn.cdim = 0;
				p->in.type = ctype(ENUMTY);
				p->in.op = ICON;
				}
			break;

		case ICON:
#ifdef VAX
			p->in.type = INT;
#endif
			p->fn.cdim = 0;
			p->fn.csiz = INT;
			break;

		case STRING:
			p->in.op = NAME;
			p->in.type = CHAR+ARY;
			p->tn.lval = 0;
			p->tn.rval = NOLAB;
			p->fn.cdim = curdim;
			p->fn.csiz = CHAR;
			break;

		case FCON:
			p->tn.lval = 0;
			p->tn.rval = 0;
			p->in.type = DOUBLE;
			p->fn.csiz = DOUBLE;
			p->fn.cdim = 0;
			break;

		case STREF:
			/* p->x turned into *(p+offset) */
			/* rhs must be a name; check correctness */
			sp = STP(r->tn.rval);
			if( sp == NULL || (sp->sclass != MOS
			    && sp->sclass != MOU && !(sp->sclass&FIELD)) ){
			        if ( sp != NULL ){
				    /* "member of structure or union required" */
				    TYPEO( sp->stype, MESSAGE( 76 ) );
				} else
				    uerror( "member of structure or union required" );
			} else if( sp->sflags & SNONUNIQ
			    && (l->in.type==PTR+STRTY || l->in.type == PTR+UNIONTY)
			    && (l->fn.csiz +1) >= 0 ){
				/*
				 * nonunique name && structure defined
				 * find the right one
				 */
				char * memnam, * tabnam;
				int j;
				register struct symtab *mp;
#ifdef BROWSER
				Cb_semtab_if sem = NULL;
#endif

				j=dimtab[l->fn.csiz+1];
				mp = STP(dimtab[j]);
				while (mp != NULL) {
					tabnam = mp->sname;
					memnam = sp->sname;
# ifndef BUG1
					if( ddebug>1 ){
						printf("member %s==%s?\n",
							memnam, tabnam);
						}
# endif
					if( mp->sflags & SNONUNIQ ){
						if ( memnam == tabnam ) {
							sp = mp;
							r->tn.rval = (int)sp;
							break;
							}
						}
					j++;
					mp = STP(dimtab[j]);
					}
#ifdef BROWSER
				if (mp != NULL) {
				switch (mp->sclass) {
				case MOU:
					cb_enter_name_mark(mp,
							   cb_ccom_line_no,
							   cb_c_ref_union_field_read,
							   sem);
					break;
				case MOS:
					cb_enter_name_mark(mp,
							   cb_ccom_line_no,
							   cb_c_ref_struct_field_read,
							   sem);
					break;
			        default: /* MOS & bit field */
					cb_enter_name_mark(mp,
							   cb_ccom_line_no,
							   cb_c_ref_struct_bitfield_read,
							   sem);
					break;
				}
				if (sem != NULL) {
					if (PCC_ISARY(mp->stype)) {
						cb_ccom_expr = cb_ccom_alloc(cb_binary,
									     cb_ccom_expr,
									     cb_ccom_alloc(cb_name_vector, sem));
					} else {
						cb_ccom_expr = cb_ccom_alloc(cb_binary,
									     cb_ccom_expr,
									     cb_ccom_alloc(cb_name, sem));
					}
				} else {
					cb_ccom_expr = NULL;
				}
			}
#endif
				if( mp == NULL )
					/* "illegal member use: %s" */
					UERROR(MESSAGE( 63 ), sp->sname);
				}
			else {
				register j;
				if( l->in.type != PTR+STRTY
				    && l->in.type != PTR+UNIONTY ){
					if (BTYPE(l->in.type) != TERROR)
						if( sp->sflags & SNONUNIQ ){
							/* "nonunique name demands struct/union or struct/union pointer"  */
							UERROR( MESSAGE( 84 ) );
							}
						else {
							/* "struct/union or struct/union pointer required" */
							WERROR(MESSAGE( 103 ));
							}
#ifdef BROWSER
					if (cb_flag &&
					    (cb_ccom_line_no>0) &&
					    (sp->sname != NULL)) {
						cb_enter_symbol(sp->sname,
								cb_ccom_line_no,
								cb_c_ref_struct_field_read);
					}
#endif
					}
				else if( (j=l->fn.csiz+1)<0 ) {
					cerror( "undefined structure or union" );
#ifdef BROWSER
					if (cb_flag &&
					    (cb_ccom_line_no>0) &&
					    (sp->sname != NULL)) {
						cb_enter_symbol(sp->sname,
								cb_ccom_line_no,
								cb_c_ref_struct_field_read);
					}
#endif
				} else if( !chkstr( sp, dimtab[j], DECREF(l->in.type) ) ){
					/* "illegal member use: %s" */
					WERROR( MESSAGE( 63 ), sp->sname );
					}
				}

			p = stref( p );
			break;

		case UNARY MUL:
			if( l->in.op == UNARY AND ){
				p->in.op = l->in.op = FREE;
				p = l->in.left;
				}
			/* "illegal indirection" */
			if( !ISPTR(l->in.type))TYPEO(l->in.type,MESSAGE( 60 ));
			p->in.type = DECREF(l->in.type);
			p->fn.cdim = l->fn.cdim;
			p->fn.csiz = l->fn.csiz;
			break;

		case UNARY AND:
			switch( l->in.op ){

			case UNARY MUL:
				p->in.op = l->in.op = FREE;
				p = l->in.left;
			case NAME:
#if TARGET == I386 && !defined(LINT)
			case VAUTO:
			case VPARAM:
			case TEMP:
#endif
				p->in.type = INCREF( l->in.type );
				p->fn.cdim = l->fn.cdim;
				p->fn.csiz = l->fn.csiz;
#if VCOUNT
				amper_of( l->tn.rval );
#endif
				break;

			case COMOP:
				lr = buildtree( UNARY AND, l->in.right, NIL );
				p->in.op = l->in.op = FREE;
				p = buildtree( COMOP, l->in.left, lr );
				break;

			case QUEST:
				lr = buildtree( UNARY AND, l->in.right->in.right, NIL );
				ll = buildtree( UNARY AND, l->in.right->in.left, NIL );
				p->in.op = l->in.op = l->in.right->in.op = FREE;
				p = buildtree( QUEST, l->in.left, buildtree( COLON, ll, lr ) );
				break;

# ifdef ADDROREG
			case OREG:
				/* OREG was built in clocal()
				 * for an auto or formal parameter
				 * now its address is being taken
				 * local code must unwind it
				 * back to PLUS/MINUS REG ICON
				 * according to local conventions
				 */
				{
				extern NODE * addroreg();
				p->in.op = FREE;
				p = addroreg( l );
				}
				break;

# endif
			default:
				/* "unacceptable operand of &" */
				UERROR( MESSAGE( 110 ) );
				/* try to recover */
				p->in.op = FREE;
				p = l;
				p->in.type = p->fn.cdim = p->fn.csiz = TERROR;
				break;
				}
			break;

		case LS:
		case RS:
		case ASG LS:
		case ASG RS:
			if(tsize(p->in.right->in.type, p->in.right->fn.cdim, p->in.right->fn.csiz) > SZINT)
				p->in.right = makety(p->in.right, INT, 0, INT );
			break;

		case RETURN:
		case ASSIGN:
		case CAST:
			/* structure assignment */
			/* take the addresses of the two sides; then make an
			/* operator using STASG and
			/* the addresses of left and right */

			{
				register TWORD t;
				register d, s;

				/* "assignment of different structures" */
				if( l->fn.csiz != r->fn.csiz ) TYPEO2( l->in.type, r->fn.type, MESSAGE( 15 ) );

				r = buildtree( UNARY AND, r, NIL );
				if ( o == ASSIGN )
				    l = buildtree( UNARY AND, l, NIL );
				t = r->in.type;
				d = r->fn.cdim;
				s = r->fn.csiz;

				l = block( STASG, l, r, t, d, s );

				if( o == RETURN ){
					p->in.op = FREE;
					p = l;
					break;
					}

				l = clocal( l );
				p->in.op = UNARY MUL;
				p->in.left = l;
				p->in.right = NIL;
				break;
				}
		case COLON:
			/* structure colon */

			/* "type clash in conditional" */
			if( l->fn.csiz != r->fn.csiz ) TYPEO2( l->in.type, r->fn.type, MESSAGE( 109 ) );
			break;

		case CALL:
			p->in.right = r = strargs( p->in.right );
		case UNARY CALL:
#if VCOUNT
			does_call();
#endif
			/* "illegal function" */
			if( !ISPTR(l->in.type)) TYPEO(l->in.type, MESSAGE( 58 ));
			p->in.type = DECREF(l->in.type);
			/* "illegal function" */
			if( !ISFTN(p->in.type)) TYPEO(p->in.type,MESSAGE( 58 ));
			p->in.type = DECREF( p->in.type );
			p->fn.cdim = l->fn.cdim;
			p->fn.csiz = l->fn.csiz;
			if( l->in.op == UNARY AND && l->in.left->in.op == NAME
			    && (sp = STP(l->in.left->tn.rval)) != NULL
			    && (sp->sclass == FORTRAN || sp->sclass ==UFORTRAN) ){
				p->in.op += (FORTCALL-CALL);
				}
			if( p->in.type == STRTY || p->in.type == UNIONTY ){
				/* function returning structure */
				/*  make function really return ptr to str., with * */

				p->in.op += STCALL-CALL;
				p->in.type = INCREF( p->in.type );
				p = buildtree( UNARY MUL, clocal(p), NIL );

				}
			break;

		default:
			cerror( "other: operator %s", pcc_opname(o) );
			}

		}

	if( actions & CVTO ) p = oconvert(p);
	p = clocal(p);

# ifndef BUG1
	if( bdebug ) fwalk( p, eprint, 0 );
# endif

	return(p);

	}

e1pr(p)
	NODE *p;
{
	fwalk(p, eprint, 0);
}

/*
 * this is a hack to detect things like &f(x) and &(x = y),
 * where f is a structure-valued function.  This has to be
 * checked in the grammar, since UNARY AND gets inserted
 * in many valid constructs, e.g., x = y = z, where x,y,z
 * are structures.
 */
hidden_strop( p )
	register NODE *p;
{
	if ( p->in.op == UNARY MUL ) {
		p = p->in.left;
		while (p->in.op == SCONV || p->in.op == PCONV) {
			p = p->in.left;
		}
		switch (p->in.op) {
		case STCALL:
		case UNARY STCALL:
		case STASG:
		case STARG:
			return(1);
		default:
			break;
		}
	}
	return(0);
}

NODE *
strargs( p ) register NODE *p;  { /* rewrite structure flavored arguments */

	if( p->in.op == CM ){
		p->in.left = strargs( p->in.left );
		p->in.right = strargs( p->in.right );
		return( p );
		}

	if( p->in.type == STRTY || p->in.type == UNIONTY ){
		p = block( STARG, p, NIL, p->in.type, p->fn.cdim, p->fn.csiz );
		p->in.left = buildtree( UNARY AND, p->in.left, NIL );
		p = clocal(p);
		}
#ifndef VAX
	else if (p->in.type==CHAR || p->in.type==SHORT)
		p = makety(p, INT, 0, INT);
	else if (p->in.type==UCHAR || p->in.type==USHORT) 
		p = makety(p, UNSIGNED, 0, UNSIGNED);
#endif
	return( p );
	}

chkstr( sp, j, type )
	register struct symtab *sp;
	register j;
	TWORD type;
{
	/* is the MOS or MOU at stab[i] OK for strict reference by a ptr */
	/* *sp has been checked to contain a MOS or MOU */
	/* j is the index in dimtab of the members... */
	register struct symtab *mp;
	extern int ddebug;

# ifndef BUG1
	if( ddebug > 1 )
		printf( "chkstr( %s(0x%x), %d )\n",
			sp == NULL ? "???" : sp->sname, sp, j );
# endif
	/* "undefined structure or union" */
	if( j < 0 ) UERROR( MESSAGE( 112 ) );
	else {
		mp = STP(dimtab[j]);
		while (mp != NULL) {
#ifdef BROWSER
			if( mp == sp ) {
				Cb_semtab_if sem = NULL;

				switch (mp->sclass) {
				case MOU:
					cb_enter_name_mark(mp,
							   cb_ccom_line_no,
							   cb_c_ref_union_field_read,
							   sem);
					break;
				case MOS:
					cb_enter_name_mark(mp,
							   cb_ccom_line_no,
							   cb_c_ref_struct_field_read,
							   sem);
					break;
			        default: /* MOS & bit field */
					cb_enter_name_mark(mp,
							   cb_ccom_line_no,
							   cb_c_ref_struct_bitfield_read,
							   sem);
					break;
				}
				if (sem != NULL) {
					if (PCC_ISARY(mp->stype)) {
						cb_ccom_expr = cb_ccom_alloc(cb_binary,
									     cb_ccom_expr,
									     cb_ccom_alloc(cb_name_vector, sem));
					} else {
						cb_ccom_expr = cb_ccom_alloc(cb_binary,
									     cb_ccom_expr,
									     cb_ccom_alloc(cb_name, sem));
					}
				} else {
					cb_ccom_expr = NULL;
				}

				return( 1 );
			}
#else
			if( mp == sp ) return( 1 );
#endif
			switch( mp->stype ){

			case STRTY:
			case UNIONTY:
				if( type == STRTY )
					break;  /* no recursive looking for strs */
				if( hflag && chkstr( sp, dimtab[mp->sizoff+1], mp->stype ) ) {
					if( mp->sname[0] == '$' )
						return(0);  /* $FAKE */
					/* "illegal member use: perhaps %s.%s?" */
					WERROR( MESSAGE( 65 ),
						mp->sname, sp->sname );
					return(1);
					}
				}
			j++;
			mp = STP(dimtab[j]);
			}
		}
	return( 0 );
	}

conval( p, o, q, ptok ) register NODE *p, *q; {
	/* apply the op o to the lval part of p; if binary, rhs is val */
	int i, u;
	CONSZ val;
	NODE fake;
	static NODE zero;
	int newtype;

	val = q->tn.lval;
	u = ISUNSIGNED(p->in.type) || ISUNSIGNED(q->in.type);
	if( u && (o==LE||o==LT||o==GE||o==GT)) o += (UGE-GE);

	if( !ptok &&(ISPTR(p->in.type) || ISPTR(q->in.type))) return(0);
	if( p->tn.rval != NONAME && q->tn.rval != NONAME ) return(0);
	if( q->tn.rval != NONAME && o!=PLUS ) return(0);
	if( p->tn.rval != NONAME && o!=PLUS && o!=MINUS ) return(0);
	/*
	 * make sure the "usual arithmetic conversions" get done
	 * if necessary.  Make sure they *don't* get done if
	 * they aren't supposed to.
	 */
	if(optype(o) == BITYPE && !ISPTR(p->in.type)
	    && !ISPTR(q->in.type)) {
		fake = zero;
		fake.in.op = o;
		fake.in.left = p;
		fake.in.right = q;
		tymatch( &fake );
		newtype = 1;
	} else {
		newtype = 0;
	}

	switch( o ){

	case PLUS:
		p->tn.lval += val;
		/*
		 * p inherits name, if any, from q
		 */
		if( p->tn.rval == NONAME ){
			p->tn.rval = q->tn.rval;
			/*p->in.type = q->in.type;*/
			}
		/*
		 * Make sure ptr+int combinations have pointer type
		 */
		if( ISPTR(q->in.type) && !ISPTR(p->in.type) ) {
			p->in.type = q->in.type;
			p->fn.cdim = q->fn.cdim;
			p->fn.csiz = q->fn.csiz;
			}
		break;
	case MINUS:
		p->tn.lval -= val;
		/*
		 * Make sure ptr+int combinations have pointer type
		 */
		if( ISPTR(q->in.type) && !ISPTR(p->in.type) ) {
			p->in.type = q->in.type;
			p->fn.cdim = q->fn.cdim;
			p->fn.csiz = q->fn.csiz;
			}
		break;
	case MUL:
		p->tn.lval *= val;
		break;
	case DIV:
		/* "division by 0" */
		if( val == 0 ) UERROR( MESSAGE( 31 ) );
		else if (u) p->tn.lval /= (unsigned)val;
		else        p->tn.lval /= val;
		break;
	case MOD:
		/* "division by 0" */
		if( val == 0 ) UERROR( MESSAGE( 31 ) );
		else if (u) p->tn.lval %= (unsigned)val;
		else        p->tn.lval %= val;
		break;
	case AND:
		p->tn.lval &= val;
		break;
	case OR:
		p->tn.lval |= val;
		break;
	case ER:
		p->tn.lval ^=  val;
		break;
	case LS:
		i = val;
		if (u) p->tn.lval = (unsigned)p->tn.lval << i;
		else   p->tn.lval = p->tn.lval << i;
		break;
	case RS:
		i = val;
		if (u) p->tn.lval = (unsigned)p->tn.lval >> i;
		else   p->tn.lval = p->tn.lval >> i;
		break;

	case UNARY MINUS:
		p->tn.lval = - p->tn.lval;
		break;
	case COMPL:
		p->tn.lval = ~p->tn.lval;
		break;
	case NOT:
		p->tn.lval = !p->tn.lval;
		break;
	case LT:
		p->tn.lval = p->tn.lval < val;
		break;
	case LE:
		p->tn.lval = p->tn.lval <= val;
		break;
	case GT:
		p->tn.lval = p->tn.lval > val;
		break;
	case GE:
		p->tn.lval = p->tn.lval >= val;
		break;
	case ULT:
		p->tn.lval = p->tn.lval < (unsigned)val;
		break;
	case ULE:
		p->tn.lval = p->tn.lval <= (unsigned)val;
		break;
	case UGE:
		p->tn.lval = p->tn.lval >= (unsigned)val;
		break;
	case UGT:
		p->tn.lval = p->tn.lval > (unsigned)val;
		break;
	case EQ:
		p->tn.lval = p->tn.lval == val;
		break;
	case NE:
		p->tn.lval = p->tn.lval != val;
		break;
	default:
		return(0);
		}
	if (newtype) {
		p->tn.type = fake.in.type;
		p->fn.cdim = fake.fn.cdim;
		p->fn.csiz = fake.fn.csiz;
		}
	return(1);
	}

chkpun(p) register NODE *p; {

	/* checks p for the existance of a pun */

	/* this is called when the op of p is ASSIGN, RETURN, CAST, COLON, or relational */

	/* one case is when enumerations are used: this applies only to lint */
	/* in the other case, one operand is a pointer, the other integer type */
	/* we check that this integer is in fact a constant zero... */

	/* in the case of ASSIGN, any assignment of pointer to integer is illegal */
	/* this falls out, because the LHS is never 0 */

	register NODE *q;
	register t1, t2;
	register d1, d2;

	t1 = p->in.left->in.type;
	t2 = p->in.right->in.type;

	if (BTYPE(t1) == TERROR || BTYPE(t2) == TERROR) return;

	if( t1==ENUMTY || t2==ENUMTY ) { /* check for enumerations */
		if( logop( p->in.op ) && p->in.op != EQ && p->in.op != NE ) {
			/* "illegal comparison of enums" */
			UERROR( MESSAGE( 54 ) );
			return;
			}
		if( t1==ENUMTY && t2==ENUMTY
		    && p->in.left->fn.csiz==p->in.right->fn.csiz )
			return;
		if( t1==STRTY || t2==STRTY || t1==UNIONTY || t2==UNIONTY ) {
			/* "operands of %s have incompatible types" */
			UERROR( MESSAGE( 89 ), opst[p->in.op] );
			}
		else {
			/* "enumeration type clash, operator %s" */
			WERROR( MESSAGE( 37 ), opst[p->in.op] );
			}
		return;
		}

	if( ISPTR(t1) || ISARY(t1) ) q = p->in.right;
	else q = p->in.left;

	if( !ISPTR(q->in.type) && !ISARY(q->in.type) ){
		if( q->in.op != ICON || q->tn.lval != 0 ){
			/* "illegal combination of pointer and integer, op %s" */
			WERROR( MESSAGE( 53 ), opst[p->in.op] );
			}
		}
	else {
		/*
		 * type is "ptr to (...)" or "array of (...)"; walk through
		 * the type constructors in the TWORD and check dimensions
		 * of any array parts.  They have to be the same in order
		 * for the types to be equivalent.
		 *
		 * NOTE: this code assumes that a reference to an item of
		 * array type has already mapped into a value of pointer
		 * type.  Also, note that within a composite type constructor,
		 * PTR and ARY are NOT equivalent.
		 */
		if( t1 == t2 ) {
			if( p->in.left->fn.csiz != p->in.right->fn.csiz ) {
				/* "illegal structure pointer combination" */
				WERROR( MESSAGE( 69 ) );
				}
			d1 = p->in.left->fn.cdim;
			d2 = p->in.right->fn.cdim;
			for( ;; ){
				if( ISARY(t1) ){
					if( dimtab[d1] != dimtab[d2] ){
						/* "illegal array size combination" */
						WERROR( MESSAGE( 49 ) );
						return;
					}
					++d1;
					++d2;
				}
				else if( !ISPTR(t1) ) break;
				t1 = DECREF(t1);
			}
		} else {
			/*
			 * "pointer to void" can be converted to or from
			 * with impunity (unless "-q" was used in "lint").
			 * XXX - should this be checked at all, even if "-q"
			 * isn't used, if "-p" isn't used?  Should it be
			 * checked even if "-p" *is* used?  Some way of telling
			 * the system "I know what I'm doing, this pointer will
			 * be correct" would be useful, and using "void *"
			 * might be the ticket.
			 */
#ifdef LINT
			if (!qflag || !TVOIDPTR(t1) && !TVOIDPTR(t2) ) {
				/* changes 10/23/80 - complain about pointer casts if cflag
				 *	is set (this implies pflag is also set)
				 */
				if ( p->in.op == CAST ){ /* this implies cflag is set */
					/* pointer casts may be troublesome */
					WERROR( MESSAGE( 98 ) );
					return;
					}
				/* "illegal pointer combination" */
				WERROR( MESSAGE( 66 ) );
			}
#else
			if ( !TVOIDPTR(t1) && !TVOIDPTR(t2) ) {
				/* "illegal pointer combination" */
				WERROR( MESSAGE( 66 ) );
			}
#endif
		}
	}
}

#ifdef VAX
NODE *
stref( p ) register NODE *p; {

	TWORD t;
	int d, s, dsc, align;
	OFFSZ off;
	register struct symtab *q;

	/* make p->x */
	/* this is also used to reference automatic variables */

	q = STP(p->in.right->tn.rval);
	if (q == NULL)
		cerror("null symbol in stref()");
	p->in.right->in.op = FREE;
	p->in.op = FREE;
	p = pconvert( p->in.left );

	/* make p look like ptr to x */

	if( !ISPTR(p->in.type)){
		p->in.type = PTR+UNIONTY;
		}

	t = INCREF( q->stype );
	d = q->dimoff;
	s = q->sizoff;

	p = makety( p, t, d, s );

	/* compute the offset to be added */

	off = q->offset;
	dsc = q->sclass;

	if( dsc & FIELD ) {  /* normalize offset */
		align = ALINT;
		s = INT;
		off = (off/align)*align;
		}
	if( off != 0 ) p = clocal( block( PLUS, p, offcon( off, t, d, s ), t, d, s ) );

	p = buildtree( UNARY MUL, p, NIL );

	/* if field, build field info */

	if( dsc & FIELD ){
		p = block( FLD, p, NIL, q->stype, 0, q->sizoff );
		p->tn.rval = PKFIELD( dsc&FLDSIZ, q->offset%align );
		}

	return( clocal(p) );
	}
#else

NODE *
stref( p ) register NODE *p; {

	TWORD t;
	int d, s, dsc, align;
	OFFSZ off;
	register struct symtab *q;

	/* make p->x */
	/* this is also used to reference automatic variables */

	q = STP(p->in.right->tn.rval);
	if (q == NULL)
		cerror("null symbol in stref()");
	p->in.right->in.op = FREE;
	p->in.op = FREE;
	p = pconvert( p->in.left );

	/* make p look like ptr to x */

	if( !ISPTR(p->in.type)){
		p->in.type = PTR+UNIONTY;
		}

	t = INCREF( q->stype );
	d = q->dimoff;
	s = q->sizoff;

	p = makety( p, t, d, s );

	/* compute the offset to be added */

	off = q->offset;
	dsc = q->sclass;

	if( dsc & FIELD ) {  /* normalize offset */
		switch(q->stype) {

		case CHAR:
		case UCHAR:
			align = ALCHAR;
			s = CHAR;
			break;

		case SHORT:
		case USHORT:
			align = ALSHORT;
			s = SHORT;
			break;

		case INT:
		case UNSIGNED:
			align = ALINT;
			s = INT;
			break;

# ifdef LONGFIELDS
		case LONG:
		case ULONG:
			align = ALLONG;
			s = LONG;
			break;
# endif
		case ENUMTY:
			align = dimtab[s+2];
			break;

		default:
			cerror( "undefined bit field type" );
			}
		off = (off/align)*align;
		}
	if( off != 0 )
	   p = clocal(block(PLUS, p,
		makety(offcon(off, t, d, s), INT, 0, INT),
		t, d, s));

	p = buildtree( UNARY MUL, p, NIL );

	/* if field, build field info */

	if( dsc & FIELD ){
		p = block( FLD, p, NIL, q->stype, 0, q->sizoff );
		p->tn.rval = PKFIELD( dsc&FLDSIZ, q->offset%align );
		}

	return( clocal(p) );
	}
#endif

notstref(p) register NODE *p; {

	/* return 0 if p a legitimate structure reference, 1 otherwise */

	again:

	switch( p->in.op ){

	case FLD:
		p = p->in.left;
		goto again;

	case UNARY MUL:
	case NAME:
# if TARGET != I386
	case OREG:
# endif
# if TARGET == I386 && !defined(LINT)
	case VAUTO:
	case VPARAM:
	case RNODE:
	case QNODE:
	case SNODE:
# endif
		if( ISARY(p->in.type) || ISFTN(p->in.type) ) return(1);
	case REG:
		return(0);

	default:
		return(1);

		}
	}

notlval(p) register NODE *p; {

	/* return 0 if p an lvalue, 1 otherwise */

	if (p == NULL) return 1;

	again:

	switch( p->in.op ){

	case FLD:
		p = p->in.left;
		goto again;

	case UNARY MUL:
		if( hidden_strop(p) ) return( 1 );
		/* FALL THROUGH */
	case NAME:
# if TARGET == I386 && !defined(LINT)
	case VAUTO:
	case VPARAM:
	case RNODE:	/* RNODE, QNODE, SNODE probably never occur here */
	case QNODE:
	case SNODE:
# else !I386
	case OREG:
# endif
		if( ISARY(p->in.type) || ISFTN(p->in.type) ) return(1);
	case REG:
		return(0);

	default:
		if (mklval(p)) 
		    goto again;
		else
		    return(1);

		}

	}

#ifndef LINT

/*
 * given an expression, do what we can to make it acceptable to 
 * notlval(). Currently this consists of just the SCONV-squashing
 * action formerly in clocal().
 */
int
mklval( p )
    NODE *p;
{
    NODE *lp;
    int  size;

    if ( p->in.op == SCONV ) {
	lp = p->in.left;
	size = tsize(p->in.type, p->fn.cdim, p->fn.csiz);
	if (size == tsize(lp->in.type, lp->fn.cdim, lp->fn.csiz)) {
	    /* paint over type of sub-expression node */
	    lp->in.type = p->in.type;
	    lp->fn.cdim = p->fn.cdim;
	    lp->fn.csiz = p->fn.csiz;
	    /* now paint lp over p */
	    *p = *lp;
	    /* and throw away the original */
	    lp->in.op = FREE;
	    return 1;
	}
    }
    return 0;
}

#endif /*LINT*/

NODE *
bcon( i ){ /* make a constant node with value i */
	register NODE *p;

	p = block( ICON, NIL, NIL, INT, 0, INT );
	p->tn.lval = i;
	p->tn.type = INT;
	p->tn.rval = NONAME;
	return( clocal(p) );
	}

NODE *
bpsize(p) register NODE *p; {
	return( offcon( psize(p), p->in.type, p->fn.cdim, p->fn.csiz ) );
	}

OFFSZ
psize( p ) NODE *p; {
	/* p is a node of type pointer; psize returns the
	   size of the thing pointed to */

	int n;
	if( !ISPTR(p->in.type) ){
		/* "pointer required" */
		TYPEO(p->in.type, MESSAGE( 90 ));
		return( SZINT );
		}
	/* note: no pointers to fields */
	n = tsize( DECREF(p->in.type), p->fn.cdim, p->fn.csiz );
	if (n == 0) {
		/* "zero-length array element" */
		WERROR( MESSAGE( 133 ) );
		}
	return(n);
	}

NODE *
convert( p, f )  register NODE *p; {
	/*  convert an operand of p
	    f is either CVTL or CVTR
	    operand has type int, and is converted by the size of the other side
	    */

	register NODE *q, *r;

	q = (f==CVTL)?p->in.left:p->in.right;

	r = block( PMCONV,
		q, bpsize(f==CVTL?p->in.right:p->in.left), INT, 0, INT );
	r = clocal(r);
	if( f == CVTL )
		p->in.left = r;
	else
		p->in.right = r;
	return(p);

	}

enumconvert( p ) register NODE *p; {

	/* change enums to ints, or appropriate types */

#ifndef LINT
	register TWORD ty;

	if( (ty=BTYPE(p->in.type)) == ENUMTY || ty == MOETY ) {
		if( dimtab[ p->fn.csiz ] == SZCHAR ) ty = CHAR;
		else if( dimtab[ p->fn.csiz ] == SZINT ) ty = INT;
		else if( dimtab[ p->fn.csiz ] == SZSHORT ) ty = SHORT;
		else ty = LONG;
		ty = ctype( ty );
#ifdef UFIELDS
		if (p->in.op == FLD) {
			ty = ENUNSIGN(ty);
			}
#endif
		p->fn.csiz = ty;
		MODTYPE(p->in.type,ty);
		if( p->in.op == ICON && ty != LONG ) p->in.type = p->fn.csiz = INT;
		}
#endif
	}

NODE *
pconvert( p ) register NODE *p; {

	/* if p should be changed into a pointer, do so */

	if( ISARY( p->in.type) ){
		p->in.type = DECREF( p->in.type );
		++p->fn.cdim;
		return( buildtree( UNARY AND, p, NIL ) );
		}
	if( ISFTN( p->in.type) )
		return( buildtree( UNARY AND, p, NIL ) );

	return( p );
	}

NODE *
oconvert(p) register NODE *p; {
	/* convert the result itself: used for pointer and unsigned */

	switch(p->in.op) {

	case LE:
	case LT:
	case GE:
	case GT:
		if( ISUNSIGNED(p->in.left->in.type)
		 || ISUNSIGNED(p->in.right->in.type) )
		    p->in.op += (ULE-LE);
	case EQ:
	case NE:
		return( p );

	case MINUS:
		return(  clocal( block( PVCONV,
			p, bpsize(p->in.left), INT, 0, INT ) ) );
		}

	cerror( "illegal oconvert: %s", pcc_opname(p->in.op) );

	return(p);
	}

NODE *
ptmatch(p)  register NODE *p; {

	/* makes the operands of p agree; they are
	   either pointers or integers, by this time */
	/* with MINUS, the sizes must be the same */
	/* with COLON, the types must be the same */

	TWORD t1, t2, t;
	int o, d2, d, s2, s;

	o = p->in.op;
	t = t1 = p->in.left->in.type;
	t2 = p->in.right->in.type;
	d = p->in.left->fn.cdim;
	d2 = p->in.right->fn.cdim;
	s = p->in.left->fn.csiz;
	s2 = p->in.right->fn.csiz;

	switch( o ){

	case CAST:
		if (ISPTR(t1) && ISFLOATING(t2)) {
			/* "illegal pointer conversion" */
			WERROR( MESSAGE( 142 ) );
			t = INT;
			d = 0;
			s = INT;
			p->in.left->in.type = t;
			p->in.left->fn.cdim = d;
			p->in.left->fn.csiz = s;
			}
		   break;

	case ASSIGN:
	case RETURN:
		{  break; }

	case MINUS:
		{  if( psize(p->in.left) != psize(p->in.right) ){
			/* "illegal pointer subtraction" */
			UERROR( MESSAGE( 67 ));
			}
		   break;
		   }
	case COLON:
		/* "illegal types in :" */
		{  if( t1 != t2 ) UERROR( MESSAGE( 71 ));
		   break;
		   }
	default:  /* must work harder: relationals or comparisons */

		if( !ISPTR(t1) ){
			t = t2;
			d = d2;
			s = s2;
			break;
			}
		if( !ISPTR(t2) ){
			break;
			}

		/* both are pointers */
		if( talign(t2,s2) < talign(t,s) ){
			t = t2;
			s = s2;
			}
		break;
		}

	p->in.left = makety( p->in.left, t, d, s );
	p->in.right = makety( p->in.right, t, d, s );
	if( o!=MINUS && !logop(o) ){

		p->in.type = t;
		p->fn.cdim = d;
		p->fn.csiz = s;
		}
	if( ISPTR(t) ) {
		switch (o) {
		/*
		 * if two pointers are being compared for
		 * ordering, use an unsigned comparison.
		 */
		case GE:
		case GT:
		case LE:
		case LT:
			p->in.op += (ULE-LE);
			break;
			}
		}
	return(clocal(p));
	}

int tdebug = 0;

#ifdef VAX
NODE *
tymatch(p)  register NODE *p; {

	/* satisfy the types of various arithmetic binary ops */

	/* rules are:
		if assignment, op, type of LHS
		if any float or doubles, make double
		if any longs, make long
		otherwise, make int
		if either operand is unsigned, the result is...
	*/

	register TWORD t1, t2, t, tu;
	register o, u;

	o = p->in.op;

	t1 = p->in.left->in.type;
	t2 = p->in.right->in.type;
	if( (t1==TVOID || t2==TVOID) && o!=CAST )
		/* "void type illegal in expression" */
		UERROR(MESSAGE( 118 ));

	u = 0;
	if( ISUNSIGNED(t1) ){
		u = 1;
		t1 = DEUNSIGN(t1);
		}
	if( ISUNSIGNED(t2) ){
		u = 1;
		t2 = DEUNSIGN(t2);
		}

	if( ( t1 == CHAR || t1 == SHORT ) && o!= RETURN ) t1 = INT;
	if( t2 == CHAR || t2 == SHORT ) t2 = INT;

	if( t1==DOUBLE || t1==FLOAT || t2==DOUBLE || t2==FLOAT ) t = DOUBLE;
	else if( t1==LONG || t2==LONG ) t = LONG;
	else t = INT;

	if( asgop(o) ){
		/* make rhs match lhs */
		tu = p->in.left->in.type;
		t = t1;
		}
	else {
		/* make lhs, rhs match t, with unsignedness preserved */
		tu = (u && UNSIGNABLE(t))?ENUNSIGN(t):t;
		}

	/* because expressions have values that are at least as wide
	   as INT or UNSIGNED, the only conversions needed
	   are those involving FLOAT/DOUBLE, and those
	   from LONG to INT and ULONG to UNSIGNED */

	if( t != t1 ) p->in.left = makety( p->in.left, tu, 0, (int)tu );

	if( t != t2 || o==CAST ) p->in.right = makety(p->in.right, tu, 0, (int)tu);

	if( asgop(o) ){
		p->in.type = p->in.left->in.type;
		p->fn.cdim = p->in.left->fn.cdim;
		p->fn.csiz = p->in.left->fn.csiz;
		}
	else if( !logop(o) ){
		p->in.type = tu;
		p->fn.cdim = 0;
		p->fn.csiz = t;
		}

# ifndef BUG1
	if( tdebug ) printf( "tymatch(0x%x): 0x%x %s 0x%x => 0x%x\n",p,t1,opst[o],t2,tu );
# endif

	return(p);
	}
#else !VAX

#ifndef LINT
extern NODE *rewrite_asgop();
extern TWORD icontype();
#endif

NODE *
tymatch(p)  register NODE *p; {

	/* satisfy the types of various arithmetic binary ops */

	/* rules are:
		if assignment, op, type of LHS
		if any float or doubles, make double
		if any longs, make long
		otherwise, make int
		if either operand is unsigned, the result is...
	*/

	register TWORD t1, t2, t, tu;
	register NODE *q;
	register o, u;
	int u1, u2;

	o = p->in.op;

	q = p->in.left;
	if (q->in.op == FLD) {
		/* clobber - assume type checking has been done already */
		t1 = q->in.type = UNSIGNED;
	} else {
		t1 = q->in.type;
	}
	q = p->in.right;
	if (q->in.op == FLD) {
		/* clobber - assume type checking has been done already */
		t2 = q->in.type = UNSIGNED;
	} else {
		t2 = q->in.type;
	}
	if( (t1==TVOID || t2==TVOID) && o!=CAST )
		/* "void type illegal in expression" */
		UERROR(MESSAGE( 118 ));

	u = 0;
	if( ISUNSIGNED(t1) ){
		u++;
		}
	if( ISUNSIGNED(t2) ){
		u++;
		}

	/*
	 * For integral constant operands, figure out the
	 * optimal type.  Note that we must have decided
	 * whether it will be unsigned before we can do this.
	 */
#ifndef LINT
	q = p->in.left;
	if (q->in.op == ICON && q->tn.rval == NONAME) {
		t1 = icontype(u,q->tn.lval);
		q = makety(q, (int)t1, 0, (int)t1);
	} 
	q = p->in.right;
	if (q->in.op == ICON && q->tn.rval == NONAME) {
		t2 = icontype(u,q->tn.lval);
		q = makety(q, (int)t2, 0, (int)t2);
	}
#endif LINT
	u1 = u2 = 0;
	if (ISUNSIGNED(t1)) {
		t1 = DEUNSIGN(t1);
		u1 = 1;
		}
	if (ISUNSIGNED(t2)) {
		t2 = DEUNSIGN(t2);
		u2 = 1;
		}
#ifndef LINT
	switch(o) {
	case ASG DIV:
	case ASG MOD:
		/*
		 * for dividing ops, if x has a smaller type than y, then
		 * x <op>= y must be rewritten as x = x <op> y.  This is
		 * true for both integral types and floating types.
		 */
		if ( t1 < t2 || u1 != u2 ) {
			return rewrite_asgop(p);
		}
		goto float_lhs_test;
	case ASG MUL:
	case ASG PLUS:
	case ASG MINUS:
		/*
		 * for other binary arithmetic ops, if x has an integral
		 * type and y has a floating type, then x <op>= y must be
		 * rewritten as x = x <op> y.  This is a stronger condition
		 * than the above case.
		 */
		if ( ISINTEGRAL(t1) && ISFLOATING(t2) ) {
			return rewrite_asgop(p);
		}
	float_lhs_test:
		/*
		 * if x is float and y is anything that can't always be
		 * represented exactly as a float, then x <op>= y must
		 * be rewritten so that it can be done in double precision.
		 */
		if (t1 == FLOAT && t2 != FLOAT && t2 >= SHORT) {
			return rewrite_asgop(p);
		}
		break;
	}

#endif
	if ( logop(o) ) switch (t1) {
	  case FLOAT:
#ifdef FLOATMATH
			if (floatmath){
				t = t2==DOUBLE ? DOUBLE : FLOAT;
				break;
				}
			/* else fall through */
#endif
	  case DOUBLE:	t = DOUBLE;
			break;

	  case LONG:
	  case INT:
	  case SHORT:
	  case CHAR:
			/* here we use ordering of TWORD type */
			t = t1 > t2 ? t1 : t2;
			if (u1 != u2) {
				/*
				 * careful: one's signed, the other isn't.
				 * If the types are the same size, or if
				 * the smaller type is the signed one, we
				 * must promote BOTH to a larger type.
				 */
				if (t1 >= t2 && u2 == 0
				    || t1 <= t2 && u1 == 0) {
					switch (t) {

					case CHAR:
						t = SHORT;
						break;

					case SHORT:
						t = INT;
						break;

					case INT:
						t = LONG;
						break;
					}
				}
			}
			t = ctype(t);
#ifdef FLOATMATH
			if ( t2==FLOAT )
			    t = (floatmath)?FLOAT:DOUBLE;
			else if (t2==DOUBLE)
			    t = DOUBLE;
#else
			if (t2==FLOAT || t2==DOUBLE)
			    t = DOUBLE;
#endif
			break;
	} else 
#ifdef FLOATMATH
	if (t1 == DOUBLE || t2 == DOUBLE) t = DOUBLE;
	else if(t1 == FLOAT || t2 == FLOAT) 
		if (floatmath)	t = FLOAT;
		else		t = DOUBLE;
#else
	if (t1==DOUBLE || t1==FLOAT || t2==DOUBLE || t2==FLOAT ) t = DOUBLE;
#endif
	else if (t1==LONG || t2==LONG) t = LONG;
	else t = INT;

	if( asgop(o) ){
		/* make rhs match lsh */
		/* make rhs match lhs */
		tu = p->in.left->in.type;
		}
	else {
		/* make lhs, rhs match t, with unsignedness preserved */
		/* make lhs, rhs match t, with unsignedness preserved */
		tu = (u && UNSIGNABLE(t))?ENUNSIGN(t):t;
                if( tu != t1 )
                        p->in.left = makety( p->in.left, tu, 0, (int)tu );
		if( tu != t1 )
			p->in.left = makety( p->in.left, tu, 0, (int)tu );
		}
	if( tu != t2 || o==CAST )
		p->in.right = makety(p->in.right, tu, 0, (int)tu);
	if( asgop(o) ){
		p->in.type = p->in.left->in.type;
		p->fn.cdim = p->in.left->fn.cdim;
		p->fn.csiz = p->in.left->fn.csiz;
		}
	else if ( logop(o) ) {
		p->in.type = INT;
		p->fn.cdim = 0;
		p->fn.csiz = INT;
		}
	else	{
		p->in.type = tu;
		p->fn.cdim = 0;
		p->fn.csiz = t;
		}

# ifndef BUG1
	if( tdebug ) printf( "tymatch(0x%x): 0x%x %s 0x%x => 0x%x\n",p,t1,opst[o],t2,tu );
# endif

	return(p);
	}
#endif

NODE *
uconv(p) register NODE *p; {

	/* perform the usual conversions for unary ops */

	/* rules are:
		make int
		if the operand is unsigned, the result is...
	*/

	register NODE *lp;

	lp = p->in.left;
	if (lp->in.type==CHAR || lp->in.type==SHORT)
		lp = makety(lp, INT, 0, INT);
	else if (lp->in.type==UCHAR || lp->in.type==USHORT) 
		lp = makety(lp, UNSIGNED, 0, UNSIGNED);
	p->in.left = lp;
	p->in.type = lp->in.type;
	p->fn.cdim = 0;
	p->fn.csiz = lp->fn.csiz;
	return(p);
	}

NODE *
makety( p, t, d, s ) register NODE *p; TWORD t; {
	/* make p into type t by inserting a conversion */

	if( p->in.type == ENUMTY && p->in.op == ICON ) enumconvert(p);
	if( t == p->in.type ){
		p->fn.cdim = d;
		p->fn.csiz = s;
		return( p );
		}

	if( t & TMASK ){
		/* non-simple type */
		return( clocal( block( PCONV, p, NIL, t, d, s )) );
		}

	if( p->in.op == ICON && p->tn.rval == NONAME){
		if( t==DOUBLE||t==FLOAT ){
			p->in.op = FCON;
			if( ISUNSIGNED(p->in.type) ){
				p->fpn.dval = (unsigned CONSZ) p->tn.lval;
				}
			else {
				p->fpn.dval = p->tn.lval;
				}

			p->in.type = p->fn.csiz = t;
			return( clocal(p) );
			}
		}

	return( clocal( block( SCONV, p, NIL, t, d, s )) );

	}

NODE *
block( o, l, r, t, d, s ) register NODE *l, *r; TWORD t; {

	register NODE *p;

	p = talloc();
	p->in.op = o;
	p->in.left = l;
	p->in.right = r;
	p->in.type = t;
	p->fn.cdim = d;
	p->fn.csiz = s;
	return(p);
	}

icons(p) register NODE *p; {
	/* if p is an integer constant, return its value */
	int val;

	if( p->in.op != ICON ){
		/* "constant expected" */
		UERROR( MESSAGE( 23 ));
		val = 1;
		}
	else {
		val = p->tn.lval;
		/* "constant too big for cross-compiler" */
		if( val != p->tn.lval ) UERROR( MESSAGE( 25 ) );
		}
	tfree( p );
	return(val);
	}

/* 	the intent of this table is to examine the
	operators, and to check them for
	correctness.

	The table is searched for the op and the
	modified type (where this is one of the
	types INT (includes char and short), LONG,
	DOUBLE (includes FLOAT), and POINTER

	The default action is to make the node type integer

	The actions taken include:
		PUN	  check for puns
		CVTL	  convert the left operand
		CVTR	  convert the right operand
		TYPL	  the type is determined by the left operand
		TYPR	  the type is determined by the right operand
		TYMATCH	  force type of left and right to match, by inserting conversions
		PTMATCH	  like TYMATCH, but for pointers
		LVAL	  left operand must be lval
		CVTO	  convert the op
		NCVT	  do not convert the operands
		OTHER	  handled by code
		NCVTR	  convert the left operand, not the right...
		RINT      widen right to "int", since left is pointer
		ELINT     widen left to "int", since right is pointer
		UCONV	  perform usual unary conversions (widenings)
		MISMATCH  types are mismatched, make node type TERROR

	*/

# define MINT 01  /* integer */
# define MDBI 02   /* integer or double */
# define MSTR 04  /* structure */
# define MPTR 010  /* pointer */
# define MPTI 020  /* pointer or integer */
# define MENU 040 /* enumeration variable or member */
# define MERR 0100 /* error type */
# define MVOID 0100000 /* void type */

opact( p )  NODE *p; {

	register mt12, mt1, mt2, o;

	mt1 = mt2 = 0;

	switch( optype(o=p->in.op) ){

	case BITYPE:
		mt2 = moditype( p->in.right->in.type );
	case UTYPE:
		mt1 = moditype( p->in.left->in.type );
		break;

		}

	/*
	 * TVOID is usually an illegal operand type,
	 * with the following exceptions:
	 */
	if ((mt1 | mt2) & MVOID) {
		switch(o) {
		case COMOP:
		case COLON:
			/* void legal here */
			break;
		case QUEST:
			/*
			 * ((...) ? (void) : (void)) is ok;
			 * ((void) ? (...) : (...)) isn't.
			 */
			if (mt1 & MVOID)
				goto illegal_void;
			break;
		case CAST:
			/*
			 * casting TO void is ok;
			 * casting FROM void isn't,
			 *	unless the TO type is also void.
			 */
			if ((mt2 & MVOID) && !(mt1 & MVOID))
				goto illegal_void;
			break;
		case RETURN:
			/*
			 * if lhs of RETURN is void,
			 * grammar will complain.
			 */
			return( MISMATCH );
		default:
		illegal_void:
			uerror( "void type illegal in expression" );
			return( MISMATCH );
			}
		}

	mt12 = mt1 & mt2;

	switch( o ){

	case NAME :
	case STRING :
	case ICON :
	case FCON :
	case CALL :
	case UNARY CALL:
	case UNARY MUL:
		{  return( OTHER ); }
	case UNARY MINUS:
		if( mt1 & MINT ) return( UCONV );
		if( mt1 & MDBI ) return( TYPL );
		break;

	case COMPL:
		if( mt1 & MINT ) return( UCONV );
		break;

	case UNARY AND:
		{  return( NCVT+OTHER ); }
	case CBRANCH:
	case NOT:
		if ( !(mt1 & MSTR ) ) return( 0 );
		break;
	case INIT:
	case CM:
		return( 0 );
	case ANDAND:
	case OROR:
		if ( !(mt1 & MSTR) && !(mt2 & MSTR) )
			return( 0 );
		break;

	case MUL:
	case DIV:
		if( mt12 & MDBI ) return( TYMATCH );
		break;

	case MOD:
	case AND:
	case OR:
	case ER:
		if( mt12 & MINT ) return( TYMATCH );
		break;

	case LS:
	case RS:
		if( mt12 & MINT ) return( UCONV+OTHER );
/*		if( mt12 & MINT ) return( ELINT+TYMATCH+OTHER ); */
		break;

	case EQ:
	case NE:
	case LT:
	case LE:
	case GT:
	case GE:
		if( (mt1&MENU)||(mt2&MENU) ) return( PTMATCH+PUN+NCVT );
		if( mt12 & MDBI ) return( TYMATCH+CVTO );
		else if( mt12 & MPTR ) return( PTMATCH+PUN );
		else if( mt12 & MPTI ) return( PTMATCH+PUN );
#ifndef VAX
		else if ( (mt1&MPTR) && (p->in.right->in.op==ICON)) return ( PTMATCH+PUN+RINT );
		else if ( (mt2&MPTR) && (p->in.left->in.op==ICON)) return ( PTMATCH+PUN+ELINT );
#endif
		else break;

	case QUEST:
		if( mt1 & MSTR ) break;
	case COMOP:
		if( mt2&MENU ) return( TYPR+NCVTR );
		return( TYPR );

	case STREF:
		return( NCVTR+OTHER );

	case FORCE:
		return( TYPL );

	case COLON:
		if( mt12 & MENU ) return( NCVT+PUN+PTMATCH );
		else if( mt12 & MDBI ) return( TYMATCH );
		else if( mt12 & MPTR ) return( TYPL+PTMATCH+PUN );
#ifdef VAX
		else if( (mt1&MINT) && (mt2&MPTR) ) return( TYPR+PUN );
		else if( (mt1&MPTR) && (mt2&MINT) ) return( TYPL+PUN );
#else
		else if( (mt1&MINT) && (mt2&MPTR) ) return( TYPR+PUN+ELINT );
		else if( (mt1&MPTR) && (mt2&MINT) ) return( TYPL+PUN+RINT );
#endif
		else if( mt12 & MSTR ) return( NCVT+TYPL+OTHER );
		else if( mt12 & MVOID ) return( TYPL );
		break;

	case ASSIGN:
	case RETURN:
		if( mt12 & MSTR ) return( LVAL+NCVT+TYPL+OTHER );
	case CAST:
		if( o==CAST && (mt1&MVOID) )return(TYPL+TYMATCH);
		if( mt12 & MDBI ) return( TYPL+LVAL+TYMATCH );
		else if( (mt1&MENU)||(mt2&MENU) ) return( LVAL+NCVT+TYPL+PTMATCH+PUN );
		/* if right is TVOID and looks like a CALL, break */
		else if ( ( mt2&MVOID ) && (p->in.right->in.op == CALL
			|| p->in.right->in.op == UNARY CALL ) ) break;
		else if( mt1 & MPTR && !(mt2 & MSTR) )
			return( LVAL+PTMATCH+PUN );
		else if( mt12 & MPTI ) return( TYPL+LVAL+TYMATCH+PUN );
		break;

	case ASG LS:
	case ASG RS:
		if( mt12 & MINT ) return( TYPL+LVAL+OTHER );
		break;

	case ASG MUL:
	case ASG DIV:
		if( mt12 & MDBI ) return( LVAL+TYMATCH );
		break;

	case ASG MOD:
	case ASG AND:
	case ASG OR:
	case ASG ER:
		if( mt12 & MINT ) return( LVAL+TYMATCH );
		break;

	case ASG PLUS:
	case ASG MINUS:
	case INCR:
	case DECR:
		if( mt12 & MDBI ) return( TYMATCH+LVAL );
#ifdef VAX
		else if( (mt1&MPTR) && (mt2&MINT) ) return( TYPL+LVAL+CVTR );
#else
		else if( (mt1&MPTR) && (mt2&MINT) ) return( TYPL+LVAL+CVTR+RINT );
#endif
		break;

	case MINUS:
		if( mt12 & MPTR ) return( CVTO+PTMATCH+PUN );
		if( mt2 & MPTR ) break;
	case PLUS:
		if( mt12 & MDBI ) return( TYMATCH );
#ifdef VAX
		else if( (mt1&MPTR) && (mt2&MINT) ) return( TYPL+CVTR );
		else if( (mt1&MINT) && (mt2&MPTR) ) return( TYPR+CVTL );
#else
		else if( (mt1&MPTR) && (mt2&MINT) ) return( TYPL+CVTR+RINT );
		else if( (mt1&MINT) && (mt2&MPTR) ) return( TYPR+CVTL+ELINT );
#endif

		}
	/* before we bitch, check it out */
	if (mt1&MERR) return TYPL;
	else if (mt2&MERR) return TYPR;
	else if ( ( mt1 == MSTR || mt2 == MSTR) &&
		( o != ASSIGN && o != RETURN && o != COLON ) )
		/* "%s is not a permitted struct/union operation" */
		UERROR( MESSAGE( 135 ), opst[o] );
	else
		/* "operands of %s have incompatible types" */
		UERROR( MESSAGE( 89 ), opst[o] );
	return( MISMATCH );
	}

moditype( ty ) TWORD ty; {

	switch( ty ){

	case TVOID:
	case UNDEF:
		return( MVOID ); /* type is void */
	case ENUMTY:
	case MOETY:
#ifdef LINT
		/*
		 * By default, lint treats enums as unique types.
		 * With -q, it takes the post-1978 view that enums
		 * are to be treated as ints.
		 */
		if (!qflag) return( MENU );
#endif
		return( MINT|MPTI|MDBI );

	case STRTY:
	case UNIONTY:
		return( MSTR );

	case CHAR:
	case SHORT:
	case UCHAR:
	case USHORT:
		return( MINT|MPTI|MDBI );
	case UNSIGNED:
	case ULONG:
	case INT:
	case LONG:
		return( MINT|MDBI|MPTI );
	case FLOAT:
	case DOUBLE:
		return( MDBI );
	default:
		if (BTYPE(ty) == TERROR) 
			return( MERR );
		return( MPTR|MPTI );

		}
	}

#ifdef DEBUG
static void
printmt(mt)
	register mt;
{
	register char *fmt, *fmt2;

	fmt = "%s";
	fmt2 = "|%s";
	printf("(");
	if (mt&MINT) { printf(fmt, "MINT"); fmt = fmt2; }
	if (mt&MDBI) { printf(fmt, "MDBI"); fmt = fmt2; }
	if (mt&MSTR) { printf(fmt, "MSTR"); fmt = fmt2; }
	if (mt&MPTR) { printf(fmt, "MPTR"); fmt = fmt2; }
	if (mt&MPTI) { printf(fmt, "MPTI"); fmt = fmt2; }
	if (mt&MENU) { printf(fmt, "MENU"); fmt = fmt2; }
	if (mt&MERR) { printf(fmt, "MERR"); fmt = fmt2; }
	printf(")\n");
}

static void
printact(act)
	register act;
{
	register char *fmt, *fmt2;

	fmt = "%s";
	fmt2 = "|%s";
	printf("(");
	if(act&NCVT)	{ printf(fmt, "NCVT"); fmt = fmt2; }
	if(act&PUN)	{ printf(fmt, "PUN"); fmt = fmt2; }
	if(act&TYPL)	{ printf(fmt, "TYPL"); fmt = fmt2; }
	if(act&TYPR)	{ printf(fmt, "TYPR"); fmt = fmt2; }
	if(act&TYMATCH)	{ printf(fmt, "TYMATCH"); fmt = fmt2; }
	if(act&LVAL)	{ printf(fmt, "LVAL"); fmt = fmt2; }
	if(act&CVTO)	{ printf(fmt, "CVTO"); fmt = fmt2; }
	if(act&CVTL)	{ printf(fmt, "CVTL"); fmt = fmt2; }
	if(act&CVTR)	{ printf(fmt, "CVTR"); fmt = fmt2; }
	if(act&PTMATCH)	{ printf(fmt, "PTMATCH"); fmt = fmt2; }
	if(act&OTHER)	{ printf(fmt, "OTHER"); fmt = fmt2; }
	if(act&NCVTR)	{ printf(fmt, "NCVTR"); fmt = fmt2; }
	if(act&RINT)	{ printf(fmt, "RINT"); fmt = fmt2; }
	if(act&ELINT)	{ printf(fmt, "ELINT"); fmt = fmt2; }
	if(act&UCONV)	{ printf(fmt, "UCONV"); fmt = fmt2; }
	printf(")\n");
}
#endif DEBUG

NODE *
doszof( p )  register NODE *p; {
	/* do sizeof p */
	int i;

	/* whatever is the meaning of this if it is a bitfield? */
	i = tsize( p->in.type, p->fn.cdim, p->fn.csiz )/SZCHAR;

	tfree(p);
	/* "sizeof returns value less than or equal to zero" */
	if( i <= 0 ) WERROR( MESSAGE( 99 ) );
	lxsizeof--;
	return( bcon( i ) );
	}

eprint( p, down, a, b )
	register NODE *p;
	int *a, *b;
{
	register ty;

	*a = *b = down+1;
	while( down > 1 ){
		printf( "\t" );
		down -= 2;
		}
	if( down ) printf( "    " );

	ty = optype( p->in.op );

	printf("0x%x) %s, ", p, opst[p->in.op] );
	if( ty == LTYPE ){
		printf( CONFMT, p->tn.lval );
		printf( ", %d, ", p->tn.rval );
	} else if (p->in.op == FLD) {
		printf( "<%d:%d> ", UPKFOFF(p->tn.rval), UPKFSZ(p->tn.rval) );
	}
	tprint( p->in.type );
	printf( ", %d, %d\n", p->fn.cdim, p->fn.csiz );
}

int edebug = 0;

#ifndef LINT
extern void p1tmpfree();
#endif

ecomp( p ) register NODE *p; {
	extern void prtdcon();
# ifndef BUG2
	if( edebug ) fwalk( p, eprint, 0 );
# endif
	if( !reached && !reachflg ){
		/* "statement not reached"  */
		WERROR( MESSAGE( 100 ) );
		reached = 1;
		}
	p = optim(p);
	walkf( p, prtdcon );
	locctr( PROG );
	ecode( p );
	tfree(p);
#ifndef LINT
	p1tmpfree();
#endif
	}

# ifdef STDPRTREE
# ifndef ONEPASS

# if TARGET != I386

prtree(p) 
	register NODE *p; 
{

	register ty;

# ifdef MYPRTREE
	MYPRTREE(p);  /* local action can be taken here; then return... */
#endif

	ty = optype(p->in.op);

	printf( "%d\t", p->in.op );

	if( ty == LTYPE ) {
		printf( CONFMT, p->tn.lval );
		printf( "\t" );
		}
	if( ty != BITYPE ) {
		if( p->in.op == NAME || p->in.op == ICON ) NAMEHACK(p);
		else printf( "%d\t", p->tn.rval );
		}

	printf( "0x%x\t", p->in.type );

	/* handle special cases */

	switch( p->in.op ){

	case NAME:
	case ICON:
		/* print external name */
		PRNAME( p );
		break;

	case STARG:
	case STASG:
	case STCALL:
	case UNARY STCALL:
		/* print out size */
		/* use lhs size, in order to avoid hassles with the structure `.' operator */

		/* note: p->in.left not a field... */
		printf( CONFMT, (CONSZ) tsize( STRTY, p->in.left->fn.cdim, p->in.left->fn.csiz ) );
		printf( "\t%d\t\n", talign( STRTY, p->in.left->fn.csiz ) );
		break;

	default:
		printf(  "\n" );
	}

	if( ty != LTYPE ) prtree( p->in.left );
	if( ty == BITYPE ) prtree( p->in.right );

}
# endif !I386
# else

static char *
mklab( lno )
    int lno;
{
    char temp[32];
    sprintf( temp, LABFMT, lno);
    return (tstr(temp));
}

# if TARGET == I386 && !defined(LINT)
/* The p2tree routine is taken from System V for Roadrunner */
extern NODE *myp2tree();

/*
 * Due to the mixture of optimizations which occur, this is changed for
 * roadrunner to return a NODE pointer.
 */
NODE *
p2tree(p)
register NODE *p;
{
	register ty;
	register o;
	struct symtab *q;
	char temp[32];			/* place to dump label stuff */

# ifdef MYP2TREE
	p = MYP2TREE(p);  /* local action can be taken here; then return... */
# endif

	/* this routine sits painfully between pass1 and pass2 */
	ty = optype(o=p->in.op);
	p->tn.goal = 0;  /* an illegal goal, just to clear it out */
	p->tn.type = ttype( p->tn.type );  /* type gets second-pass (bits) */

	switch( o )
	{
	case NAME:
	case ICON:
		if( p->tn.rval == NONAME ) {
			p->in.name = (char *) 0;
		} else if (p->tn.rval < 0 ) {
			p->tn.name = mklab( -p->tn.rval );
		} else {
			 /* copy name from exname */
			q = STP( p->tn.rval );
			if ( q->sclass == STATIC && q->slevel >0){
				p->in.name = mklab( q->offset );
			} else {
				p->in.name = tstr(exname(q->sname) );
			}
		}
		break;

	case PCONV:
		p->in.op = SCONV;
		break;

	case STARG:
	case STASG:
	case STCALL:
	case UNARY STCALL:
		/* set up size parameters */
		{
		    /* information about structure size, align is in this node
		    ** for assign and arg, left side for others
		    */
		    NODE * stinfo = (o == STASG || o == STARG ? p : p->in.left);

		    p->stn.stsize = tsize(STRTY,stinfo->fn.cdim,stinfo->fn.csiz);
		    p->stn.stalign = talign(STRTY,stinfo->fn.csiz);
		}
		break;

	/* leave FCON alone; clearing p->in.name zeroes the floating constant! */
	case FCON:
		break;

		/* this should do something only if temporary regs are
		/* built into the tree by machine-dependent actions */
	case REG:
		rbusy( p->tn.rval, p->in.type );
	default:
		p->in.name = (char *) 0;
	}

	if( ty != LTYPE ) p->in.left = p2tree( p->in.left );
	if( ty == BITYPE ) p->in.right = p2tree( p->in.right );
	return p;
}
# else !I386
p2tree(p) 
	register NODE *p; 
{
	register ty;
	int nameflag;
	register struct symtab *q;

# ifdef MYP2TREE
	MYP2TREE(p);  /* local action can be taken here; then return... */
# endif

	ty = optype(p->in.op);

	switch( p->in.op ){

	case FCON:
		break;
	case EQ:
	case NE:
	case LT:
	case LE:
	case GT:
	case GE:
	case ULT:
	case ULE:
	case UGT:
	case UGE:
		p->bn.reversed = 0;
		break;

	case NAME:
	case ICON:
		nameflag = NAMEHACK(p);
		if( p->tn.rval == NONAME ) p->in.name = "";
		else if (p->tn.rval <0 ){
		    p->tn.name = mklab( -p->tn.rval );
		} else {
		    q = STP(p->tn.rval);
		    if ( q->sclass == STATIC && q->slevel >0){
			p->in.name = mklab( q->offset );
		    } else {
			p->in.name = tstr(exname(q->sname) );
		    }
		}
		p->tn.rval = nameflag;
		break;

	case STARG:
	case STASG:
	case STCALL:
	case UNARY STCALL:
		/* set up size parameters */
		p->stn.stsize = (tsize(STRTY,p->in.left->fn.cdim,p->in.left->fn.csiz)+SZCHAR-1)/SZCHAR;
		p->stn.stalign = talign(STRTY,p->in.left->fn.csiz)/SZCHAR;
		break;

	case REG:
		rbusy( p->tn.rval, p->in.type );

	default:
		p->in.name = "";
		}

	p->in.rall = NOPREF;

	if( ty != LTYPE ) p2tree( p->in.left );
	if( ty == BITYPE ) p2tree( p->in.right );
	}
# endif
# endif
# endif

