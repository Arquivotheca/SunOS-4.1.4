#ifndef lint
static  char sccsid[] = "@(#)fortran.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * FORTRAN dependent symbol routines.
 */

#include "defs.h"
#include "symbols.h"
#include "printsym.h"
#include "languages.h"
#include "fortran.h"
#include "tree.h"
#include "eval.h"
#include "operators.h"
#include "mappings.h"
#include "process.h"
#include "runtime.h"
#include "machine.h"

private Integer indentlevel;                    /* Current indentation */

private fortran_evalparams();
private fortran_printdecl();
private fortran_printval();
private fortran_printparams();
private Boolean fortran_typematch();
private Node fortran_buildaref();
private int fortran_evalaref();

#define isrange(t, name) (t->class == RANGE and istypename(t->type, name))

/*
    #define dummy_proc(param) (param->type->class == FFUNC)
*/

#define MAXDIM  20
/*
 * Initialize FORTRAN language information.
 */

public fortran_init()
{
    Language lang;

    lang = language_define("fortran");
    language_suffix_add(lang, ".f");
    language_suffix_add(lang, ".F");
    language_suffix_add(lang, ".vf");
    language_suffix_add(lang, ".for");
    language_setop(lang, L_PRINTDECL, fortran_printdecl);
    language_setop(lang, L_PRINTVAL, fortran_printval);
    language_setop(lang, L_PRINTPARAMS, fortran_printparams);
    language_setop(lang, L_EVALPARAMS, fortran_evalparams);
    language_setop(lang, L_TYPEMATCH, fortran_typematch);
    language_setop(lang, L_BUILDAREF, fortran_buildaref);
    language_setop(lang, L_EVALAREF, fortran_evalaref);
}

/*
 * Test if two types are compatible.
 *
 * Integers and reals are not compatible since they cannot always be mixed.
 */

private Boolean fortran_typematch(type1, type2)
Symbol type1, type2;
{

/* only does integer for now; may need to add others       */
/* added string type match Feb 10 1988 DT bug # 1003916    */
/* March 3 1988 D Teeple, allowed integer -> char coercion */
/* fixes bug 1005218                                       */

    Boolean b;
    register Symbol t1, t2;

    t1 = rtype(type1);
    t2 = rtype(type2);
    if (t1 == nil or t1->type == nil or t2 == nil or t2->type == nil) {
	b = false;
    } else {
	b = (Boolean)   (
            (t1 == t2) or 
	    (t1->type == t_int and t2->type == t_int) or
	    (t1->type == t_int and (istypename(t2->type, "integer*4") or
                                    istypename(t2->type, "integer*2"))) or
	    (t2->type == t_int and (istypename(t1->type, "integer*4") or
                                    istypename(t1->type, "integer*2"))) or
	    (isfloat(t1) and isfloat(t2)) or
	    ( istypename(t1->type, "char") and 
	      istypename(t2->type, "$char") and
	      (size(t1) == (size(t2)-1))) or
	    ( istypename(t1->type, "char") and
	      istypename(t2->type, "$integer"))
            );
         }
    /*OUT fprintf(stderr," %d compat %s %s \n", b,
      (t1 == nil or t1->type == nil ) ? "nil" : symname(t1->type),
      (t2 == nil or t2->type == nil ) ? "nil" : symname(t2->type)  );*/
    return b;
}

private String typename(s)
Symbol s;
{
    int ub;
    static char buf[20];
    char *pbuf;
    Symbol st,sc;

	 if(s->type->class == TYPE || s->type->class == TYPEID)
     	return(symname(s->type));

	 for(st = s->type;
     	st->type->class != TYPE && st->type->class != TYPEID ; st = st->type);

     pbuf=buf;

     if(istypename(st->type,"char"))  { 
	  sprintf(pbuf,"character*");
          pbuf += strlen(pbuf);
	  sc = st->chain;
          if(sc->symvalue.rangev.uppertype == R_ARG or
             sc->symvalue.rangev.uppertype == R_TEMP) {
	      if( ! getbound(s,sc->symvalue.rangev.upper, 
                    sc->symvalue.rangev.uppertype, &ub) )
		sprintf(pbuf,"(*)");
	      else 
		sprintf(pbuf,"%d",ub);
          }
 	  else sprintf(pbuf,"%d",sc->symvalue.rangev.upper);
     }
     else {
          sprintf(pbuf,"%s ",symname(st->type));
     }
     return(buf);
}

