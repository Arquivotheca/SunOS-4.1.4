#ifndef lint
static	char sccsid[] = "@(#)symtab.c 1.1 94/10/31 SMI";
#endif

#include "cpass1.h"

/*
 * procedural interface to symbol table
 */

/* export */

char	*pcc_sname(/*sym*/);
int	pcc_sclass(/*sym*/);
TWORD	pcc_stype(/*sym*/);
int	pcc_soffset(/*sym*/);
int	pcc_ssize(/*sym*/);
int	pcc_salign(/*sym*/);
int	pcc_slevel(/*sym*/);
LIST	*pcc_symrefs(/*sym*/);
void	pcc_setsymrefs(/*sym,listp*/);
void	pcc_setvarno(/*sym,varno*/);
int	pcc_svarno(/*sym*/);
int	pcc_unknown_control_flow(/*sym*/);
int	pcc_makes_regs_inconsistent(/*sym*/);
void	pcc_set_no_reg(/*sym*/);
int	pcc_no_reg(/*sym*/);

char *
pcc_sname(sym)
    struct symtab *sym;
{
    return sym->sname;
}

int
pcc_sclass(sym)
    struct symtab *sym;
{
    return sym->sclass;
}

/*
 * type of symbol, filtered for IR; mimic enumconvert
 */
TWORD
pcc_stype(sym)
    struct symtab *sym;
{
    TWORD type, btype;

    type = sym->stype;
    btype = BTYPE(type);
    if( btype == ENUMTY || btype == MOETY ) {
	switch( dimtab[ sym->sizoff ] ) {
	case SZCHAR:
	    btype = CHAR;
	    break;
	case SZSHORT:
	    btype = SHORT;
	    break;
	case SZINT:
	    btype = INT;
	    break;
	default:
	    btype = LONG;
	    break;
	}
	btype = ctype(btype);
	MODTYPE(type, btype);
    }
    return type;
}

OFFSZ
pcc_soffset(sym)
    struct symtab *sym;
{
    return sym->offset;
}

/*
 * size of object described by symbol.  This would be just
 * a call to tsize(), except for the following:
 *	extern struct undefined a[];
 *	char *p;
 *	...
 *	p = (char*)a;
 * In this case, you are allowed to take the address of a[0]
 * even though the size of a[0] is unknown.  If you ever really
 * need to know the size of a[0], you won't be going through
 * here, and tsize will complain.
 *
 * This routine must emulate tsize, but without bitching about
 * forward references.
 */
int
pcc_ssize(sym)
    struct symtab *sym;
{
    TWORD ty;
    int d;
    int s;
    int i;
    OFFSZ mult;

    ty = sym->stype;
    d = sym->dimoff;
    s = sym->sizoff;
    mult = 1;

    for( i=0; i<=(SZINT-BTSHIFT-1); i+=TSHIFT ){
	switch( (ty>>i)&TMASK ){
	case FTN:
	    return( 0 );
	case PTR:
	    return( SZPOINT * mult );
	case ARY:
	    mult *= (unsigned int) dimtab[ d++ ];
	    continue;
	case 0:
	    break;
	}
    }
    if( dimtab[s]==0 ) {
	return( SZINT );
    }
    return( (int) dimtab[ s ] * mult );
}

int
pcc_salign(sym)
    struct symtab *sym;
{
    return talign(sym->stype, sym->sizoff);
}

int
pcc_slevel(sym)
    struct symtab *sym;
{
    return sym->slevel;
}

LIST*
pcc_symrefs(sym)
    struct symtab *sym;
{
    return sym->symrefs;
}

void
pcc_setsymrefs(sym, listp)
    struct symtab *sym;
    LIST *listp;
{
    sym->symrefs = listp;
}

int
pcc_svarno(sym)
    struct symtab *sym;
{
    return sym->svarno;
}

void
pcc_setvarno(sym, varno)
    struct symtab *sym;
    int varno;
{
    sym->svarno = varno;
}

int
pcc_unknown_control_flow(sym)
    struct symtab *sym;
{
    return sym->sflags&S_UNKNOWN_CONTROL_FLOW;
}

int
pcc_makes_regs_inconsistent(sym)
    struct symtab *sym;
{
    /* it's not pcc's fault, it's setjmp's */
    return sym->sflags&S_MAKES_REGS_INCONSISTENT;
}

void
pcc_set_no_reg(sym)
    struct symtab *sym;
{
    /* mark this name ineligible to reside in a register */
    sym->sflags |= S_NO_REG;
}

int
pcc_no_reg(sym)
    struct symtab *sym;
{
    /* returns 1 if name is ineligible to reside in a register */
    return ((sym->sflags&S_NO_REG) != 0);
}
