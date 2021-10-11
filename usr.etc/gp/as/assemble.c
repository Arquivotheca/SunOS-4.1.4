#ifndef lint
static	char sccsid[] = "@(#)assemble.c 1.1 94/10/31 SMI";
#endif

/* Microassembler instruction builder */

#include <stdio.h>
#include "micro.h"
#define CHKRNG(x) if(x<0){ error("RAM address %d out of range", x); return; }
#define COUNT_OPERANDS(x,y,z) if (z<x||z>y){ if (x==y) error("Expected %d operands, got %d", x,z); else error("Expected %d to %d operands, got %d", x,y,z); return;}
#define SRCDST_DEFAULT 0
#define NO (-100)

extern NODE     n[NNODE + 1];
extern NODE    *curnode;
extern int      curlineno;
extern int      curaddr;
extern char    *curfilename;
extern char    *curline;

#ifdef VIEW
extern Boolean  aflt();

short           srcdesttab[9][14] =
/*               a   s   f   f   n   l   b   s   p   f   f   f   f   f */
/*               m   h   p   p   r   e   r   h   r   i   p   p   p   l */
/*                   m   r   r   e   d   r   m   o   f   a   b   d   1 */
/*                   e   e   e   g   r   e   e   m   o   p   p   p   r */
/*                   m   g   g       e   g   m   p   1               e */
/*                   p   h   l       g       p                       g */