private Symbol mksubs(pbuf,st)
Symbol st;
char  **pbuf;
{   
   int lb, ub;
   Symbol r;

   if(st->class != ARRAY or (istypename(st->type, "char")) ) return;
   else {
          mksubs(pbuf,st->type);
          assert( (r = st->chain)->class == RANGE);

          if(r->symvalue.rangev.lowertype == R_ARG or
             r->symvalue.rangev.lowertype == R_TEMP) {
	      if( ! getbound(st,r->symvalue.rangev.lower, 
                    r->symvalue.rangev.lowertype, &lb) )
		sprintf(*pbuf,"?:");
	      else 
		sprintf(*pbuf,"%d:",lb);
	  }
          else {
		lb = r->symvalue.rangev.lower;
		sprintf(*pbuf,"%d:",lb);
		}
    	  *pbuf += strlen(*pbuf);

	  if(r->symvalue.rangev.uppertype == R_ADJUST) {
		sprintf(*pbuf,"*,");
	  } else if(r->symvalue.rangev.uppertype == R_ARG or
             r->symvalue.rangev.uppertype == R_TEMP) {
	      if( ! getbound(st,r->symvalue.rangev.upper, 
                    r->symvalue.rangev.uppertype, &ub) )
		sprintf(*pbuf,"?,");
	      else 
		sprintf(*pbuf,"%d,",ub);
	  }
          else {
		ub = r->symvalue.rangev.upper;
		sprintf(*pbuf,"%d,",ub);
		}
    	  *pbuf += strlen(*pbuf);

       }
}

/*
 * Print out the declaration of a FORTRAN variable.
 */
