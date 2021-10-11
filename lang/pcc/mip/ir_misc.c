#ifndef lint
static char sccsid[] = "@(#)ir_misc.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "ir_misc.h"
#include "opdescr.h"

/*
 * this source file implements a collection of basic operations
 * on IR, including generation of constants and various forms of
 * table lookup.  It uses IR storage allocation primitives (from
 * ir_alloc) and machine description parameters (from machdep.h)
 * Apart from the use of standard PCC types, it is language
 * independent and may be used in other front ends.
 */

/* from ir_alloc */
extern LEAF    *new_leaf(/*class*/);
extern BLOCK   *new_block(/*s, labelno, is_entry, is_global*/);
extern IR_NODE *new_triple(/*op,arg1,arg2,type*/);
extern char    *new_string(/*string*/);
extern char    *add_bytes(/*strp, len*/);
extern LIST    *new_list();
extern CHAIN   *new_chain(/*item,link*/);
extern SEGMENT *new_segment(/*segname, segdescr, segbase*/);

/* export */

LIST    *leaf_hash_tab[LEAF_HASH_SIZE];
CHAIN	*seg_hash_tab[SEG_HASH_SIZE];
LEAF	*lookup_leaf(/*class,type,location,insert*/);
IR_NODE *ir_auto_var(/*name, segoffset, seglen, segalign, offset, type*/);
IR_NODE *ir_param_var(/*name, offset, type*/);
IR_NODE *ir_reg_var(/*name, regno, type*/);
IR_NODE *ir_static_var(/*name, labelno, offset, type*/);
IR_NODE *ir_extern_var(/*name, offset, type*/);
IR_NODE *ir_funcname(/*name, type*/);
IR_NODE *ir_addrcon(/*np, offset, type*/);
void	ir_mkpoint(/*np, type*/);
void	ir_mkoverlap(/*leaf1, leaf2*/);
void	ir_mkneighbor(/*leaf1, leaf2*/);
void	fill_can_access();
IR_NODE *ir_fcon(/*dval,type*/);
IR_NODE *ir_icon(/*i,type*/);
IR_NODE *ileaf(/*i*/);
IR_NODE *cleaf(/*s*/);
void    ir_pass(/*s*/);
void    ir_labeldef(/*lab*/);
void    ir_branch(/*n*/);
BOOLEAN same_irtype(/*t1,t2*/);
void	ir_init(/*procname, procno, proctype, procglobal, stsize, stalign,
		source_file, source_lineno */);
void	ir_stmt(/*filename, lineno*/);
int	ir_cost(/*np*/);
BOOLEAN non_volatile(/*np*/);
BOOLEAN ir_infunc();

LEAF	*heap_leaf=NULL;
BOOLEAN ir_chk_ovflo();
BOOLEAN ir_set_chk_ovflo(/*x*/);
void	quita(/*a1,a2,a3,a4,a5*/);

/* forward */

static void init_segments();
static SEGMENT *
    lookup_segment(/*name, segdescr, segbase, segoffset, seglen, segalign*/);
static int hash_leaf(/*class,location*/);
static LIST *copy_list(/*tail*/);

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* standard IR types */

