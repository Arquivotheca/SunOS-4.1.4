#ifndef lint
static	char sccsid[] = "@(#)struct.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include "cpass2.h"
#include "ctype.h"

/*
 * structure copying can be done by
 * either straight-line code or by a loop
 * this constant determines the cutoff point.
 * If nxfer > NLOOP ld/st pairs are necessary,
 * then looping is done, else straight-line code
 * is emitted.
 */
#define NLOOP	6

void
stmove( p , cookie, use_regpairs ) register NODE *p;
{
	NODE *from, *to;
	register size, i;
	extern NODE resc[];
	int labl;
	int argreg;
	char *cnt, *lhs, *rhs;
	char *xfer, *xfer2;
	char *ld, *st;
	int  xfersize; /* 1, 2, or 4 bytes */
	int  nxfer;    /* number of "whole" transfers necessary */

	size = p->stn.stsize;
	to = p->in.left;
	from = p->in.right;
	/* memory-memory move */

	xfer = rnames[resc[0].tn.rval];
	rhs  = rnames[from->tn.rval];
	lhs  = rnames[to->tn.rval];
	if (malign_flag) {
		p->stn.stalign = ALCHAR/SZCHAR;
	}
	switch( p->stn.stalign ){
	case ALCHAR/SZCHAR:  
		/* byte-aligned struct */
		ld = "ldub"; st="stb"; xfersize = 1;
		break;
	case ALSHORT/SZCHAR: 
		/* short-aligned struct */
		ld = "lduh"; st="sth"; xfersize = 2;
		break;
	case ALINT/SZCHAR: 
		/* word-aligned struct */
		ld = "ld";   st="st";  xfersize = 4;
		break;
	case ALDOUBLE/SZCHAR: 
		/* double-aligned struct */
		if (dalign_flag && use_regpairs) {
			ld = "ldd";   st="std";  xfersize = 8;
		} else {
			ld = "ld";   st="st";  xfersize = 4;
		}
		break;
	default:             
		cerror("insane structure alignment");
		/*NOTREACHED*/
	}
	nxfer = size/xfersize;
	/*
	 * now we make a totally arbitrary decision about
	 * when to loop, and when to emit straight-line code.
	 */

	if (nxfer > 0 && nxfer <= NLOOP ){
	    /* straight-line, double-buffered */

	    xfer2 = rnames[resc[1].tn.rval];
	    i = 0;
	    /* prime the pump */
	    printf("	%s	[%s],%s\n", ld, rhs, xfer );
	    i += xfersize;
	    size -= xfersize;
	    /* the double-buffering, main transfer */
	    while ( size >= 2*xfersize){
		printf("	%s	[%s+%d],%s\n", ld, rhs, i, xfer2 );
		printf("	%s	%s,[%s+%d]\n	%s	[%s+%d],%s\n",
		    st, xfer,lhs,i-xfersize,
		    ld, rhs,i+xfersize,xfer);
		printf("	%s	%s,[%s+%d]\n", st, xfer2, lhs, i );
		i += 2*xfersize;
		size -= 2*xfersize;
	    }
	    if ( size >= xfersize ){
		printf("	%s	[%s+%d],%s\n", ld, rhs, i, xfer2 );
		printf("	%s	%s,[%s+%d]\n", st, xfer, lhs, i-xfersize);
		printf("	%s	%s,[%s+%d]\n", st, xfer2, lhs, i);
		i += xfersize;
		size -= xfersize;
	    } else {
		printf("	%s	%s,[%s+%d]\n", st, xfer, lhs, i-xfersize);
	    }
	} else if (nxfer > NLOOP){
	    /* loop code */

	    i = size - (size % xfersize); /* iterations of object loop */
	    size %= xfersize; /* leftover bits */
	    cnt = rnames[resc[1].tn.rval];
	    genimmediate( i, resc[1].tn.rval );
	    print_label(labl= getlab());
	    /* now emit the 4-instruction loop */
	    printf("	subcc	%s,%d,%s\n", cnt, xfersize, cnt );
	    printf("	%s	[%s+%s],%s\n", ld, rhs, cnt, xfer);
	    printf("	bne	L%d\n", labl );
	    printf("	%s	%s,[%s+%s]\n", st, xfer,lhs, cnt);

	} else {
	    /* nxfer == 0 and we just handle it with the clean up code ... */
	    i = 0;
	}
	while (size>0){
	    /* do special cases */
	    switch (size){
	    case 1:
		ld = "ldub"; st = "stb";
		xfersize = 1;
		break;
	    case 2:
	    case 3:
		ld = "lduh"; st = "sth";
		xfersize = 2;
		break;
	    default:
		ld = "ld"; st = "st";
		xfersize = 4;
		break;
	    }
	    printf("	%s	[%s+%d],%s\n	%s	%s,[%s+%d]\n",
		ld,rhs,i,xfer,st,xfer,lhs,i);
	    i += xfersize;
	    size -= xfersize;
	}
	if (size != 0 )
	    cerror("insane structure size");

}

/*
 * move n bytes of (usually) struct contents
 * into registers starting at "to"
 */
move_to_regs( p, n, to )
    register NODE *p;
    register n;
{
    char **toreg, **fromreg;
    char *ptr;
    int i;

    toreg  = rnames+to;
    if (p->tn.op == REG && p->tn.type == STRTY){
	/* oh good, the easy case -- register-register moves */
	fromreg = rnames+p->tn.rval;
	while ( n > 0){
	    printf("	mov	%s,%s\n", *fromreg, *toreg );
	    fromreg++;
	    toreg++;
	    n -= 4;
	}
    } else {
	/*
	 * have an address or pointer. Evaluate it into a reg
	 * if not already there, and do loads through it.
	 * This is really crude (OREGs could be handled much better)
	 * but will give us a first approximation.
	 */
	if (!ISPTR(p->in.type)){
	    /* yeucch -- its a name or something */
	    incref(p);
	}
	if (p->tn.op != REG){
	    order( p, INTAREG );
	}
	ptr = rnames[p->tn.rval];
	/* now do loads */
	i = 0;
	while ( n > 0 ){
	    printf("	ld	[%s+%d],%s\n", ptr, i, *toreg );
	    toreg++;
	    i += 4;
	    n -= 4;
	}
    }
}

/*
 * move n bytes of (usually) struct contents
 * out of registers starting at "frm"
 */
move_from_regs( p, n, frm )
    register NODE *p;
    register n;
{
    char **toreg, **fromreg;
    char *ptr;
    int i;

    fromreg  = rnames+frm;
    if (p->tn.op == REG){
	/* oh good, the easy case -- register-register moves */
	toreg = rnames+p->tn.rval;
	while ( n > 0){
	    printf("	mov	%s,%s\n", *fromreg, *toreg );
	    fromreg++;
	    toreg++;
	    n -= 4;
	}
    } else {
	/*
	 * have an address or pointer. Evaluate it into a reg
	 * if not already there, and do stores through it.
	 * This is really crude (OREGs could be handled much better)
	 * but will give us a first approximation.
	 */
	if (!ISPTR(p->in.type)){
	    /* yeucch -- its a name or something */
	    incref(p);
	}
	if (p->tn.op != REG){
	    order( p, INTAREG );
	}
	ptr = rnames[p->tn.rval];
	/* now do stores */
	i = 0;
	while ( n > 0 ){
	    printf("	st	%s,[%s+%d],%s\n", *fromreg, ptr, i );
	    fromreg++;
	    i += 4;
	    n -= 4;
	}
    }
}