private fortran_printdecl(s)
Symbol s;
{
    register Symbol st = s->type;
    char *type_mod = 0;
    Boolean newline;

    if( s->class != RECORD  and s->class != UNION and  st == nil )
	error("nil type in fortran_printdecl" );

    newline = true;
    switch (s->class) {
	case TYPE:
	case TYPEID:
	    /*
	     * There are two ways to get a "typedef" in FORTRAN.
	     * --> You can say "whatis aStructureTag", or
	     * --> After you've done that, there is an extra TYPE node
	     *		under each FIELD.  This is handled under FIELD.
	     */
	    if( st->class == RECORD ) {
		printf("structure /%s/\n", symname(s) );
		fortran_printdecl(st);
		printf("end structure %s", symname(s));
	    } else  {
		fortran_printdecl(st);
	    }
	    break;

	case CONST:
	    printf("parameter %s = ", symname(s));
            printval(s);
	    break;

        case BASED:
	    type_mod = " (based variable) ";
	    goto print_type_mod;

        case REF:
	    type_mod = " (dummy argument) ";
	    goto print_type_mod;

	case FVAR:
	    type_mod = " (function variable) ";

 print_type_mod:
	    printf("%s", type_mod );
	    /* fall through */

	case VAR:
	case RANGE:
	    if (st->class == ARRAY and
	        (not istypename(st->type, "char"))
	    ) {
		char bounds[130], *p1, **p;

		p1 = bounds;
                p = &p1;
                mksubs(p, st);
                *p -= 1; 
                **p = '\0';   /* get rid of trailing ',' */
		printf("%s %s[%s] ",
		  typename(s), symname(s), bounds);
	    } else {
		if( st->type->class == RECORD ) {
		    printf("structure /%s/ %s", typename(s), symname(s));
		} else {
		    if( st->class == FFUNC ) {
			printf(" (dummy argument) ");
		    }
		    printf("%s %s", typename(s), symname(s));
		}
	    }
	    break;

	case RECORD:
	case UNION:
	    {
		Symbol mem;
     
		indentlevel++;
		for (mem = s->chain; mem != nil; mem = mem->chain) {
		    indent(indentlevel);
		    fortran_printdecl(mem);
		}
		newline = false;
		indentlevel--;
		break;
	    }

	/*
	 * Weird shenanigans if it's a field:  the FIELD contains the
	 * name, but its "->type" is what we need to print the decl of.
	 * For now, at least, FIELDs always have types of class TYPE,
	 * RECORD, or ARRAY.  For TYPE, you need to go down *two* levels
	 * to get at the good stuff; for ARRAYs, just fool ourselves
	 * into thinking that it's a normal VAR.
	 */
	case FIELD:
	    { struct Symbol fakeField;

		fakeField = *st;		/* copy the type */
		fakeField.name = s->name;	/* forge the name */

		switch( st->class ) {
		 case RECORD:
		 case UNION:
		    printf("%s /%s/\n", classname(st), symname(s) );
		    fortran_printdecl( &fakeField );
		    indent(indentlevel);
		    printf("end structure %s\n", symname(s));
		    break;
		 case TYPE:
		    fakeField = *( st->type );
		    fakeField.name = s->name;	/* forge the name */
		    fortran_printdecl( &fakeField );
		    break;
		 case ARRAY:
		    fakeField = *s;
		    fakeField.class = VAR;	/* lie about our class */
		    fortran_printdecl( &fakeField );
		    break;
		 default:
		    error( "field class %s in fortran_printdecl\n",
			classname( st ) );
		}
		newline = false;
		break;
	    }

	case FUNC:
	    if (not istypename(st, "void")) {
                printf(" %s function ", typename(s) );
	    } else {
		printf(" subroutine");
	    }
	    printf(" %s ", symname(s));
	    fortran_listparams(s);
	    break;

	case MODULE:
	    printf("source file \"%s.f\"", symname(s));
	    break;

	case PROG:
	    printf("executable file \"%s\"", symname(s));
	    break;

	case COMMON:
	    printf("common area \"%s\"", symname(s));
	    break;

	default:
	    error("class %s in fortran_printdecl", classname(s));
    }
    if( newline ) {
	printf("\n");
    }
}

/*
 * List the parameters of a procedure or function.
 * No attempt is made to combine like types.
 */

private fortran_listparams(s)
Symbol s;
{
    register Symbol t;

    printf("(");
    for (t = s->chain; t != nil; t = t->chain) {
	printf("%s", symname(t));
	if (t->chain != nil) {
	    printf(", ");
	}
    }
    printf(")");
    if (s->chain != nil) {
	printf("\n");
	for (t = s->chain; t != nil; t = t->chain) {
	/*
	 * Dummy procedure arguments are not reference parameters;
	 * the address in the stack is the address of the routine.
	 * The class of the param is VAR and param->type->class = FFUNC.
	 */
	    if (t->class != REF and t->type->class != FFUNC) {
		panic("unexpected class %d for parameter", t->class);
	    }
	    printdecl(t);
	}
    } else {
	printf("\n");
    }
}

/*
 * Determine whether the parameter/function has extra words pushed on 
 * for it.  Here, check for char.
 */

private Boolean char_type(p, param)
Symbol p;
Boolean param;
{
    register Symbol t;
    register Boolean b;

    b = false;
    if (param and p->class != REF and p->type->class != FFUNC) {
	printf("%s: ", symname(p));
	error("parameter type is not REF or dummy routine");
    }
    p = p->type;
    if (p != nil) {
        t = rtype(p->type);
    	if (t->class == RANGE and istypename(t->type, "char")) {
	    b = true;
	}
    }
    return b;
}

/*
 * Determine whether the function has extra words pushed on 
 * for it.  Here, check for complex.
 */

public Boolean complex_type(p)
Symbol p;
{
    register Symbol t;
    register Boolean b;

    b = false;
    p = p->type;
    if (p != nil) {
        t = rtype(p->type);
    	if (t != nil and t->class == RANGE) {
	    if (istypename(t->type, "complex") or
	      istypename(t->type, "double complex")) {
	    	b = true;
	    }
	}
    }
    return b;
}
    