TYPE undeftype =  { PCC_UNDEF,    0,		   0		   };
TYPE inttype =    { PCC_INT,      ALINT/SZCHAR,    SZINT/SZCHAR    };
TYPE uinttype =   { PCC_UNSIGNED, ALINT/SZCHAR,    SZINT/SZCHAR    };
TYPE chartype =   { PCC_CHAR,     ALCHAR/SZCHAR,   SZCHAR/SZCHAR   };
TYPE uchartype =  { PCC_UCHAR,    ALCHAR/SZCHAR,   SZCHAR/SZCHAR   };
TYPE shorttype =  { PCC_SHORT,    ALSHORT/SZCHAR,  SZSHORT/SZCHAR  };
TYPE ushorttype = { PCC_USHORT,   ALSHORT/SZCHAR,  SZSHORT/SZCHAR  };
TYPE longtype =   { PCC_LONG,     ALLONG/SZCHAR,   SZLONG/SZCHAR   };
TYPE ulongtype =  { PCC_ULONG,    ALLONG/SZCHAR,   SZLONG/SZCHAR   };
TYPE floattype =  { PCC_FLOAT,    ALFLOAT/SZCHAR,  SZFLOAT/SZCHAR  };
TYPE doubletype = { PCC_DOUBLE,   ALDOUBLE/SZCHAR, SZDOUBLE/SZCHAR };
TYPE ptrtype =    { PCC_PTR,	  ALPOINT/SZCHAR,  SZPOINT/SZCHAR  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * Note on IR segment allocation
 *	The C front end allocates IR segments for all names that
 *	map to local or global static storage.  Segment allocation
 *	is reset at the beginning of each procedure; there is no
 *	memory of segments from one routine to the next, other than
 *	the built in segments.  The names of the built-in segments
 *	are used only for debugging information; they are not used
 *	in assembler storage declarations.
 */

SEGMENT
    *arg_seg,
    *bss_small,	/*NOTUSED*/
    *bss_large,	/*NOTUSED*/
    *data_seg,	/*NOTUSED*/
    *auto_seg,	/*NOTUSED*/
    *heap_seg,
    *dreg_seg,
    *areg_seg,
    *freg_seg;

static struct seginit {
    SEGMENT **segvar;
    SEGMENT seg;
} seginit[] = {
    &arg_seg,	{ "ARG_SEG",  {ARG_SEG,STG_SEG,BUILTIN_SEG}, INARGREG },
    &bss_small,	{ "BSS_SMALL",{BSS_SEG,STG_SEG,BUILTIN_SEG}, NOBASEREG },
    &bss_large,	{ "BSS_LARGE",{BSS_SEG,STG_SEG,BUILTIN_SEG}, NOBASEREG },
    &data_seg,	{ "DATA_SEG", {DATA_SEG,STG_SEG,BUILTIN_SEG}, NOBASEREG },
    &auto_seg,	{ "AUTO_SEG", {AUTO_SEG,STG_SEG,BUILTIN_SEG}, AUTOREG },
    &heap_seg,	{ "HEAP_SEG", {HEAP_SEG,STG_SEG,BUILTIN_SEG,EXTSTG_SEG}, NOBASEREG },
#if TARGET==MC68000
    &dreg_seg,	{ "DREG_SEG", {DREG_SEG,REG_SEG,BUILTIN_SEG}, NOBASEREG },
    &areg_seg,	{ "AREG_SEG", {AREG_SEG,REG_SEG,BUILTIN_SEG}, NOBASEREG },
    &freg_seg,	{ "FREG_SEG", {FREG_SEG,REG_SEG,BUILTIN_SEG}, NOBASEREG },
#endif

#if TARGET==SPARC || TARGET == I386
    &dreg_seg,	{ "DREG_SEG", {DREG_SEG,REG_SEG,BUILTIN_SEG}, NOBASEREG },
    &freg_seg,	{ "FREG_SEG", {FREG_SEG,REG_SEG,BUILTIN_SEG}, NOBASEREG },
#endif

    NULL
};

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static struct segdescr_st auto_descr = {
    AUTO_SEG, STG_SEG, USER_SEG, LCLSTG_SEG
};

static struct segdescr_st extern_descr = {
    BSS_SEG, STG_SEG, USER_SEG, EXTSTG_SEG
};

static struct segdescr_st lstatic_descr = {
    BSS_SEG, STG_SEG, USER_SEG, LCLSTG_SEG
};

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void
init_segments()
{
    register struct seginit *ip;
    register SEGMENT *segp;

    /* assume segment table is initially empty */
    for (ip = seginit; ip->segvar != NULL; ip++) {
	segp = new_segment(ip->seg.name, &(ip->seg.descr), ip->seg.base);
	*(ip->segvar) = segp;
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static SEGMENT *
lookup_segment(name, segdescr, segbase, segoffset, seglen, segalign)
    char *name;
    struct segdescr_st *segdescr;
    int segbase;
    OFFSZ segoffset;
    OFFSZ seglen;
    int segalign;
{
    register unsigned h;
    register unsigned char c, *cp;
    register SEGMENT *segp;
    register CHAIN *p;

    h = 0;
    segp = NULL;
    for (cp = (unsigned char*)name; c = *cp++; h += c) {
	/*nothing*/
    }
    h %= SEG_HASH_SIZE;
    for (p = seg_hash_tab[h]; p != NULL; p = p->nextp) {
	segp = (SEGMENT*)p->datap;
	if (strcmp(name, segp->name) == 0
	  && segdescr->class == segp->descr.class
	  && segdescr->content == segp->descr.content
	  && segdescr->builtin == segp->descr.builtin
	  && segdescr->external == segp->descr.external
	  && segbase == segp->base
	  && segoffset == segp->offset
	  && seglen == segp->len
	  && segalign == segp->align) {
	    return segp;
	}
    }
    /* none found, make a new one */
    segp = new_segment(name, segdescr, segbase);
    segp->offset = segoffset;
    segp->len = seglen;
    segp->align = segalign;
    seg_hash_tab[h] = new_chain((char*)segp, seg_hash_tab[h]);
    return segp;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int
hash_leaf(class,location)
    LEAF_CLASS class;
    LEAF_VALUE *location;
{
    register unsigned key;
    register char *cp;
    register SEGMENT *sp;
    register ADDRESS *ap;
    register struct constant *constp;

    key = 0;
    if (class == VAR_LEAF || class == ADDR_CONST_LEAF)  {
	ap = (ADDRESS*)location;
	sp = ap->seg;
	if(sp->descr.builtin == BUILTIN_SEG) {
	    key = ( ( (int) sp->descr.class << 16) | (ap->offset) );
	} else {
	    for(cp=sp->name;*cp!='\0';cp++) key += *cp;
	    key = ( (key << 16) | (ap->offset) );
	}
    } else {
	/* class == CONST_LEAF */
	constp = (struct constant*)location;
	key = (unsigned) constp->c.i; 
    }
    key %= LEAF_HASH_SIZE;
    return( (int) key);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

LEAF *
lookup_leaf(class,type,location,insert)
    LEAF_CLASS class;
    TYPE type;
    LEAF_VALUE *location;
    BOOLEAN insert;
{
    register LEAF *leafp;
    register LIST *hash_listp;
    register SEGMENT *sp;
    register ADDRESS *ap,*ap2;
    register struct constant *constp;
    int index;
    LIST *new_l;

    if (class == VAR_LEAF || class == ADDR_CONST_LEAF )  {
	ap = (ADDRESS*)location;
	sp = ap->seg;
    } else { 
	constp = (struct constant*)location;
    }
    index = hash_leaf(class,location);

    LFOR(hash_listp,leaf_hash_tab[index]) {
	if(hash_listp && LCAST(hash_listp,LEAF)->class == class )  {
	    leafp = (LEAF*)hash_listp->datap;
	    if(same_irtype(leafp->type,type) == FALSE) continue;
	    if(class == VAR_LEAF || class == ADDR_CONST_LEAF) {
		ap2 = &leafp->val.addr;
		if(ap->labelno == ap2->labelno && 
		    ap2->seg == ap->seg && ap2->offset == ap->offset) {
		       return(leafp);
		} else {
		    continue;
		}
	    } else {
		if(constp->isbinary == TRUE) break;
		if( IR_ISINT(type.tword) ) {
		    if(leafp->val.const.c.i == constp->c.i) return(leafp);
		    else continue;
		} else if (IR_ISCHAR(type.tword) || IR_ISPTRFTN(type.tword)) {
		    if(strcmp(leafp->val.const.c.cp,constp->c.cp) == 0) 
			return(leafp);
		    else continue;
		} else if( IR_ISREAL(type.tword) ) {
		    if(strcmp(leafp->val.const.c.fp[0],constp->c.fp[0]) == 0) 
			return(leafp);
		    else continue;
		}
		quita("lookup_leaf: no literal pool for tword");
		continue;
	    }
	}
    }
    if(insert == FALSE) {
	return(NULL);
    }
    leafp = new_leaf(class);
    leafp->type = type;
    leafp->class=class;
    if(leafp->class == VAR_LEAF || leafp->class == ADDR_CONST_LEAF) {
	leafp->val.addr = *ap;
    } else {
	leafp->val.const = *constp;
    }
    new_l = new_list();
    (LEAF*) new_l->datap = leafp;
    LAPPEND(leaf_hash_tab[index],new_l);
    if(ir_debug > 3) {
	print_leaf(leafp);
    }
    return(leafp);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static SEGMENT *
regseg(n)
    int n;
{
#   if TARGET == MC68000
	if (n <= D7) return dreg_seg;
	if (n <= SP) return areg_seg;
	return freg_seg;
#   endif
#   if TARGET == SPARC
	if (n <= RI7) return dreg_seg;
	return freg_seg;
#   endif
#   if TARGET == I386
        if (n == REG_FP0) return freg_seg;
        return dreg_seg;
#   endif
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int
firstreg(regclass)
    SEGCLASS regclass;
{
#   if TARGET == MC68000
	switch(regclass) {
	case DREG_SEG: return D0;
	case AREG_SEG: return A0;
	case FREG_SEG: return FP0;
	}
#   endif
#   if TARGET == SPARC
	switch(regclass) {
	case DREG_SEG: return RG0;
	case FREG_SEG: return FR0;
	}
#   endif
#   if TARGET == I386
	switch(regclass) {
	case DREG_SEG: return REG_EAX;
	case FREG_SEG: return REG_FP0;
	}
#   endif
    quita("not a register segment");
    /*NOTREACHED*/
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int
regnum(n)
    int n;
{
#   if TARGET == MC68000
	if (n <= D7) return n - firstreg(DREG_SEG);
	if (n <= SP) return n - firstreg(AREG_SEG);
	return n - firstreg(FREG_SEG);
#   endif
#   if TARGET == SPARC
	if (n <= RI7) return n - firstreg(DREG_SEG);
	return n - firstreg(FREG_SEG);
#   endif
#   if TARGET == I386
        return n - REG_EAX;
#   endif
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if TARGET == I386
static char *regnames[] = {
        "%eax","%edx","%ecx","%st0",
	"%wf2","%wf4","%wf6","%wf8",
	"%ebx","%esi","%edi","%esp","%ebp",
	"%wf10","%wf12","%wf14","%wf16",
	"%wf18","%wf20","%wf22","%wf24",
	"%wf26","%wf28","%wf30"
};
#endif
 
char *
regname(n)
    int n;
{
    static char buf[10];
    char *fmt;
#   if TARGET == MC68000
	if (n <= D7) {
	    fmt = "d%d"; n -= firstreg(DREG_SEG);
	} else if (n <= SP) {
	    fmt = "a%d"; n -= firstreg(AREG_SEG);
	} else {
	    fmt = "fp%d"; n -= firstreg(FREG_SEG);
	}
#   endif
#   if TARGET == SPARC
	if (n <= RO7) {
	    fmt = "%%o%d"; n -= firstreg(DREG_SEG);
	} else if (n <= RL7) {
	    fmt = "%%l%d"; n -= RL0;
	} else if (n <= RI7) {
	    fmt = "%%i%d"; n -= RI0;
	} else {
	    fmt = "%%f%d"; n -= firstreg(FREG_SEG);
	}
#   endif
#   if TARGET != I386
    sprintf(buf, fmt, n);
    return buf;
#   else
    return regnames[n];
#   endif
}


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

IR_NODE *
ir_reg_var(name, regno, type)
    char *name;
    int regno;
    TYPE type;
{
    ADDRESS addr;
    LEAF *leaf;
    int segno;

    bzero((char*)&addr, sizeof(addr));
    addr.seg = regseg(regno);
    addr.offset = regnum(regno);
    leaf =  lookup_leaf(VAR_LEAF, type, &addr, TRUE);
    leaf->pass1_id = new_string(name);
    return (IR_NODE*)leaf;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

IR_NODE *
ir_auto_var(name, segoffset, seglen, segalign, offset, type)
    char *name;
    long segoffset;	/* offset of segment (relocatable unit of storage) */
    long seglen;
    int  segalign;
    long offset;	/* offset of variable/field within segment */
    TYPE type;
{
    ADDRESS addr;
    LEAF *leaf;
    char lname[40];

    bzero((char*)&addr, sizeof(addr));
    sprintf(lname, "AUTO_SEG.%d.%d", segoffset, seglen);
    addr.seg = lookup_segment(lname, &auto_descr, AUTOREG,
	    segoffset, seglen, segalign);
    addr.offset = offset;
    leaf =  lookup_leaf(VAR_LEAF, type, &addr, TRUE);
    leaf->pass1_id = new_string(name);
    return (IR_NODE*)leaf;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

IR_NODE *
ir_param_var(name, offset, type)
    char *name;
    int offset;
    TYPE type;
{
    ADDRESS addr;
    LEAF *leaf;

    bzero((char*)&addr, sizeof(addr));
    addr.seg = arg_seg;
    addr.offset = offset;
    leaf =  lookup_leaf(VAR_LEAF, type, &addr, TRUE);
    leaf->pass1_id = new_string(name);
#   if TARGET == SPARC
	leaf->must_store_arg = TRUE;
#   endif
    return (IR_NODE*)leaf;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * this function maps a local, static variable, which is
 * characterized by the lack of an external name (a label
 * is generated from labelno in lieu of a real name).  The
 * declared name of the variable, if any, is passed along
 * in the pass1_id field for debugging purposes.
 *
 * Local static variables must always be mapped as
 * externals, because of the possibility of recursion.
 *
 * Anonymous local statics (i.e., strings) are mapped
 * as locals.
 */
IR_NODE *
ir_static_var(name, labelno, offset, type)
    char *name;
    int labelno;
    OFFSZ offset;
    TYPE type;
{
    ADDRESS addr;
    LEAF *leaf;
    char lname[200];

    bzero((char*)&addr, sizeof(addr));
    sprintf(lname, LABFMT, labelno);
    /* that's right, locals are really global; see NOTE above */
    if (name[0] == '\0') {
	/* anonymous local static: no direct references */
	addr.seg = lookup_segment(lname, &lstatic_descr, NOBASEREG, 0, 0, 0);
    } else {
	/* named local static: must map as external, because of recursion */
	addr.seg = lookup_segment(lname, &extern_descr, NOBASEREG, 0, 0, 0);
    }
    addr.offset = offset;
    leaf =  lookup_leaf(VAR_LEAF, type, &addr, TRUE);
    leaf->pass1_id = new_string(name);
    return (IR_NODE*)leaf;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * map an external symbol
 */
IR_NODE *
ir_extern_var(name, offset, type)
    char    *name;
    int    offset;
    TYPE   type;
{
    ADDRESS    addr;
    LEAF    *leaf;

    bzero((char*)&addr, sizeof(addr));
    addr.seg = lookup_segment(name, &extern_descr, NOBASEREG, 0, 0, 0);
    addr.offset = offset;
    leaf = lookup_leaf(VAR_LEAF, type, &addr, TRUE);
    leaf->pass1_id = new_string(name);
    return (IR_NODE*)leaf;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

IR_NODE *
ir_funcname(name, type)
    char    *name;
    TYPE   type;
{
    struct constant const;

    const.isbinary = FALSE;
    const.c.cp = new_string(name);
    return( (IR_NODE*)lookup_leaf(CONST_LEAF, type, &const, IR_TRUE) );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

IR_NODE *
ir_addrcon(np, offset, type)
    IR_NODE *np;
    TYPE type;
{
    LEAF *leafp;
    ADDRESS addr;

    if (np->operand.tag != ISLEAF) {
	quita("ir_addrcon: operand must be leaf node");
    }
    addr = np->leaf.val.addr;
    addr.offset += offset;
    leafp = lookup_leaf(ADDR_CONST_LEAF, type, (LEAF_VALUE*)&addr, TRUE);
    leafp->pass1_id = np->leaf.pass1_id;
    leafp->addressed_leaf = (LEAF*)np;
    return (IR_NODE*)leafp;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void
ir_mkpoint(np, type)
    IR_NODE *np;
    TYPE type;
{
    if (PCC_ISPTR(type.tword) && np->leaf.pointerno == -1) {
	np->leaf.pointerno = npointers++;
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * add leaf2 to leaf1's list of neighbors
 */
void
ir_mkneighbor(leaf1, leaf2)
    LEAF *leaf1, *leaf2;
{
    LEAF *leafp;
    LIST *lp, *neighbors, *newlistp;

    if (leaf1 != leaf2) {
	neighbors = leaf1->neighbors;
	LFOR(lp,neighbors) {
	    leafp = (LEAF*)lp->datap;
	    if (leafp == leaf2) return;
	}
	newlistp = new_list();
	newlistp->datap = (union list_u*)leaf2;
	newlistp->next = leaf1->neighbors;
	leaf1->neighbors = newlistp;
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * add leaf2 to leaf1's overlap list
 */
void
ir_mkoverlap(leaf1, leaf2)
    LEAF *leaf1, *leaf2;
{
    LEAF *leafp;
    LIST *lp, *overlap, *newlistp;

    if (leaf1 != leaf2) {
	overlap = leaf1->overlap;
	LFOR(lp,overlap) {
	    leafp = (LEAF*)lp->datap;
	    if (leafp == leaf2) return;
	}
	newlistp = new_list();
	newlistp->datap = (union list_u*)leaf2;
	newlistp->next = leaf1->overlap;
	leaf1->overlap = newlistp;
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * Given a value-producing operand, what is the set of objects
 * that may be accessed by dereferencing its value?  The answer
 * to this question may be represented as a leaf node pointer,
 * where
 *	if NULL	  => set of accessible objects is {},
 *	otherwise => set of accessible objects is leaf->overlap.
 * When the answer is indeterminate, (as is usually the case when
 * pointer vars are involved) the result is heap_leaf, whose overlap
 * list includes all externals and all locals whose addresses have been
 * taken.
 */

static LEAF *
can_access_leaf(np)
    IR_NODE *np;
{
    IR_OP op;
    IR_NODE *lp, *rp;
    PCC_TWORD ltype, rtype;
    LEAF *leafp;

    /*
     * Leaves: only cases of interest are ADDR_CONSTs and pointer vars
     */
    if (np->operand.tag == ISLEAF) {
	if (np->leaf.class == ADDR_CONST_LEAF) {
	    /* result is referenced variable */
	    leafp = np->leaf.addressed_leaf;
	    if (leafp->elvarno != -1) {
		/* aliased variable */
		return leafp;
	    }
	    /* non-aliased variable, e.g., string */
	    return NULL;
	}
	if (np->leaf.class == VAR_LEAF && PCC_ISPTR(np->leaf.type.tword)) {
	    /* pointer value - indeterminate */
	    return heap_leaf;
	}
	return NULL;
    }
    /*
     * Triples: assume we are in an expression context,
     * and all triples produce a value.
     */
    op = np->triple.op;
    switch(op) {
    case IR_FCALL:
    case IR_IFETCH:
	if (PCC_ISPTR(np->triple.type.tword))
	    return heap_leaf;
	return NULL;
    case IR_CONV:	/* beware (int*)x; */
    case IR_PCONV:	/* type punning operator */
	if (PCC_ISPTR(np->triple.type.tword)) {
	    lp = np->triple.left;
	    if (PCC_ISPTR(lp->operand.type.tword)) {
		/* ptr-ptr conversion: possible addressees are the same */
		return can_access_leaf(lp);
	    }
	    /* conversion of non-pointer to pointer; indeterminate */
	    return heap_leaf;
	}
	return NULL;
    case IR_EQ:
    case IR_NE:
    case IR_LT:
    case IR_GT:
    case IR_LE:
    case IR_GE:
	return NULL;
    default:
	if (ISOP(op, UN_OP)) {
	    /* unary ops */
	    if (PCC_ISPTR(np->triple.type.tword)) {
		return can_access_leaf(np->triple.left);
	    }
	    return NULL;
	} else {
	    /* binary ops */
	    lp = np->triple.left;
	    rp = np->triple.right;
	    ltype = lp->operand.type.tword;
	    rtype = rp->operand.type.tword;
	    if (PCC_ISPTR(ltype) && PCC_ISPTR(rtype)) {
		/* pointers on both sides; if op is MINUS, result is INT */
		if (np->triple.op == IR_MINUS) {
		    return NULL;
		}
		quita("can_access_leaf: strange ptr-ptr combination");
	    }
	    /* normal cases: one side is pointer, but not both */
	    if (PCC_ISPTR(ltype)) {
		return can_access_leaf(lp);
	    }
	    if (PCC_ISPTR(rtype)) {
		return can_access_leaf(rp);
	    }
	}
    }
    return NULL;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * Make a copy of the 'overlap' and 'neighbors' lists
 * for a leaf of class VAR_LEAF.  Used to construct the
 * 'can_access' list for triples.
 */
static LIST *
copy_overlaps(leafp)
    LEAF  *leafp;
{
    LIST *newlistp, *overlap, *neighbors;

    if (leafp == NULL) {
	return NULL;
    }
    newlistp = new_list();
    newlistp->datap = (union list_u*)leafp;
    if (leafp->overlap != NULL) {
	overlap = copy_list(leafp->overlap);
	LAPPEND(newlistp, overlap);
    }
    if (leafp->neighbors != NULL) {
	neighbors = copy_list(leafp->neighbors);
	LAPPEND(newlistp, neighbors);
    }
    return newlistp;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * generate conservative aliasing information, in the case
 * where no IROPT aliasing is assumed.  The information
 * consists of lists of leaf nodes accessible via IFETCH,
 * ISTORE, and PARAM triples.
 */
void
fill_can_access()
{
    BLOCK *bp;
    register TRIPLE *first, *tp, *argp;
    LEAF  *leafp;

    if ( optimize < FULL_OPT_LEVEL ) {
	return;
    }
    for(bp=first_block; bp; bp=bp->next) {
	first = bp->last_triple;
	if(first != TNULL) {
	    first = first->tnext;
	    TFOR(tp, first) {
		switch (tp->op) {
		case IR_IFETCH:
		case IR_ISTORE:
		    /* compute can_access lists for address-valued
		       expression */
		    leafp = can_access_leaf(tp->left);
		    tp->can_access = copy_overlaps(leafp);
		    break;
		case IR_SCALL:
		case IR_FCALL:
		    tp->can_access = copy_overlaps(heap_leaf);
		    break;
		}
	    }
	}
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

IR_NODE *
ir_fcon(dval, type)
    double dval;
    TYPE type;
{
    struct constant const;
    float fval;

    bzero((char*)&const, sizeof(const));
    const.isbinary = TRUE;
    if (type.tword == PCC_FLOAT) {
	/* convert to single precision (bletch) */
	fval = (float)dval;
	const.c.bytep[0] = add_bytes((char*)(&fval), sizeof(float));
    } else {
	const.c.bytep[0] = add_bytes((char*)(&dval), sizeof(double));
    }
    return( (IR_NODE*)lookup_leaf(CONST_LEAF, type, &const, TRUE) );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

IR_NODE *
ir_icon(i, t)
    int i;
    TYPE t;
{
    struct constant const;
    PCC_TWORD tword;
    IR_NODE *np;

    bzero((char*)&const, sizeof(const));
    tword = t.tword;
    if(PCC_ISCHAR(tword)) {
	/*
	 * FORTRAN hangover: chars cannot be treated as ints
	 * in IR.  They have to be represented as strings.
	 */
	char stupid[2];
	stupid[0] = (char)i;
	stupid[1] = '\0';
	np = cleaf(stupid);
	np->operand.type = t;
	return np;
    }
    if(PCC_ISPTR(tword)) {
	/*
	 * i is the value of a no-name pointer constant.
	 * Ignoring the fact that pointers are not integers...
	 */
	t.tword = PCC_INT;
    }
    const.isbinary = FALSE;
    const.c.i=i;
    np = (IR_NODE*)lookup_leaf(CONST_LEAF, t, &const, TRUE);
    if (PCC_ISPTR(tword)) {
	/*
	 * If this is being used as a pointer, insert an
	 * explicit conversion, so we know that we're carrying
	 * a hand grenade with the pin missing...
	 */
	t.tword = tword;
	np = (IR_NODE*)new_triple(IR_CONV, np, NULL, t);
    }
    return np;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

IR_NODE *
ileaf(i)
    int i;
{
    return( ir_icon(i, inttype) );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

IR_NODE *
cleaf(s)
    char *s;
{
    struct constant const;
    TYPE t;

    bzero((char*)&const, sizeof(const));
    ir_set_type(t, IR_STRING, strlen(s), sizeof(char));
    const.isbinary = FALSE;
    const.c.cp= new_string(s);
    return( (IR_NODE*)lookup_leaf(CONST_LEAF,t,&const,TRUE) );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * pass a string to the IR file
 */
void
ir_pass(s)
    register char *s;
{
    IR_NODE *np;
    int len;

    np = cleaf(s);
    s = np->leaf.val.const.c.cp;
    len = np->leaf.type.size;
    if (s[len-1] == '\n') {
	/* extra '\n'; clip it */
	s[len-1] = '\0';
    }
    (void)new_triple(IR_PASS, np, NULL, np->leaf.type); 
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * generate an unconditional branch to a label n.  If n is the
 * sentinel EXIT_LABELNO, this will become a branch to the
 * procedure epilogue, which is not part of any IR block.
 * Only one such branch should occur per procedure; iropt
 * uses this to identify a unique basic block as a procedure's
 * exit point.
 */
void
ir_branch(n)
    int n;
{
    IR_NODE  *lab;

    lab = new_triple(IR_LABELREF, ileaf(n), ileaf(0), inttype);
    (void)new_triple(IR_GOTO, NULL, (IR_NODE*) lab, inttype);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * define a label.  Every block must begin with a label
 * definition, and every conditional branch should be followed
 * by one; this is assumed by iropt when it builds the flow
 * graph of a procedure.
 */
void
ir_labeldef(lab)
    int lab;
{
    (void) new_triple(IR_LABELDEF, ileaf(lab), (IR_NODE*) NULL, inttype);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static LIST *
copy_list(tail)
    register LIST *tail;
{
    register LIST *lp1, *lp2, *last;

    last = LNULL;
    LFOR(lp1,tail) {
	lp2=new_list();
	lp2->datap = lp1->datap;
	LAPPEND(last,lp2);
    }
    if(last != LNULL) {
	return(last->next);
    } else {
	return(last);
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

BOOLEAN
same_irtype(t1,t2)
    TYPE t1,t2;
{
    if(t1.tword == t2.tword ) {
	return((BOOLEAN)(t1.size == t2.size && t1.align == t2.align));
    }
    if (PCC_ISPTR(t1.tword) && PCC_ISPTR(t2.tword)) {
	return(TRUE);
    }
    return(FALSE);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * Initialize per-procedure IR data structures.  Initialize the
 * header with all the data we know about at beginning of the routine.
 * Table size and other info comes later.
 *
 * This routine defines the block structure of an IR procedure.
 * A procedure body consists of three blocks:
 *
 * - an entry block, which defines the procedure name and consists
 *   of a branch to the a main block;
 * - a main block, which contains the procedure code and terminates
 *   with a branch to the exit block;
 * - an exit block, which consists of a branch to EXIT_LABELNO,
 *   which has no associated block.
 *
 * The purpose of this structure is to simplify life for iropt, which
 * wants a single-entry single-exit view of the world, in spite of such
 * things as C return statements and FORTRAN multiple entry points.
 *
 * We assume that the extra branches are cleaned up by the peephole
 * optimizer.
 */

void
ir_init(procname, procno, proctype, procglobal, stsize, stalign,
	source_file, source_lineno)
    char *procname;
    int procno;
    PCC_TWORD proctype;
    int procglobal;
    int stsize;
    int stalign;
    char *source_file;
    int source_lineno;
{
    ADDRESS addr;
    int main_label;

    bzero((char*)&addr, sizeof(addr));
    mk_irdope();
    init_segments();

    /* initialize modes */
    (void)ir_set_chk_ovflo(FALSE);

    /* reset resource id counters */
    npointers = 0;
    naliases = 0;

    /* build heap leaf */
    addr.seg = heap_seg;
    heap_leaf = lookup_leaf(VAR_LEAF, undeftype, &addr, TRUE);
    heap_leaf->pass1_id = new_string("<heap>");
    heap_leaf->elvarno = naliases++;

    /* initialize header */
    hdr.procname = new_string(procname);
    hdr.procno = procno;
    hdr.source_file = new_string(source_file);
    hdr.source_lineno = source_lineno;
    ir_set_type(hdr.proc_type, proctype, stsize, stalign);

    /* define entry block */
    (void)new_block(procname, getlab(),  TRUE, (BOOLEAN)procglobal);
    main_label = getlab();
    ir_branch(main_label);

    /* define main block */
    (void)new_block(NULL, main_label, FALSE, FALSE);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

char *
ir_optim_level(arg)
    char *arg;
{
    if (arg != NULL && arg[0] == 'O') {
	switch (arg[1]) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	    optimize = arg[1] - '0';
	    arg++;
	    break;
	default:
	    optimize = DEFAULT_OPT_LEVEL;
	}
    }
    return arg;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void
ir_stmt(filename, lineno)
    char *filename;
    int lineno;
{
    static char *last_filename;	/* we assume filenames are hashed */
    static int   last_lineno;

    if (filename != last_filename || lineno != last_lineno) {
	(void)new_triple(IR_STMT, cleaf(filename), ileaf(lineno), undeftype);
	last_filename = filename;
	last_lineno = lineno;
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * simplistic cost estimator for IR operands.  We use this
 * when compiling assignments and whatnot for value, given
 * a choice between two operands, one of which may be used
 * as the result.
 */
int
ir_cost(np)
    IR_NODE *np;
{
    int cval;
    SEGCLASS sclass;

    if (np->operand.tag == ISLEAF) {
	switch(np->leaf.class) {
	case CONST_LEAF:
	    if (IR_ISINT(np->leaf.type.tword)) {
		cval = np->leaf.val.const.c.i;
#		if TARGET == SPARC
		    if (cval >= -4096 && cval <= 4095)
			return COST_SMALLCONST;
#		endif
#		if TARGET == MC68000 || TARGET == I386
		    if (cval >= -32768 && cval <= 32767)
			return COST_SMALLCONST;
#		endif
	    }
	    return COST_LARGECONST;
	case ADDR_CONST_LEAF:
	case VAR_LEAF:
	    if (np->leaf.type.tword == IR_BITFIELD) {
		return COST_EXPR;
	    }
	    if (np->leaf.val.addr.seg->descr.content == REG_SEG) {
		return COST_REG;
	    }
	    switch(np->leaf.val.addr.seg->descr.class) {
	    case AUTO_SEG:
	    case ARG_SEG:
		return COST_AUTO;
	    default:
		return COST_EXTSYM;
	    }
	}
    }
    return COST_EXPR;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * is this operand potentially "volatile"?, i.e., would
 * we break the program by adding/deleting/reordering
 * accesses to this variable, independent of properties
 * that can be determined by data flow analysis?
 */
BOOLEAN
non_volatile(np)
    register IR_NODE *np;
{
    if (np->leaf.tag == ISLEAF) {
	switch(np->leaf.class) {
	case CONST_LEAF:
	case ADDR_CONST_LEAF:
	    return IR_TRUE;
	case VAR_LEAF:
	    if (np->leaf.val.addr.seg->descr.content == REG_SEG) {
		return IR_TRUE;
	    }
	    switch(np->leaf.val.addr.seg->descr.class) {
	    case AUTO_SEG:
	    case ARG_SEG:
		return IR_TRUE;
	    default:
		return IR_FALSE;
	    }
	    /*NOTREACHED*/
	}
    }
    /* don't know if it's safe -- assume false */
    return IR_FALSE;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

BOOLEAN
ir_infunc()
{
	return (BOOLEAN)(hdr.procname != NULL);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static BOOLEAN check_overflow = FALSE;

BOOLEAN
ir_chk_ovflo()
{
    return check_overflow;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

BOOLEAN
ir_set_chk_ovflo(x)
    BOOLEAN x;
{
    BOOLEAN temp;
    temp = check_overflow;
    check_overflow = x;
    return temp;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int
ir_optimizing()
{
    return (optimize >= PARTIAL_OPT_LEVEL);
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*VARARGS1*/
void
quita(a1, a2, a3, a4, a5)
    char *a1;
{
    (void)fprintf(stderr, a1, a2, a3, a4, a5);
    exit(1);
    /*NOTREACHED*/
}
