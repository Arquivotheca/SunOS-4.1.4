#ifndef lint
static	char sccsid[] = "@(#)debug.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 *  Routines to help debug the debugger
 */

#include "defs.h"
#include "tree.h"
#include "operators.h"
#include "eval.h"
#include "events.h"
#include "symbols.h"
#include "scanner.h"
#include "source.h"
#include "object.h"
#include "mappings.h"
#include "names.h"
#include "process.h"
#include "machine.h"
#include <signal.h>


#define NDBFLAGS 20
public int debug_flag[ NDBFLAGS ];

#ifdef DEBUGGING

char LookingFor[80];
int xLookingFor;

#endif


/*
 * dbxdebug command --
 * "dowhat" is a string describing what to do,
 * "optstring" is a possible string argument to that,
 * and "towhom" is an integer which can be a debug_flag number,
 * a symbol pointer, or any of several other things.
 *
 * If dowhat is omitted, it is taken to mean "set a flag".
 * What you can do:
 *	events -- dump the list of events in internal form
 * 	help -- tell what you can do
 *	flag -- set a flag
 *	funcs -- dump the function table
 *	funclocs -- dump all of the symbols of a given function
 *	readsym[s] -- when readsyms sees the symbol, it will
 *		call _readsyms_hook.
 *	sym -- dump a specific symbol
 *	type -- follow the type chain of a symbol
 *
 * Currently meaningful flags:
 *	0 is to remain unused:  towhom == 0 means it wasn't supplied.
 *	1 --> `tree.c`build gets verbose about what nodes it has built.
 *	2 --> `eval.c`eval gets verbose about what nodes it is evaluating.
 */
public dbxdebug( dowhat, optstring, towhom )
char *dowhat, *optstring;
int towhom;
{
    char *doeswhat;
    Name np;
    Symbol sp;
    Boolean verbose, gonly;

    if( dowhat == nil ) {
	if( towhom == 0 ) goto usage;
	dowhat = "flag" ;	/* set a flag */
    }

    printf( "Debugging dbxdebug:  dowhat is <%s>, towhom is %d (0x%x).\n",
	dowhat, towhom, towhom );

    if( streq( dowhat, "flag" ) ) {

	switch( towhom ) {
	 case 1: doeswhat = "build prints nodes"; break;
	 case 2: doeswhat = "eval prints nodes"; break;
	 default:  doeswhat = "unknown debug code"; break;
	}

	if(  towhom >= 0  and  towhom < NDBFLAGS ) {
	    if(debug_flag[towhom])  debug_flag[towhom]=0;
	    else debug_flag[towhom] =1;
	    printf("DBXdebug flag %d (%s) is %d \n", towhom,
		doeswhat, debug_flag[towhom]);
	} else {
	    printf(" unknown debug code %ld \n", towhom);
	}
	return;
    }

    if( optstring ) {
	np = identname( optstring, false ); /* allocate a name */
    } else {
	np = nil;
    }

    verbose = false;
    if( *dowhat == 'v' ) {
	verbose = true;
	++dowhat;
    }

    gonly = false;
    if( streq( dowhat, "gfuncs" ) ) {
	gonly = true;
	++dowhat;
    }

    if( streq( dowhat, "events" ) ) {
	dumpeventlist( towhom, verbose );
	return;
    }

    /*
     * Set up towhom as the argument (usually a symbol
     * pointer) for any of the following dbxdebug commands.
     */
    if( towhom == 0 ) {
	if( np ) {
	    towhom = (int)which( np );
	} else {
	    towhom = 0;
	}
    }

    if( streq( dowhat, "funcs" ) ) {
	if( towhom ) {
	    psym( towhom, true );
	} else {
	    dumpfunctab( verbose, gonly );
	}
	return;
    }

	if (streq (dowhat, "syms")) {
		dump_syms();
		return;
	}

    if( streq( dowhat, "funclocs" ) ) {
	if( towhom ) {
	    dump_fcn_locals( towhom, verbose );
	} else {
	    dumpfunctab( false );
	}
	return;
    }

    if( streq( dowhat, "sym" ) || streq( dowhat, "symtype" ) ) {
	if( towhom ) {
	    psym( towhom, verbose );
	} else {
	    printf( "no symbol." );
	    goto usage;
	}
	return;
    }

#ifdef DEBUGGING
    if( streq( dowhat, "readsym" )  ||  streq( dowhat, "readsyms" ) ) {
	if( np ) {
	    strcpy( LookingFor, optstring );
	} else {
	    xLookingFor = towhom;
	}
	return;
    }
#endif DEBUGGING

 usage:
    /* Anything else --> */
    printf( "events -- dump the list of events in internal form\n" );
    printf( "flag -- set a flag:" );
		printf( "  meaningful flags are\n" );
		printf( "\t1 -->`tree.c`build gets verbose about what nodes it has built.\n" );
		printf( "\t2 -->`eval.c`eval gets verbose about what nodes it is evaluating.\n" );
    printf( "funcs -- dump the function table\n" );
    printf( "funclocs -- dump all of the symbols of a given function\n" );
    printf( "readsym[s] -- when readsyms sees the symbol, it will " );
    		printf( "call _readsyms_hook.\n" );
    printf( "sym -- dump a specific symbol\n" );
    printf( "syms -- dump all symbols\n" );
    printf( "type -- follow the type chain of a symbol\n" );
    return;
}