/*
 * Print the values of the parameters of the given procedure or function.
 * The frame distinguishes recursive instances of a procedure.
 */

private fortran_printparams(f, frame)
Symbol f;
Frame frame;
{
    Symbol param;
    int n, m;

    n = nargspassed(frame);
    param = f->chain;
    if (param != nil or n > 0) {
	printf("(");
	m = n;	
	    /* if routine returns char or complex type, return value's
	       addr [and size] are also on stack so ignore */
	if (char_type(f, false)) {
	    m -= 2;
	} else if (complex_type(f)) {
	    m -= 1;
	}
	if (param != nil) {
	    for (;;) {
		/* if (char_type(param, true) or dummy_proc(param)) { */
		if (char_type(param, true)) {
		    m -= 2;
		} else {
		    m -= 1;
		}
		is_in_where = 1;
		printv(param, frame);
		is_in_where = 0;
		param = param->chain;
	    	if (param == nil) break;
		printf(", ");
	    }
	}
	printf(")");
    }
}

/*
 * Print out the value on the top of the expression stack
 * in the format for the type of the given symbol.
 * fixed Feb 5 1988 DT bug # 1007158, support for logical*n
 * fixed Feb 5 1988 DT bug # 1005217,1006569, char array problems
 */

private fortran_printval(s)
Symbol s;
{
    register Symbol t;
    register Address a;
    register int i, len;
    char *str;
    double d;

    switch (s->class) {
	case TYPE:
	    if (istypename(s, "logical")   ||
	        istypename(s, "logical*1") ||    /*  Feb 5 1988 DT */
	        istypename(s, "logical*2") ||    /*  Feb 5 1988 DT */
	        istypename(s, "logical*4"))  {   /*  Feb 5 1988 DT */
		i = popsmall(s);
		printf(((Boolean) i) ? "true" : "false");
		break;
    	    } /* else FALL THROUGH */
		
	case CONST:
	case VAR:
	case BASED:
	case REF:
	case FVAR:
	case TYPEID:
	    fortran_printval(s->type);
	    break;

	case ARRAY:
	    t = rtype(s->type);
	    if (t->class == RANGE and istypename(t->type, "char")) {
		len = size(s);
		sp -= len;
		printf("\"%.*s\"", len, sp);
	    } else {
		indentlevel++;
		fortran_printarray(s);
		indentlevel--;
	    }
	    break;

	case RECORD:
	    fortran_printrecord(s);
	    break;

	case RANGE:
	     if (isfloat(s)) {
		i = s->symvalue.rangev.lower;
		if ((i == sizeof(double) and istypename(s->type, "complex")) or
		  (i == (sizeof(double) * 2) and
		  istypename(s->type, "double complex"))) {
		    printf("(");
		    d = pop(double);
		    prtreal(pop(double));
		    printf(",");
		    prtreal(d);
		    printf(")");
		} else if (i == sizeof(double) or i == sizeof(float)) {
		    prtreal(pop(double));
		} else {
		    panic("bad size \"%d\" for real", t->symvalue.rangev.lower);
		}
	    } else 
	    if (istypename(s->type, "char")) {
	    	printf("\"%c\"", popsmall(s));  /* Feb 5 1988 DT  */
	    } else {
		printint(popsmall(s), s);
	    }
	    break;

	case FFUNC:
	    a = pop(Address);
	    if (a == 0) {
		printf("(nil)");
	    } else if ((str = srcprocname(a)) != nil) {
		printf("%s()", str);
	    } else {
		printf("0x%x", a);
	    }
	    break;
	
	case PTR:
	    a = pop(Address);
	    if (a == 0) {
		printf("(nil)");
	    } else {
		printf("0x%x", a);
	    }
	    break;

	case PROC:
	case FUNC:
	    t = pop(Symbol);
	    printf("%s", symname(t));
	    break;

	case FIELD:
		if(s->type->class == TYPE && (t = s->type->type) &&
			t->class == RECORD) {
			printf(" structure /%s/", symname(s->type) );
			printval( t );
		} else {
			indent(indentlevel);
	        printval( s->type );
		}
	    break;

	case UNION:
		printf("<union>\n");
		break;

	case SCAL:
	default:
	    if (ord(s->class) > ord(TYPEREF)) {
		panic("printval: bad class %d", ord(s->class));
	    }
	    error("don't know how to print a %s", fortran_classname(s));
	    /* NOTREACHED */
    }
}