/*am      */ {{  7, 10, 11, 11,  3,  2,  8, 15,  9,  6, 12, 13, 14,  5 },
/*shmem   */  { 23, -1, 27, 27, -1, -1, 24, 31, 25, 22, 28, 29, 30, -1 },
/*fpregh  */  { 55, 58, 59, -1, -1, -1, 56, 63, 57, 54, 60, 61, 62, -1 },
/*fpregl  */  { 55, 58, -1, 59, -1, -1, 56, 63, 57, 54, 60, 61, 62, -1 },
/*fifo2   */  { 17, -1, -1, -1, -1, -1, 18, 21, 19, -1, -1, -1, -1, -1 },
/*fpstreg */  { 49, 50, 51, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
/*prom    */  { 33, 34, 35, 35, -1, -1, -1, -1, -1, 32, -1, -1, -1, -1 },
/*swfl2reg*/  {  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
/*GENERAL */  { 39, 42, 43, 43, -1, -1, 40, 47, 41, 38, 44, 45, 46, 37 }};
#endif
#ifdef PAINT
short		 srcdesttab[20][10] =
/*              a   s   f   z   v   p   v   m   f  G */
/*              m   c   i   b   r   p   s   u   l  E */
/*                  r   f   r   d   p   t   l   1  N */
/*                  m   o   d   r   r   r   r   r  E */
/*                  e   1   r   e   o   e   e   e  R */
/*                  m       e   g   m   g   s   g  A */
/*                          g                      L */
/*am     */ {{  9, 89, 97, 33, 57, 25, 65,121,  1, 73},
/*scrmem */  { 10, -1, -1, -1, -1, 26, 66,122, -1, 74},
/*nreg   */  {  3, -1, -1, -1, -1, -1, -1, -1, -1, -1},
/*stlreg */  {  2, -1, -1, -1, -1, -1, -1, -1, -1, -1},
/*brreg  */  {  4,116,100, 36, -1, -1, -1, -1, -1, 68},
/*scrmemp*/  {  5,117,101, 37, -1, -1, -1, -1, -1, 69},
/*pppromp*/  {  7, -1,103, 39, -1, -1, -1, -1, -1, 71},
/*fifo2  */  {  8, 88, -1, -1, -1, 24, 64,120, -1, 72},
/*fl2reg */  {  6, -1, -1, -1, -1, -1, -1, -1, -1, 70},
/*zbwdreg*/  { 11, 91, -1, -1, -1, 27, 67,123, -1, 75},
/*vwdreg */  { 13, 93, -1, -1, 61, 29, -1,125, -1, 77},
/*mulx   */  { 14, 94, -1, -1, 62, 30, -1,126, -1, 78},
/*muly   */  { 15, 95, -1, -1, 63, 31, -1,127, -1, 79},
/*mulmode*/  { 16,112, -1, -1, -1, -1, -1, -1, -1, 80},
/*iidreg */  { 17,113, -1, -1, -1, -1, -1, -1, -1, 81},
/*vhiareg*/  { 18,114, 98, 34, -1, -1, -1, -1, -1, 82},
/*vloareg*/  { 19,115, 99, 35, -1, -1, -1, -1, -1, 83},
/*vctlreg*/  { 20, -1, -1, -1, -1, -1, -1, -1, -1, 84},
/*zbhiptr*/  { 22,118, -1, -1, -1, -1, -1, -1, -1, 86},
/*zbloptr*/  { 23,119, -1, -1, -1, -1, -1, -1, -1, 87}};
#endif

void
init_one(i)
    short           i;
{
    n[i].filename = 0;
    n[i].word1 = 0x2800;
    n[i].word2 = 0;
    n[i].word3 = 0;
    n[i].word4 = 0;
    n[i].symptr = 0;
    n[i].lineno = 0;
    n[i].line = 0;
    n[i].sccsid = False;
    n[i].filled = False;
    n[i].org_pseudo = False;
    n[i].dreg_opd = False;
    n[i].has_genl = False;
    n[i].genl_sym = False;
    n[i].imm_sym = False;
    n[i].imm_29116 = False;
#ifdef VIEW
    n[i].fp_used = False;
    n[i].no_fpstore = False;
    n[i].no_shmread = False;
    n[i].no_shminc = False;
    n[i].no_fpinc = False;
    n[i].fphilo = NOTFP;
#endif
#ifdef PAINT
    n[i].no_scrmread = False;
    n[i].no_scrminc = False;
    n[i].three_way = False;
#endif
    n[i].no_flag = False;
    n[i].status = False;
}

init_assm()
{
    short           i;

    for (i = 0; i <= 3; i++) {
	init_one(i);
    }
}

copyfill(p1, p2)
    register NODE  *p1;
    register NODE  *p2;
{
    p2->status = p1->status;
    p2->symptr = p1->symptr;
    p2->imm_29116 = p1->imm_29116;
    p2->imm_sym = p1->imm_sym;
    p2->no_promread = p1->no_promread;
#ifdef VIEW
    p2->word1 = p1->word1;
    p2->word2 = p1->word2;
    p2->word3 = p1->word3;
    p2->word4 = p1->word4;
    p2->fp_used = p1->fp_used;
    p2->fphilo = p1->fphilo;
    p2->no_fpstore = p1->no_fpstore;
    p2->no_fpinc = p1->no_fpinc;
    p2->no_shmread = p1->no_shmread;
    p2->no_shminc = p1->no_shminc;
#endif
#ifdef PAINT
    p2->no_scrmread = p1->no_scrmread;
    p2->no_scrminc = p1->no_scrminc;
#endif
}

anext()
{
    register NODE  *rp;
    int             i;

    /* if (!curnode->filled) { copyfill(curnode+2,curnode+3);
     * copyfill(curnode+1,curnode+2); (curnode+1)->fp_used =
     * curnode->fp_used; } */
    rp = ++curnode;
    if (curnode - n + 4 < NNODE) {
	init_one(curnode - n + 4);
    }
    if (curnode >= &n[NNODE]) {
	fatal("too many instructions!");
    }
    rp->filename = curfilename;
    rp->lineno = curlineno;
    rp->line = curline;
}

anop()
{
#ifdef VIEW
    Boolean         bool;

#endif

    curnode->filled = True;
    curnode->word1 = 0x280e;
    if (!curnode->imm_29116) {
	curnode->word2 = 0x71;
	curnode->word3 |= 0x4000;
    }
#ifdef VIEW
    bool = aflt(0, 0, 0, 0, 0);
#endif
}

adebug(dsw)
    int             dsw;
{
    curnode->word4 |= dsw << 6;
}

astop()
{
    curnode->word4 |= 0x20;
}

abkpt()
{
    curnode->word4 |= 0x10;
}

astatus()
{
    if (curnode->status) {
	curnode->word1 &= 0xdfff;
    }
}


assemble(rp, sc, nop, op)
    RESERVED       *rp;
    int             nop,
                    sc;
    OPERAND        *op;
{
    curnode->filled = True;
    (rp->kracker) (rp, nop, op);
    if (sc == 1) {
	if ((curnode + 1)->imm_29116) {
	    (curnode + 1)->status = True;
	} else {
	    curnode->word1 &= 0xdfff;
	}
    }
}


sor(rp, nop, op)
    register RESERVED *rp;
    int             nop;
    register OPERAND *op;
{
    /* move-like instructions -- type (quadrant) depends on the operands */
    unsigned short  bits;
    int             quad;
    struct sym     *sp;
    int             a,
                    b,
                    c;

    COUNT_OPERANDS(2, 2, nop);
    if (op->r == D) {
	curnode->no_flag = True;
    }
    if (op->r == RAM) {
	static short    ramsrc[] =
	{NO, NO, 11, 2, NO, NO, 3, NO, NO, 0, NO, NO};

	quad = 2;
	b = op->v;
	CHKRNG(b);
	a = ramsrc[(op + 1)->r];
	if ((op + 1)->r == RAM && (op + 1)->v != b) {
	    c = (op + 1)->v;
	    if ((b & 0x10) != (c & 0x10)) {
		error("r[%d] and r[%d] not in same half of ram", b, c);
		return;
	    }
	    curnode->dreg_opd = True;
	}
    } else if ((op + 1)->r == RAM) {
	static short    ramdst[] =
	{8, 6, NO, NO, 9, 10, NO, NO, 7, 4, NO, 7};

	quad = 2;
	b = (op + 1)->v;
	CHKRNG(b);
	a = ramdst[op->r];
    } else {
	static short    movsrc[] =
	{8, 6, NO, NO, 9, 10, NO, NO, 7, 4, NO, 7};
	static short    movdst[] =
	{NO, NO, NO, 0, NO, NO, 4, 5, NO, 1, NO, NO};

	quad = 3;
	a = movsrc[op->r];
	if (op->r == Z && op->v != 0)
	    goto botch;
	b = movdst[(op + 1)->r];
    }
    if (a < 0 || b < 0) {
botch:
	error("Incompatible operands");
	return;
    }
    /* assemble the instruction */
    bits = (rp->value1 << 15) | (rp->value2 << 9);
    bits |= (quad << 13) | (a << 5) | b;
    curnode->word2 |= bits >> 8;
    curnode->word3 |= bits << 8;
    if (curnode->dreg_opd) {
	curnode->word2 |= (c & 0xf) << 8;
	curnode->word1 &= 0xf7ff;
    }
    if (op->r == IMM) {
	(curnode + 1)->word2 |= (op->v >> 8) & 0xff;
	(curnode + 1)->word3 |= op->v << 8;
	(curnode + 1)->imm_29116 = True;
    } else if (op->r == SYM) {
	if ((sp = lookup(op->v)) == 0) {
	    sp = enter(op->v);
	}
	(curnode + 1)->symptr = sp;
	(curnode + 1)->imm_sym = True;
    }
    DEBUG("Sor: b/w=%d, quad=%d, op=%x, a=%x, b=%x\n", rp->value1, quad, rp->value2, a, b);
}


tor(rp, nop, op)
    register RESERVED *rp;
    int             nop;
    register OPERAND *op;
{
    /* two-operand instructions -- type (quadrant) depends on the operands */
    unsigned short  bits;
    int             quad;
    int             a,
                    b,
                    c;
    struct sym     *sp;
    static short    ramsrc[] =
    {NO, NO, 12, 8, NO, NO, NO, NO, NO, 0, NO, NO};

#define ACC_DOPE 0
#define IMM_DOPE 2
#define DEE_DOPE 3
    static short    dest[] =
    {NO, NO, NO, 0, NO, NO, 4, 5, NO, 1, NO, NO};

    COUNT_OPERANDS(3, 3, nop);
    if (op->r == D) {
	curnode->no_flag = True;
    }
    if (op->r == RAM) {
	quad = 0;
	b = op->v;
	CHKRNG(b);
	switch ((op + 1)->r) {
	  case ACC:
	    a = ACC_DOPE + ramsrc[(op + 2)->r];
	    break;
	  case IMM:
	  case Z:
	  case SYM:
	    a = IMM_DOPE + ramsrc[(op + 2)->r];
	    break;
	  default:
	    goto botch;
	}
	if ((op + 2)->r == RAM && (op + 2)->v != b) {
	    c = (op + 2)->v;
	    if ((b & 0x10) != (c & 0x10)) {
		error("r[%d] and r[%d] not in same half of ram", b, c);
	    }
	    if ((op + 1)->r == IMM || (op + 1)->r == Z || (op + 1)->r == SYM) {
		error("two-register form illegal with immediate");
	    }
	    curnode->dreg_opd = True;
	}
    } else if ((op + 1)->r == RAM) {
	quad = 0;
	b = (op + 1)->v;
	CHKRNG(b);
	switch (op->r) {
	  case D:
	    a = DEE_DOPE + ramsrc[(op + 2)->r];
	    break;
	  default:
	    goto botch;
	}
	if ((op + 2)->r == RAM && (op + 2)->v != b) {
	    c = (op + 2)->v;
	    if ((b & 0x10) != (c & 0x10)) {
		error("r[%d] and r[%d] not in same half of ram", b, c);
	    }
	    curnode->dreg_opd = True;
	}
    } else if ((op + 2)->r == RAM) {
	quad = 2;
	b = (op + 2)->v;
	CHKRNG(b);
	switch (op->r) {
	  case D:
	    switch ((op + 1)->r) {
	      case ACC:
		a = 1;
		break;
	      case IMM:
	      case Z:
	      case SYM:
		a = 5;
		break;
	      default:
		goto botch;
	    }
	    break;
	  case ACC:
	    switch ((op + 1)->r) {
	      case IMM:
	      case Z:
	      case SYM:
		a = 2;
		break;
	      default:
		goto botch;
	    }
	    break;
	}
    } else {
	quad = 3;
	switch (op->r) {
	  case D:
	    switch ((op + 1)->r) {
	      case ACC:
		a = 1;
		break;
	      case IMM:
	      case Z:
	      case SYM:
		a = 5;
		break;
	      default:
		goto botch;
	    }
	    break;
	  case ACC:
	    switch ((op + 1)->r) {
	      case IMM:
	      case Z:
	      case SYM:
		a = 2;
		break;
	      default:
		goto botch;
	    }
	    break;
	  default:
	    goto botch;
	}
	b = dest[(op + 2)->r];
    }
    /* assemble the instruction */
    if (a < 0 || b < 0)
	goto botch;
    bits = (rp->value1 << 15) | (rp->value2 << 5);
    bits |= (quad << 13) | (a << 9) | b;
    curnode->word2 |= bits >> 8;
    curnode->word3 |= bits << 8;
    if (curnode->dreg_opd) {
	curnode->word2 |= (c & 0xf) << 8;
	curnode->word1 &= 0xf7ff;
    }
    if ((op + 1)->r == IMM || (op + 1)->r == Z) {
	(curnode + 1)->word2 |= ((op + 1)->v >> 8) & 0xff;
	(curnode + 1)->word3 |= (op + 1)->v << 8;
	(curnode + 1)->imm_29116 = True;
    } else if ((op + 1)->r == SYM) {
	if ((sp = lookup((op + 1)->v)) == 0) {
	    sp = enter((op + 1)->v);
	}
	(curnode + 1)->symptr = sp;
	(curnode + 1)->imm_sym = True;
    }
    DEBUG("Tor: b/w=%d, quad=%d, op=%x, a=%x, b=%x\n", rp->value1, quad, rp->value2, a, b);
    return;
botch:
    error("incompatible operands");
#   undef ACC_DOPE
#   undef IMM_DOPE
#   undef DEE_DOPE
}


shf(rp, nop, op)
    register RESERVED *rp;
    int             nop;
    register OPERAND *op;
{
    /* single-bit shifts */
    unsigned short  bits;
    int             quad;
    int             a,
                    b,
                    c;

    COUNT_OPERANDS(2, 2, nop);
    if (op->r == D) {
	curnode->no_flag = True;
    }
    if ((op + 1)->r == RAM) {
	quad = 2;
	b = (op + 1)->v;
	CHKRNG(b);
	switch (op->r) {
	  case RAM:
	    a = 6;
	    if (op->v != b) {
		c = b;
		b = op->v;
		if ((b & 0x10) != (c & 0x10)) {
		    error("r[%d] and r[%d] not in same half of ram",
			  b, c);
		}
		curnode->dreg_opd = True;
	    }
	    break;
	  case D:
	    a = 7;
	    break;
	  default:
    botch:
	    error("incompatible operands");
	    return;
	}
    } else {
	quad = 3;
	switch (op->r) {
	  case ACC:
	    a = 6;
	    break;
	  case D:
	    a = 7;
	    break;
	  default:
	    goto botch;
	}
	switch ((op + 1)->r) {
	  case Y:
	    b = 0;
	    break;
	  case ACC:
	    b = 1;
	    break;
	  default:
	    goto botch;
	}
    }
    /* assemble the instruction */
    bits = (rp->value1 << 15) | (rp->value2 << 5);
    bits |= (quad << 13) | (a << 9) | b;
    curnode->word2 |= bits >> 8;
    curnode->word3 |= bits << 8;
    if (curnode->dreg_opd) {
	curnode->word2 |= (c & 0xf) << 8;
	curnode->word1 &= 0xf7ff;
    }
    DEBUG("Shf: b/w=%d, quad=%d, op=%x, a=%x, b=%x\n", rp->value1, quad, rp->value2, a, b);
}


bor(rp, nop, op)
    register RESERVED *rp;
    int             nop;
    register OPERAND *op;
{
    /* bit testing and clearing, and other weird stuff */
    unsigned short  bits;
    int             quad;
    int             a,
                    b,
                    c;
    int             o;
    Boolean         n_used = False;

    COUNT_OPERANDS(2, 3, nop);
    if ((op + 1)->r == D) {
	curnode->no_flag = True;
    }
    /* first operand is bit number -- must be in range */
    if (op->r == NREG) {
	n_used = True;
	a = 0;
    } else if ((op->r != Z && op->r != IMM) || (a = op->v) < 0 || a > 15) {
	error("bad bit index");
	return;
    }
    if ((op + 1)->r == RAM) {
	static short    quads[] = {3, 3, 3, 2, 2, 2, 2};
	static short    oos[] = {13, 14, 15, 14, 15, 12, 13};

	b = (op + 1)->v;
	CHKRNG(b);
	quad = quads[rp->value2];
	o = oos[rp->value2];
	if (nop == 3) {
	    if ((op + 2)->r == RAM) {
		if (b == (op + 2)->v) {
		    c = 0;
		} else {
		    c = (op + 2)->v;
		    curnode->dreg_opd = True;
		    if ((b & 0x10) != (c & 0x10)) {
			error("r[%d] and r[%d] not in same half of ram", b, c);
		    }
		}
	    } else {
		goto botch;
	    }
	}
    } else {
	static short    acc[] = {2, 1, 0, 4, 5, 6, 7};
	static short    dee[] = {18, 17, 16, 20, 21, NO, NO};
	static short    yew[] = {NO, NO, NO, NO, NO, 22, 23};

	if ((rp->value2 == 0 || rp->value2 == 1 ||
	     rp->value2 == 3 || rp->value2 == 4) && (op + 1)->r == D) {
	    if (nop != 3 || (op + 2)->r != Y) {
		goto botch;
	    }
	} else if (nop == 3) {
	    goto botch;
	}
	o = 12;
	quad = 3;
	switch ((op + 1)->r) {
	  case ACC:
	    b = acc[rp->value2];
	    break;
	  case D:
	    b = dee[rp->value2];
	    break;
	  case Y:
	    b = yew[rp->value2];
	    break;
	  default:
    botch:
	    error("incompatible operands");
	    return;
	}
	if (b < 0)
	    goto botch;
    }
    /* assemble the instruction */
    bits = (rp->value1 << 15) | (quad << 13);
    bits |= (a << 9) | (o << 5) | b;
    curnode->word2 |= bits >> 8;
    curnode->word3 |= bits << 8;
    if (n_used) {
	curnode->word1 |= 0x1000;
    }
    if (curnode->dreg_opd) {
	curnode->word2 |= (c & 0xf) << 8;
	curnode->word1 &= 0xf7ff;
    }
    DEBUG("Bor: b/w=%d, quad=%d, op=%x, a=%x, b=%x\n", rp->value1, quad, o, a, b);
}

rot(rp, nop, op)
    register RESERVED *rp;
    int             nop;
    register OPERAND *op;
{
    /* rotate by n bits */
    unsigned short  bits;
    int             quad;
    Boolean         n_used = False;
    int             a,
                    b,
                    c;
    int             o;
    static short    ramsrc[] =
    {NO, NO, 15, 14, NO, NO, NO, NO, NO, 12, NO, NO};
    static short    ramsrc2[] =
    {NO, 1, NO, NO, NO, NO, NO, NO, NO, 0, NO, NO};
    static short    xsrc[] =
    {NO, NO, NO, 24, NO, NO, NO, NO, NO, 25, NO, NO};

#   define DEE_DOPE 0
#   define ACC_DOPE 4

    COUNT_OPERANDS(3, 3, nop);
    if ((op + 1)->r == D) {
	curnode->no_flag = True;
    }
    /* first operand is bit number -- must be in range */
    if (op->r == NREG) {
	n_used = True;
	a = 0;
    } else if ((op->r != Z && op->r != IMM && op->r != SYM)
	       || (a = op->v) < 0 || a > 15) {
	error("bad bit index");
	return;
    }
    if ((op + 1)->r == RAM) {
	quad = 0;
	b = (op + 1)->v;
	CHKRNG(b);
	o = ramsrc[(op + 2)->r];
	if ((op + 2)->r == RAM && (op + 2)->v != b) {
	    c = (op + 2)->v;
	    if ((b & 0x10) != (c & 0x10)) {
		error("r[%d] and r[%d] not in same half of ram", b, c);
	    }
	    curnode->dreg_opd = True;
	}
    } else if ((op + 2)->r == RAM) {
	quad = 1;
	b = (op + 2)->v;
	CHKRNG(b);
	o = ramsrc2[(op + 1)->r];
    } else {
	quad = 3;
	o = 12;
	switch ((op + 1)->r) {
	  case D:
	    b = DEE_DOPE + xsrc[(op + 2)->r];
	    break;
	  case ACC:
	    b = ACC_DOPE + xsrc[(op + 2)->r];
	    break;
	  default:
	    goto botch;
	}
    }

    /* assemble the instruction */
    if (b < 0 || o < 0)
	goto botch;
    bits = (rp->value1 << 15) | (quad << 13);
    bits |= (a << 9) | (o << 5) | b;
    curnode->word2 |= bits >> 8;
    curnode->word3 |= bits << 8;
    if (n_used) {
	curnode->word1 |= 0x1000;
    }
    if (curnode->dreg_opd) {
	curnode->word2 |= (c & 0xf) << 8;
	curnode->word1 &= 0xf7ff;
    }
    DEBUG("Rot: b/w=%d, quad=%d, op=%x, a=%x, b=%x\n", rp->value1, quad, o, a, b);
    return;
botch:
    error("incompatible operands");
#   undef ACC_DOPE
#   undef DEE_DOPE
}


rom(rp, nop, op)
    register RESERVED *rp;
    int             nop;
    register OPERAND *op;
{
    /* rotate and xxxxx instructions */
    /* operands are: shift count; shifted source; mask; destination
     * (comparand) */
    unsigned short  bits;
    int             quad = 1;
    int             a,
                    b;
    int             o;
    struct sym     *sp;
    Boolean         n_used = False;
    register struct dee {
	short           accdst[12];
	short           ramdst[12];
	short           accram,
	                ramacc;
    }              *d;
    static struct dee darray[2] = {
				   {   /* for rotate-and-merge */
			   {NO, NO, 8, NO, NO, NO, NO, NO, 7, NO, NO, 7},
			  {NO, NO, NO, NO, NO, NO, NO, NO, 9, 10, NO, 9},
				    12, 14,
				    },
				   {   /* for rotate-and-compare */
			  {NO, NO, NO, NO, NO, NO, NO, NO, 2, NO, NO, 2},
			   {NO, NO, NO, NO, NO, NO, NO, NO, 3, 4, NO, 3},
				    NO, 5,
				    },
    };

    COUNT_OPERANDS(4, 4, nop);
    /* first operand is bit number -- must be in range */
    if (op->r == NREG) {
	n_used = True;
	a = 0;
    } else if ((op->r != Z && op->r != IMM) || (a = op->v) < 0 || a > 15) {
	error("bad bit index");
    }
    if ((op + 1)->r == D) {
	curnode->no_flag = True;
    }
    b = 0;
    d = &darray[rp->value2];
    if ((op + 1)->r == D) {
	switch ((op + 2)->r) {
	  case ACC:
	    o = d->accdst[(op + 3)->r];
	    if ((op + 3)->r == RAM) {
		b = (op + 3)->v;
	    }
	    break;
	  case RAM:
	    o = d->ramdst[(op + 3)->r];
	    b = (op + 2)->v;
	    break;
	  default:
	    goto botch;
	}
    } else if ((op + 1)->r == ACC && ((op + 3)->r == IMM || (op + 3)->r == Z
	       || (op + 3)->r == SYM)
	       && (op + 2)->r == RAM) {
	o = d->accram;
	b = (op + 2)->v;
    } else if ((op + 1)->r == RAM && ((op + 3)->r == IMM || (op + 3)->r == Z
	       || (op + 3)->r == SYM)
	       && (op + 2)->r == ACC) {
	o = d->ramacc;
	b = (op + 1)->v;
    } else {
botch:
	error("incompatible operands");
	return;
    }
    if ((op + 3)->r == IMM || (op + 3)->r == Z) {
	(curnode + 1)->word2 |= ((op + 3)->v >> 8) & 0xff;
	(curnode + 1)->word3 |= (op + 3)->v << 8;
	(curnode + 1)->imm_29116 = True;
    } else if ((op + 3)->r == SYM) {
	if ((sp = lookup((op + 3)->v)) == 0) {
	    sp = enter((op + 3)->v);
	}
	(curnode + 1)->symptr = sp;
	(curnode + 1)->imm_sym = True;
    }
    /* assemble the instruction */
    CHKRNG(b);
    if (o < 0)
	goto botch;
    bits = (rp->value1 << 15) | (quad << 13);
    bits |= (a << 9) | (o << 5) | b;
    curnode->word2 |= bits >> 8;
    curnode->word3 |= bits << 8;
    if (n_used) {
	curnode->word1 |= 0x1000;
    }
    DEBUG("Rom: b/w=%d, quad=%d, op=%x, a=%x, b=%x\n", rp->value1, quad, o, a, b);
}

etc(rp, nop, op)
    register RESERVED *rp;
    int             nop;
    register OPERAND *op;
{
    /* random, unrelated instructions */
    unsigned short  bits;
    int             quad;
    struct sym     *sp;
    int             a,
                    b,
                    c;
    int             o;
    Boolean         hasopd1;
    static short    nnops[] = {3, 1, 1, 1, 0};
    static short    pmasks[] = {10, NO, NO, NO, NO, NO, NO, NO, 11, 8, NO, 11};
    static short    pdest1[] = {NO, NO, NO, 2, NO, NO, NO, NO, NO, 0, NO, NO};
    static short    pdest2[] = {NO, NO, 11, 10, NO, NO, NO, NO, NO, 8, NO, NO};
    static short    pdest4[] = {NO, NO, NO, 0, NO, NO, NO, NO, NO, 1, NO, NO};
    static short    psrc2[] = {NO, 9, NO, NO, NO, NO, NO, NO, NO, 7, NO, NO};
    static short    psrc3[] = {NO, 6, 3, NO, NO, NO, NO, NO, NO, 4, NO, NO};
    static short    psrc4[] = {NO, 6, NO, NO, NO, NO, NO, NO, NO, 4, NO, NO};
    static short    stdst1[] = {3, 5, 6, 9, 10};
    static short    stdst2[] = {NO, NO, NO, 0, NO, NO, NO, NO, NO, 1, NO, NO};

    hasopd1 = False;
    COUNT_OPERANDS(nnops[rp->value2], nnops[rp->value2], nop);
    if (op != 0 && nnops[rp->value2] >= 1 && op->r == D) {
	curnode->no_flag = True;
    }
    switch (rp->value2) {
      case 0:			       /* prioritize instruction */
	/* three operands: source, mask, destination */
	hasopd1 = True;
	if (op->r == RAM) {
	    /* case 1: RAM source */
	    if ((op + 2)->r == RAM)
		goto case3;	       /* cases hard to tell apart */
	    quad = 2;
	    o = pdest1[(op + 2)->r];
	    a = pmasks[(op + 1)->r];
	    b = op->v;
	    CHKRNG(b);
	} else if ((op + 1)->r == RAM) {
	    /* case 2: RAM mask */
	    quad = 2;
	    o = psrc2[op->r];
	    a = pdest2[(op + 2)->r];
	    b = (op + 1)->v;
	    CHKRNG(b);
	} else if ((op + 2)->r == RAM) {
    case3:			       /* case 3: RAM destination */
	    quad = 2;
	    o = psrc3[op->r];
	    a = pmasks[(op + 1)->r];
	    b = (op + 2)->v;
	    CHKRNG(b);
	    if (op->r == RAM && op->v != b) {
		c = b;
		b = op->v;
		if ((b & 0x10) != (c & 0x10)) {
		    error("r[%d] and r[%d] not in same half of ram",
			  b, c);
		}
		if ((op + 1)->r == RAM) {
		    error("two-register form illegal with immediate");
		}
		curnode->dreg_opd = True;
	    }
	} else {
	    /* case 4: no RAM involved in transaction */
	    quad = 3;
	    o = psrc4[op->r];
	    a = pmasks[(op + 1)->r];
	    b = pdest4[(op + 2)->r];
	}
	if ((op + 1)->r == Z && (op + 1)->v != 0)
	    goto botch;
	/* assemble it */
	if (a < 0 || b < 0 || o < 0)
	    goto botch;
	bits = (rp->value1 << 15) | (quad << 13);
	bits |= (a << 9) | (o << 5) | b;
	DEBUG("Prioritize: b/w=%d, quad=%d, op=%x, a=%x, b=%x\n", rp->value1, quad, o, a, b);
	break;

      case 1:			       /* crc instructions       */
	if (op->r != RAM)
	    goto botch;
	b = op->v;
	CHKRNG(b);
	if (rp->value1)
	    o = 9;
	else
	    o = 3;
	bits = (6 << 13) + (6 << 9) + (o << 5) + b;
	DEBUG("Crc: o=%x, b=%x\n", o, b);
	break;

      case 2:			       /* status register instructions */
	curnode->word2 |= (rp->value1 == 0) ? 0x77 : 0x75;
	curnode->word3 |= 0x4000;
	curnode->word3 |= stdst1[op->r] << 8;
	break;

      case 3:			       /* status register saving
				        * instructions */
	if (op->r == RAM) {
	    if (rp->value1 == 0)
		goto rbotch;
	    bits = 0xcf40 | (op->v & 0xf);
	} else {
	    bits = ((rp->value1 == 0) ? 0x6f40 : 0xef40) |
		(stdst2[op->r] & 0xf);
	}
	break;

      case 4:			       /* noop instruction */
	bits = 0x7140;
	DEBUG("Nop\n");
	break;
      default:
	fatal("etc subop out of range");
    }
    curnode->word2 |= bits >> 8;
    curnode->word3 |= bits << 8;
    if (curnode->dreg_opd) {
	curnode->word2 |= (c & 0xf) << 8;
	curnode->word1 &= 0xf7ff;
    }
    if (hasopd1 && (op + 1)->r == IMM) {
	(curnode + 1)->word2 |= ((op + 1)->v >> 8) & 0xff;
	(curnode + 1)->word3 |= (op + 1)->v << 8;
	(curnode + 1)->imm_29116 = True;
    } else if (hasopd1 && (op + 1)->r == SYM) {
	if ((sp = lookup((op + 1)->v)) == 0) {
	    sp = enter((op + 1)->v);
	}
	(curnode + 1)->symptr = sp;
	(curnode + 1)->imm_sym = True;
    }
    return;
botch:
    error("incompatible operands");
    return;
rbotch:
    error("movsw required for ram destination");
}

#ifdef VIEW
Boolean
asrcdest(src, dest, gentype, num, name)
    int             src,
                    dest;
    SYMTYPE         gentype;
    int             num;
    char           *name;
{
    int             sd;
    struct sym     *sp;

    curnode->filled = True;
    if (curnode->no_flag && src == FL2REG && dest == AM) {
	warn("instr. moves fl2reg to am29116 & has D reg. as an am29116 operand");
    }
    if (src == -1 && dest == -1) {
	curnode->word1 |= SRCDST_DEFAULT << 4;
    } else {
	if ((sd = srcdesttab[src][dest]) == -1) {
	    error("illegal source/destination combination");
	    return False;
	} else {
	    if (src == GENERAL) {
		curnode->has_genl = True;
		if (gentype == NUMBER) {
		    curnode->word3 |= (num >> 8) & 0xff;
		    curnode->word4 |= num << 8;
		    curnode->genl_sym = False;
		} else {
		    if ((sp = lookup(name)) == 0) {
			sp = enter(name);
		    }
		    curnode->symptr = sp;
		    curnode->genl_sym = True;
		}
	    }
	    curnode->word1 |= sd << 4;
	    if (dest == FPREGH || dest == FPREGL) {
		(curnode + 1)->no_fpstore = True;
	    }
	    if (dest == SHMEM) {
		(curnode + 1)->no_shmread = True;
	    }
	    if ((src == SHMEM) && curnode->no_shmread) {
		warn("shared memory write followed by shared memory read");
	    }
	    if (dest == VPPROMP) {
		(curnode + 1)->no_promread = True;
		(curnode + 2)->no_promread = True;
	    }
	    if ((src == VPPROM) && curnode->no_promread) {
		warn("write to prom pointer followed by read from prom");
	    }
	    if (curnode->no_shminc && dest == SHMEM) {
		curnode->no_shminc = False;
	    }
	    if (curnode->no_fpinc && (dest == FPREGH || dest == FPREGL)) {
		curnode->no_fpinc = False;
	    }
	}
    }
    if (curnode->fphilo != NOTFP) {
	if ((curnode->fphilo == HIGH && (src == FPREGL || dest == FPREGL))
	    || (curnode->fphilo == LOW && (src == FPREGH || dest == FPREGH))) {
	    error("inconsistent setting of hi/lo bit");
	    return False;
	}
    }
    if (src == FPREGH || dest == FPREGH) {
	curnode->fphilo = HIGH;
    } else if (src == FPREGL || dest == FPREGL) {
	curnode->fphilo = LOW;
	curnode->word1 |= 0x400;
    }
    return True;
}
#endif
#ifdef PAINT
Boolean
asrcdest(src, dest, gentype, num, name)
    int             src,
                    dest;
    SYMTYPE         gentype;
    int             num;
    char           *name;
{
    int             sd;
    struct sym     *sp;

    curnode->filled = True;
    if (curnode->no_flag && src == FL2REG && dest == AM) {
	warn("instr. moves fl2reg to am29116 & has D reg. as an am29116 operand");
    }
    if (src == -1 && dest == -1) {
	curnode->word1 |= SRCDST_DEFAULT << 4;
    } else {
	if ((sd = srcdesttab[dest][src]) == -1) {
	    error("illegal source/destination combination");
	    return False;
	} else {
	    if (src == GENERAL) {
		curnode->has_genl = True;
		if (gentype == NUMBER) {
		    curnode->word3 |= (num >> 8) & 0xff;
		    curnode->word4 |= num << 8;
		    curnode->genl_sym = False;
		} else {
		    if ((sp = lookup(name)) == 0) {
			sp = enter(name);
		    }
		    curnode->symptr = sp;
		    curnode->genl_sym = True;
		}
	    }
	    curnode->word1 |= sd << 4;
	    if (dest == SCRMEM) {
		(curnode + 1)->no_scrmread = True;
	    }
	    if ((src == SCRMEM) && curnode->no_scrmread) {
		warn("shared memory write followed by shared memory read at location %7x", curnode - 1);
	    }
	    if (dest == PPPROMP) {
		(curnode + 1)->no_promread = True;
		(curnode + 2)->no_promread = True;
	    }
	    if ((src == PPPROM) && curnode->no_promread) {
		warn("write to prom pointer followed by read from prom at location %7x", curnode - 1);
	    }
	    if (curnode->no_scrminc && dest == SCRMEM) {
		curnode->no_scrminc = False;
	    }
	}
    }
    return True;
}
#endif

Boolean
aseq(rp, notflag, ccp, which, num, sym)
    register RESERVED *rp;
    Boolean         notflag;
    register RESERVED *ccp;
    SYMTYPE         which;
    int             num;
    char           *sym;
{
    SYMBOL         *sp;

    curnode->filled = True;
    if (rp == 0) {
	curnode->word1 |= CONTINUE;
    } else {
#ifdef VIEW
	curnode->word1 |= (rp == 0) ? -1 : rp->value1;
	curnode->word2 |=
	    (ccp == 0) ? 0 : (((int) notflag << 3) | ccp->value1) << 12;
	if (curnode->fp_used) {
	    error("multiple use of variable field");
	    return False;
	}
#endif
#ifdef PAINT
	curnode->word1 |= rp->value1;
	if (ccp != 0) {
	    if (ccp->value1 == -1) {
		curnode->word1 |= 0x8000;
	    } else {
		curnode->word2 |= (((int) notflag << 3)
				   | (ccp->value1 % 16)) << 12;
		if (ccp->value1 >= 16) {
		    if (curnode->dreg_opd) {
			error("three-way branch not allowed with two-register Am29116 instruction");
			return False;
		    }
		    curnode->three_way = True;
		}
	    }
	}
#endif
	if (which == NUMBER) {
	    if (curnode->has_genl) {
		if (curnode->genl_sym
		    || (curnode->word3 & 0xff) != ((num >> 8) & 0xff)
		    || (curnode->word4 & 0xff00) != ((num << 8))) {
		    error("multiple use of variable field");
		    return False;
		}
	    } else {
		curnode->has_genl = True;
		curnode->genl_sym = False;
		curnode->word3 |= (num >> 8) & 0xff;
		curnode->word4 |= num << 8;
	    }
	} else if (which == ALPHA) {
	    if ((sp = lookup(sym)) == 0) {
		sp = enter(sym);
	    }
	    if (curnode->has_genl) {
		if (!curnode->genl_sym
		    || (curnode->symptr != sp)) {
		    error("multiple use of variable field");
		    return False;
		}
	    } else {
		curnode->has_genl = True;
		curnode->genl_sym = True;
		curnode->symptr = sp;
	    }
	} else {
	    curnode->word1 |= 0x4000;
	}
#ifdef PAINT
	if (curnode->three_way && !curnode->has_genl) {
	    error("three-way branch requires label in 2910 inst.");
	    return False;
	}
#endif
    }
    return True;

}

#ifdef VIEW
Boolean
almode(round, inf, mode, code)
    int             round,
                    inf,
                    mode,
                    code;
{
    if (curnode->has_genl) {
	error("line has both general field value and floating point");
	return False;
    } else if (curnode->fphilo != NOTFP) {
	if ((curnode->fphilo == HIGH && mode == 1)
	    || (curnode->fphilo == LOW && mode == 0)) {
	    error("hi/lo bit inconsistent with mode operand in lmode");
	    return False;
	}
    }
    curnode->filled = True;
    curnode->word4 |= (round << 14) | 0x3000;
    curnode->word3 |= inf;
    curnode->word3 |= code << 1;
    curnode->word1 |= mode << 10;
    curnode->word1 |= 0x8000;
    return True;
}

Boolean
aflt(rp, load, unld, store, select)
    register RESERVED *rp;
    int             load,
                    unld,
                    store,
                    select;
{
    curnode->filled = True;
    if (rp != 0) {
	if (curnode->has_genl) {
	    error("general field value and floating-point");
	    return False;
	}
	if (curnode->fphilo != NOTFP) {
	    if ((curnode->fphilo == HIGH && select == 1)
		|| (curnode->fphilo == LOW && select == 0)) {
		error("inconsistent setting of hi/lo bit");
		return False;
	    }
	}
	(curnode + 1)->fphilo = (select == 0) ? LOW : HIGH;
	curnode->fp_used = True;
	curnode->word3 |= rp->value1 >> 2;
	curnode->word4 |= rp->value1 << 14;
	curnode->word4 |= rp->value2 << 11;
	curnode->word4 |= load << 12;
	curnode->word4 |= unld << 9;
	curnode->word4 |= store << 8;
	curnode->word1 |= select << 10;
	(curnode + 1)->word1 |= (!select) << 10;
	curnode->word1 |= 0x8000;
	(curnode + 1)->word1 |= 0x8000;
	/* bus move to fpregh or fpregl followed by flpt with store */
	if (curnode->no_fpstore && (curnode->word1 & 0x8000)
	    && (curnode->word4 & 0x100)) {
	    warn("move to fpregh or fpregl followed by floating point store");
	}
    }
    if ((curnode - 1)->fp_used) {
	if (curnode->fp_used) {
	    error("floating-point instructions in two consecutive lines");
	    return False;
	} else {
	    if (curnode->has_genl) {
		error("fl. pt. line followed by general field line");
		return False;
	    } else {
		curnode->word1 |= 0x8000;
		curnode->word3 |= (curnode - 1)->word3 & 0xff;
		curnode->word4 |= (curnode - 1)->word4 & 0xff00;
	    }
	}
    }
    return True;
}
#endif

#ifdef VIEW
Boolean
acontrol(ctrl, op, ctrl2)
    int             ctrl,
                    op,
                    ctrl2;
{
    curnode->filled = True;
    if (ctrl == -1) {
	return False;
    } else {
	if ((curnode->word1 & 0x800) == 0) {
	    error("two-register 29116 instruction and control field");
	    return False;
	} else {
	    if (ctrl == 1) {
		if (ctrl2 == -1) {
		    curnode->word2 |= (ctrl + op) << 8;
		} else {
		    curnode->word2 |= (8 + (op << 1) + ctrl2) << 8;
		}
	    } else {
		curnode->word2 |= ctrl << 8;
	    }
	}
	if ((curnode + 1)->no_shmread && (ctrl == 0) && (op != 0)) {
	    (curnode + 1)->no_shminc = True;
	}
	if (curnode->no_shminc && (ctrl == 0) && (op != 0)) {
	    warn("write to shared mem. with inc. or dec. followed by non-write to shared mem. with inc. or dec.");
	}
	if ((curnode + 1)->no_fpstore && (ctrl != 0)) {
	    (curnode + 1)->no_fpinc = True;
	}
	if (curnode->no_fpinc && (ctrl == 0)) {
	    warn("write to floating reg. with inc. followed by non-write to floating reg. with inc.");
	}
    }
    return True;
}
#endif
#ifdef PAINT
Boolean
acontrol(ctrl)
    int             ctrl;
{
    curnode->filled = True;
    if (ctrl == -1) {
	if (curnode->three_way) {
	    error("three-way branch requires vmerd or vmewr control field");
	    return False;
	}
	return True;
    } else {
	if ((curnode->word1 & 0x800) == 0) {
	    error("two-register 29116 instruction and control field");
	} else {
	    if (curnode->three_way) {
		if (ctrl >= 6) {
		    curnode->word2 |= (ctrl + 8) << 8;
		    curnode->word1 |= 0x4000;
		} else {
		    error("three-way branch requires vmerd or vmewr control field");
		    return False;
		}
	    } else {
		curnode->word2 |= ctrl << 8;
	    }
	}
	if ((curnode + 1)->no_scrmread && (ctrl == 0)) {
	    (curnode + 1)->no_scrminc = True;
	}
	if (curnode->no_scrminc && (ctrl == 0)) {
	    warn("write to shared mem. with inc. or dec. followed by non-write to shared mem. with inc. or dec. at location %7x", curnode - 1);
	}
    }
    return True;
}
#endif

void
asccs(cp)
    char           *cp;
{
    --cp;
    while (True) {
	curnode->filled = True;
	curnode->sccsid = True;
	curnode->addr = curaddr;
	curnode->word1 = 0;
	curnode->word2 = 0;
	curnode->word3 = 0;
	curnode->word4 = 0;
	if (*(++cp) == '\n') {
	    curnode->word1 = '\0' << 8;
	    break;
	} else {
	    curnode->word1 = *cp << 8;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curnode->word1 |= '\0';
	    break;
	} else {
	    curnode->word1 |= *cp & 0xff;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curnode->word2 = '\0' << 8;
	    break;
	} else {
	    curnode->word2 = *cp << 8;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curnode->word2 |= '\0';
	    break;
	} else {
	    curnode->word2 |= *cp & 0xff;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curnode->word3 = '\0' << 8;
	    break;
	} else {
	    curnode->word3 = *cp << 8;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curnode->word3 |= '\0';
	    break;
	} else {
	    curnode->word3 |= *cp & 0xff;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curnode->word4 = '\0' << 8;
	    break;
	} else {
	    curnode->word4 = *cp << 8;
	}
	if (*cp == '\0')
	    break;
	curlineno++;
	anext();
	curnode->line = NULL;
	curaddr++;
    }
}

resolve_addrs()
{
    NODE           *nd;
    struct sym     *sp;

    for (nd = n; nd <= curnode; nd++) {
	if (nd->has_genl && nd->genl_sym) {
	    sp = nd->symptr;
	    if (sp->defined) {
		nd->word3 |= ((sp->node->addr) >> 8) & 0xff;
		nd->word4 |= (sp->node->addr) << 8;
	    }
	} else if (nd->imm_sym) {
	    sp = nd->symptr;
	    if (sp->defined) {
		nd->word2 &= 0xff00;
		nd->word2 |= ((sp->node->addr) >> 8) & 0xff;
		nd->word3 &= 0xff;
		nd->word3 |= (sp->node->addr) << 8;
	    }
	}
    }
}