/*
 * Dump a tree recursively
 */
public dumptree(p)
Node p;
{

    r_dumptree( p );
    printf( "\n" );	/* make sure it is finally followed by newline */
}

public r_dumptree(p)
register Node p;
{
    register Node q;
    Operator op;
    static recurse  =0;
    ++recurse;

    if (p != nil) {
	op = p->op;
	if (ord(op) > ord(O_LASTOP)) {
	    /* Was : panic("bad op %d in r_dumptree", p->op); */
	    printf( "******** bad op %d in r_dumptree\n", p->op);
	    --recurse;
	    return;
	}
        { int n_args;
	  printf("\n level %d op %s node %ld (0x%lx) ",
	    recurse, opname(op), p, p );
          for(n_args=0;n_args < nargs(op); n_args++)
            printf(" arg%d %ld (0x%lx) ", n_args,
	      p->value.arg[n_args], p->value.arg[n_args]);
          printf("\n");
        }
        if(p->nodetype) {printf("nodetype: "); psym(p->nodetype);}
	switch (op) {
	    case O_NAME:
		printf("%s", idnt(p->value.name));
		break;

	    case O_SYM:
		printname(p->value.sym);
		break;

	    case O_QLINE:
		if (nlhdr.nfiles > 1) {
		    r_dumptree(p->value.arg[0]);
		    printf(":");
		}
		r_dumptree(p->value.arg[1]);
		break;

	    case O_LCON:
		if (compatible(p->nodetype, t_char)) {
		    printf("'%c'", p->value.lcon);
		} else {
		    printf("%d", p->value.lcon);
		}
		break;

	    case O_FCON:
		printf("%.18g", p->value.fcon);
		break;

	    case O_SCON:
		printf("\"%s\"", p->value.scon);
		break;

	    case O_INDEX:
		r_dumptree(p->value.arg[0]);
		printf("[");
		r_dumptree(p->value.arg[1]);
		printf("]");
		break;

	    case O_COMMA:
		r_dumptree(p->value.arg[0]);
		if (p->value.arg[1] != nil) {
		    printf(", ");
		    r_dumptree(p->value.arg[1]);
		}
		break;

	    case O_RVAL:
		if (p->value.arg[0]->op == O_SYM) {
		    printname(p->value.arg[0]->value.sym);
		} else {
		    r_dumptree(p->value.arg[0]);
		}
		break;

	    case O_ITOF:
		r_dumptree(p->value.arg[0]);
		break;

	    case O_CALL:
		r_dumptree(p->value.arg[0]);
		if (p->value.arg[1]!= nil) {
		    printf("(");
		    r_dumptree(p->value.arg[1]);
		    printf(")");
		}
		break;

	    case O_INDIR:
		q = p->value.arg[0];
		if (isvarparam(q->nodetype)) {
		    r_dumptree(q);
		} else {
		    if (q->op == O_SYM or q->op == O_LCON or q->op == O_DOT) {
			r_dumptree(q);
			printf("^");
		    } else {
			printf("*(");
			r_dumptree(q);
			printf(")");
		    }
		}
		break;

	    case O_DOT:
		q = p->value.arg[0];
		if (q->op == O_INDIR) {
		    r_dumptree(q->value.arg[0]);
		} else {
		    r_dumptree(q);
		}
		printf(".%s", symname(p->value.arg[1]->value.sym));
		break;

	    default:
		switch (degree(op)) {
		    case BINARY:
			r_dumptree(p->value.arg[0]);
			printf("%s", opname(op));
			r_dumptree(p->value.arg[1]);
			break;

		    case UNARY:
			printf("%s", opname(op));
			r_dumptree(p->value.arg[0]);
			break;

		    default:
                        if(degree(op) < ord(O_LASTOP) )
                        {      int i;
                               if( nargs(op)  != 0)
                                 for(i=0;i<nargs(op);i++)
                                  r_dumptree(p->value.arg[i]);
			}
			else
                          error("internal error: bad op %d in r_dumptree", op);
		}
		break;
	}
    }
    --recurse;
    fflush(stdout);
}


#ifdef DEBUGGING

_readsyms_hook ( name, inx )  char *name; {
  char *col_p;

	if( xLookingFor ) {
	    if( inx == xLookingFor ) {
		printf( "_readsyms_hook found symbol with index %d\n",
			inx );
	    }
	} else if( LookingFor[0]  &&  name ) {
	    col_p = index( name, ':' );
	    if( col_p ) *col_p = 0;
	    if( streq( name, LookingFor )  ||  streq( name+1, LookingFor ) ) {
		printf( "_readsyms_hook found symbol #%d, named <%s>\n",
			inx, name );

		if( col_p ) printf( "The rest is <%s>.\n", col_p+1 );
		else        printf( "Not a STAB.\n" );
		fflush( stdout );
		(void) index( "trashjunk", 'k' );	/* junk line */
	    }
	    /* Clean up after self */
	    if( col_p ) *col_p = ':';
	}

}

#endif DEBUGGING