/*
 * Print out the value of a FORTRAN record (similar to a C structure)
 */
private fortran_printrecord( s )
Symbol s;
{
    register Symbol f;
    register Stack *savesp;
    register Integer n, off, len;
    String   name;
    Integer  namelen;
    Integer  maxlen;
    Integer  i;

    /*
     * Find the length of the longest field name
     */
    maxlen = 0;
    for (f = s->chain; f != nil; f = f->chain) {
	name = symname(f);
	namelen = strlen(name);
	if (namelen > maxlen) {
	    maxlen = namelen;
	}
    }

    /*
     * Print out the fields' values
     */
    sp -= size(s);
    savesp = sp;
    indentlevel++;
    printf("(\n");
    for (f = s->chain; f != nil; f = f->chain) {
	indent(indentlevel);
	name = symname(f);
	namelen = strlen(name);
	printf("%s", name);
	for (i = namelen; i < maxlen; i++) {
	    printf(" ");
	}
	printf(" = ");
	/* size includes offset % 8, here we add offset div 8 */
	n = size(f);
	sp += (n + (f->symvalue.field.offset div BITSPERBYTE));
	p_str_arr_val(f, n);
	printf("\n");
	sp = savesp;
    }
    indentlevel--;
    indent(indentlevel);
    printf(")");
}

/*
 * Print out an int 
 * repaired Feb 5 1988 DT bug # 107158, support for byte
 */

private printint(i, t)
Integer i;
register Symbol t;
{
    if ( (t->type == t_int) or 
         istypename(t->type, "byte") or         /* DT Feb 5 1988 */
         istypename(t->type, "integer*1") or    /* DT Feb 5 1988 */
         istypename(t->type, "integer*2") or
         istypename(t->type, "integer*4") ) {
	printf("%ld", i);
    } else {
        error("unknown type in fortran printint");
    }
}

/*
 * Return the FORTRAN name for the particular class of a symbol.
 */

public String fortran_classname(s)
Symbol s;
{
    String str;

    switch (s->class) {
	case REF:
	    str = "dummy argument";
	    break;

	case CONST:
	    str = "parameter";
	    break;

	default:
	    if( s != nil ) {
		if( s->type == nil ) {
		    panic( "Missing type in fortran_classname" );
		} else if( s->type->class == FFUNC ) {
		    str = "dummy argument";
		}
	    } else {
	    	str = classname(s);
	    }
    }
    return str;
}

/* reverses the indices from the expr_list; should be folded into buildaref
 * and done as one recursive routine
 */
Node private rev_index(here,n)
register Node here,n;
{
 
  register Node i;

  if( here == nil  or  here == n) i=nil;
  else if( here->value.arg[1] == n) i = here;
  else i=rev_index(here->value.arg[1],n);
  return i;
}

private Node fortran_buildaref(a, slist)
Node a, slist;
{
    register Symbol as;      /* array of array of .. cursor */
    register Node en;        /* Expr list cursor */
    Symbol etype;            /* Type of subscript expr */
    Node esub, tree;         /* Subscript expression ptr and tree to be built*/

    tree=a;

    as = rtype(tree->nodetype);     /* node->sym.type->array*/
    if ( not (
                   (    tree->nodetype->class == VAR
		     or tree->nodetype->class == BASED
		     or tree->nodetype->class == REF
		     or tree->nodetype->class == FVAR
			 or tree->nodetype->class == FIELD
		   )
                and as->class == ARRAY
             ) ) {
	beginerrmsg();
	prtree(a);
	printf(" is not an array");
	/*printf(" a-> %x as %x ", tree->nodetype, as ); OUT*/
	enderrmsg();
    } else {
	for (en = rev_index(slist,nil); en != nil and as->class == ARRAY;
                     en = rev_index(slist,en), as = as->type) {
	    esub = en->value.arg[0];
	    etype = rtype(esub->nodetype);
            assert(as->chain->class == RANGE);
	    if ( not compatible( t_int, etype) ) {
		beginerrmsg();
		printf("subscript ");
		prtree(esub);
		printf(" is type %s ",symname(etype->type) );
		enderrmsg();
	    }
	    tree = build(O_INDEX, tree, esub);
	    tree->nodetype = as->type;
	}
	if (en != nil or
             (as->class == ARRAY && (not istypename(as->type,"char"))) ) {
	    beginerrmsg();
	    if (en != nil) {
		printf("too many subscripts for ");
	    } else {
		printf("not enough subscripts for ");
	    }
	    prtree(tree);
	    enderrmsg();
	}
    }
    return tree;
}

/*
 * Evaluate a subscript index.
 */

private int fortran_evalaref(s, i)
Symbol s;
long i;
{
    Symbol r;
    long lb, ub;

    r = rtype(s)->chain;
    lb = get_lowbound(s, r);
    ub = get_upbound(s, r);

    if (i < lb) {
	error("subscript out of range");
    } else if ((i > ub) and (r->symvalue.rangev.uppertype != R_ADJUST)) {
	error("subscript out of range");
    }
    return (i - lb);
}

private fortran_printarray(a)
Symbol a;
{
    struct Bounds { int lb, val, ub } dim[MAXDIM];

    Symbol sc,st,eltype;
    char buf[50];
    char *subscr;
    int i,ndim,elsize, first;
    Stack *savesp;
    Boolean done;

    st = a;

    sp -= size(a);
    savesp = sp;
    ndim=0;

    for(;;){
	sc = st->chain;
	if(sc->symvalue.rangev.uppertype == R_ADJUST) {
	    error("can't print array of unknown size");
	}
	dim[ndim].lb = get_lowbound(a, sc);
	dim[ndim].ub = get_upbound(a, sc);

	ndim ++;
	if (st->type->class == ARRAY) st=st->type;
	else break;
    }

    if(istypename(st->type,"char")) {
	eltype = st;
	ndim--;
    }
    else eltype=st->type;
    elsize=size(eltype);
    sp += elsize;
    /*printf("ndim %d elsize %lx in fortran_printarray\n",ndim,elsize);OUT*/

    ndim--;
    for (i=0;i<=ndim;i++){
	dim[i].val=dim[i].lb;
	/*OUT printf(" %d %d %d \n",i,dim[i].lb,dim[i].ub);
	fflush(stdout); OUT*/
    }


    first = 1;
    for(;;) {
	buf[0]=',';
	subscr = buf+1;

	for (i=ndim-1;i>=0;i--)  {
	    sprintf(subscr,"%d,",dim[i].val);
	    subscr += strlen(subscr);
	}
	*--subscr = '\0';

	for(i=dim[ndim].lb;i<=dim[ndim].ub;i++) {
	    if( first == 0 )
		indent( indentlevel );
	    else
		first = 0;
	    printf("[%d%s]\t",i,buf);
	    p_str_arr_val(eltype, elsize);
	    printf("\n");
	    sp += 2*elsize;
	}
	dim[ndim].val=dim[ndim].ub;

	i=ndim-1;
	if (i<0) break;

	done=false;
	do {
	    dim[i].val++;
	    if(dim[i].val > dim[i].ub) { 
		dim[i].val = dim[i].lb;
		if(--i<0) done=true;
	    }
	    else done=true;
	} while (not done);

	if (i<0) break;
    }
    sp = savesp;
}


#define MAX_CONSTARGS	10

struct	constarg {
	Node	node;			/* Some constant node */
	Symbol	arg;			/* Description of the arg */
	Address pushpoint;		/* Address on stack where the actual */
					/* value will be pushed */
};

/*
 * Evaluate arguments left-to-right.
 */
private fortran_evalparams(proc, arglist)
Symbol proc;
Node arglist;
{
    Node p;
    Node exp;
    Symbol arg;
    Symbol t;
    Stack *savesp;
    Integer ntypesize, argsize, i;
    struct constarg consts[MAX_CONSTARGS];
    struct constarg *cp;
    int nconsts;
    Address stackpointer;
    float f;

    savesp = sp;
    stackpointer = reg(REG_SP);
    nconsts = 0;
    arg = proc->chain;
    for (p = arglist; p != nil; p = p->value.arg[1]) {
	if (arg == nil) {
	    sp = savesp;
	    error("too many parameters to %s", symname(proc));
	}
	if ((rtype(arg))->class == CARRAY) {
	    sp = savesp;
	    error("Can't currently call with a conformant array parameter");
	}
	exp = p->value.arg[0];
	/* DT Jan 11 1987 fix problem of dimensioned FORTRAN arrays
	   causing size mismatch errors */
	if ((rtype(arg))->class == ARRAY) {
		ntypesize = sizeof(Word);
		argsize = sizeof(Word);
	} else {
		ntypesize = size(exp->nodetype);
		argsize = size(arg->type);
	}

	if (arg->class == REF) {
	    if (exp->op != O_RVAL) {
		if (nconsts > MAX_CONSTARGS) {
		    sp = savesp;
		    error("Too many constant arguments - max is %d",
		      MAX_CONSTARGS);
		}
		cp = &consts[nconsts];
		nconsts++;
		cp->node = exp;
		cp->arg = arg;
		if (exp->op == O_SCON) {
		    cp->pushpoint = stackpointer-round(ntypesize, sizeof(short));
		} else {
		    cp->pushpoint = stackpointer - argsize;
		}
		stackpointer = cp->pushpoint;
	    } else if (ntypesize != argsize) {
	    	    warning("expression for parameter %s may be wrong type",
	      	      symname(arg));
	    }
	} else if (ntypesize != argsize) {
	    warning("expression for parameter %s may be wrong type",
	      symname(arg));
	}
	arg = arg->chain;
    }
    if (arg != nil) {
	sp = savesp;
	error("not enough parameters to %s", symname(proc));
    }
    nconsts = 0;
    arg = proc->chain;
    for (p = arglist; p != nil; p = p->value.arg[1]) {
	exp = p->value.arg[0];
	ntypesize = size(exp->nodetype);
	if (arg->class == REF) {
	    if (exp->op != O_RVAL) {
		push(Address, consts[nconsts].pushpoint);
		nconsts++;
	    } else {
		push(Address, lval(exp->value.arg[0]));
	    }
	} else {
	    eval(exp);
	    if (ntypesize < sizeof(long)) {
		i = popsmall(exp->nodetype);
	        push(long, i);
	    }
	}
	arg = arg->chain;
    }

    /*
     * Look for any string arguments.  String arguments have
     * an implicit extra argument after all the rest.
     */
    arg = proc->chain;
    for (p = arglist; p != nil; p = p->value.arg[1]) {
	t = rtype(arg);
	ntypesize = size(exp->nodetype);

	if (t->class == ARRAY and istypename(t->type,"char") ) {
	    exp = p->value.arg[0];
	    if (exp->op == O_SCON) {
		push(long, strlen(exp->value.scon));
	    } else {
		t = rtype(exp->nodetype)->chain;
		push(long, get_upbound(exp->nodetype, t));
	    }
	}
	arg = arg->chain;
    }

    /*
     * Assign any constants to their temporary locations.
     */
    for (i = nconsts - 1; i >= 0; i--) {
	cp = &consts[i];
	eval(cp->node);
	t = rtype(cp->arg);
	if (isfloat(t) and size(t) == sizeof(float)) {
	    f = pop(double);
	    push(float, f);
	}
    }
}
