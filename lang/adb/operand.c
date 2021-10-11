#ifndef lint
static	char sccsid[] = "@(#)operand.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* import */

#include "adb.h"
#include "symtab.h"
#include "oper.h"

extern char *regnames[];

/* export */

void get_operand(/* o, mode, reg, size */);
void print_operand(/* o */);
void printea(/* mode, reg, size */);

/*
 * fill in all the trash from a 68020 indexed/indirect
 * effective address
 */
static void
get_memind(o, index)
    register struct oper *o;
    register unsigned index;
{
    if (BIT(index,7))
	o->flags_o |= O_BSUPRESS;
    switch(BITS(index,5,4)) {	/* base-displacement size */
    case 2:
	o->flags_o |= O_WDISP;
	o->disp_o = instfetch(2);
	break;
    case 3:
	o->flags_o |= O_LDISP;
	o->disp_o = instfetch(4);
	break;
    }
    if (BIT(index,6) == 0) {
	/* index suppress == 0; evaluate and add index operand */
	switch(BITS(index,2,0)) {
	case 0:
	    /* no memory indirection */
	    o->flags_o |= O_PREINDEX;
	    break;
	case 1:
	    /* indirect pre-indexed, null displacement */
	    o->flags_o |= (O_INDIRECT|O_PREINDEX);
	    break;
	case 2:
	    /* indirect pre-indexed, word displacement */
	    o->flags_o |= (O_INDIRECT|O_PREINDEX|O_WDISP2);
	    o->disp2_o = instfetch(2);
	    break;
	case 3:
	    /* indirect pre-indexed, long displacement */
	    o->flags_o |= (O_INDIRECT|O_PREINDEX|O_LDISP2);
	    o->disp2_o = instfetch(4);
	    break;
	case 4:
	    /* reserved */
	    break;
	case 5:
	    /* indirect post-indexed, null displacement */
	    o->flags_o |= (O_INDIRECT|O_POSTINDEX);
	    break;
	case 6:
	    /* indirect post-indexed, word displacement */
	    o->flags_o |= (O_INDIRECT|O_POSTINDEX|O_WDISP2);
	    o->disp2_o = instfetch(2);
	    break;
	case 7:
	    /* indirect post-indexed, word displacement */
	    o->flags_o |= (O_INDIRECT|O_POSTINDEX|O_LDISP2);
	    o->disp2_o = instfetch(4);
	    break;
	}
    } else {
	/* suppress index operand */
	switch(BITS(index,2,0)) {
	case 0:
	    /* no memory indirection */
	    break;
	case 1:
	    /* memory indirect, null displacement */
	    o->flags_o |= O_INDIRECT;
	    break;
	case 2:
	    /* memory indirect, word displacement */
	    o->flags_o |= (O_INDIRECT|O_WDISP2);
	    o->disp2_o = instfetch(2);
	    break;
	case 3:
	    /* memory indirect, long displacement */
	    o->flags_o |= (O_INDIRECT|O_LDISP2);
	    o->disp2_o = instfetch(4);
	    break;
	case 4:
	case 5:
	case 6:
	case 7:
	    /* reserved */
	    break;
	}
    }
}

/*
 * Get an indexed operand and associated offsets.  This
 * handles the 68010 indexed modes and uses get_memind()
 * to handle all the bells & whistles of the 68020.
 */
static void
get_index(o)
    register struct oper *o;
{
    register unsigned index;

    index = instfetch(2);
    o->value_o = BITS(index,15,12);
    if (BIT(index,11) == 0)
	o->flags_o |= O_WINDEX;
    o->scale_o = (1 << BITS(index,10,9));
    if (BIT(index,8)) {
	/* full format */
	get_memind(o, index);
    } else {
	/* brief format */
	o->disp_o = (char)index;
	o->flags_o |= O_PREINDEX;
    }
}

/* 
 * Fetch operand value and register subfields from the instruction
 * stream and unpack them into the operand structure. This routine will
 * fetch only one set of value and register subfields.
 */
void
get_operand(o, mode, reg, size)
    register struct oper *o;
    unsigned mode;
    unsigned reg;
    unsigned size;
{
    unsigned index;
    int disp;
    static struct oper zero;

    *o = zero;
    switch(mode) {
    case 0:	/* dn */
	o->type_o = T_REG; o->value_o = reg;
	break;
    case 1:	/* an */
	o->type_o = T_REG; o->value_o = reg+REG_AR0;
	break;
    case 2:	/* an@ */
	o->type_o = T_DEFER; o->value_o = reg+REG_AR0;
	break;
    case 3:	/* an@+ */
	o->type_o = T_POSTINC; o->value_o = reg+REG_AR0;
	break;
    case 4:	/* an@- */
	o->type_o = T_PREDEC; o->value_o = reg+REG_AR0;
	break;
    case 5:	/* an@(disp16) */
	o->type_o = T_DISPL; o->reg_o = reg+REG_AR0;
	o->value_o = instfetch(2);
	break;
    case 6:	/* indexed modes */
	o->type_o = T_INDEX; o->reg_o = reg+REG_AR0;
	get_index(o);
	break;
    case 7:
	switch (reg) {
	case 0:		/* xxx:w */
	    o->type_o = T_ABSS;
	    o->value_o = instfetch(2);
	    break;
	case 1:		/* xxx:l */
	    o->type_o = T_ABSL;
	    o->value_o = instfetch(4);
	    break;
	case 2:		/* pc@(disp16) */
	    /* bind value now, display as label+off */
	    o->type_o = T_PCDISPL;
	    disp = instfetch(2);
	    o->value_o = disp + inkdot(dotinc-2);
	    break;
	case 3:		/* pc@(disp,index), etc. */
	    o->type_o = T_PCINDEX;
	    get_index(o);
	    break;
	case 4:		/* #xxx */
	    o->type_o = T_IMMED;
	    index = instfetch(size==4?4:2);
	    if (size==1) index &= 0xff;
	    o->value_o = index;
	    break;
	}
	break;
    }
}

static void
printoffset( flags, value, sym )
    unsigned flags;
    int value;
    struct asym *sym;
{
    if (sym != NULL){
        printf("%s",sym->s_name);
        if (value) printf("+%z", value);
    } else {
        printf("%+z", value);
    }
    if (flags&(O_WDISP|O_WDISP2)) {
	printf(":w");
    }
    else if (flags&(O_LDISP|O_LDISP2)) {
	printf(":l");
    }
}


static  void
printxreg(o)
    register struct oper   *o;
{
    printf("%s:%c", regnames[o->value_o], (o->flags_o&O_WINDEX) ? 'w' : 'l');
    if (o->scale_o != 1)
	printf(":%d", o->scale_o);
}

static void
printindex(o)
    register struct oper *o;
{
    printf("(");
    printoffset(o->flags_o, o->disp_o, o->sym_o);
    if (o->flags_o & O_PREINDEX) {
	printf(",");
	printxreg(o);
    }
    printf(")");
    if (o->flags_o & O_INDIRECT) {
	printf("@");
	if (o->disp2_o || o->sym2_o || o->flags_o & O_POSTINDEX) {
	    printf("(");
	    if (o->disp2_o || o->sym2_o) {
		printoffset(o->flags_o, o->disp2_o, o->sym2_o);
		if (o->flags_o & O_POSTINDEX)
		    printf(",");
	    }
	    if (o->flags_o & O_POSTINDEX) {
		printxreg(o);
	    }
	    printf(")");
	}
    }
}

void
print_operand(o)
    register struct oper   *o;
{
    char fbuf[32];

    switch (o->type_o) {
	case T_REG: 
	    printf("%s", regnames[o->value_o]);
	    break;
	case T_DEFER: 
	    printf("%s@", regnames[o->value_o]);
	    break;
	case T_POSTINC: 
	    printf("%s@+", regnames[o->value_o]);
	    break;
	case T_PREDEC: 
	    printf("%s@-", regnames[o->value_o]);
	    break;
	case T_DISPL: 
	    printf("%s@(", regnames[o->reg_o]);
	    printoffset((o->flags_o&O_LDISP), o->value_o, o->sym_o);
	    printf(")");
	    break;
	case T_PCDISPL: 
	    psymoff(o->value_o, ISYM, "");
	    break;
	case T_INDEX: 
	    if (!(o->flags_o & O_BSUPRESS)) {
		printf("%s@", regnames[o->reg_o]);
	    }
	    printindex(o);
	    break;
	case T_PCINDEX:
	    printf("pc@");
	    printindex(o);
	    break;
	case T_ABSS: 
	    psymoff(o->value_o, ISYM, ":w");
	    break;
	case T_ABSL: 
	    psymoff(o->value_o, ISYM, ":l");
	    break;
	case T_IMMED: 
	    printf("#");
	    printoffset(0, o->value_o, o->sym_o);
	    break;
	case T_NORMAL: 
	    printoffset(o->flags_o, o->value_o, o->sym_o);
	    break;
	case T_REGPAIR: 
	    printf("%s:%s", regnames[o->value_o], regnames[o->reg_o]);
	    break;
	case T_FLOAT:
	    sprintf(fbuf,"%.7e", o->dval_o);
	    printf("#0r%s", fbuf);
	    break;
	case T_DOUBLE:
	    sprintf(fbuf,"%.16e", o->dval_o);
	    printf("#0r%s", fbuf);
	    break;
	default: 
	    printf("???(operand type = %d)", o->type_o);
	    break;
    }
}

void
printea(mode, reg, size)
    unsigned mode;
    unsigned reg;
    unsigned size;
{
    static struct oper oper;
    get_operand(&oper, mode, reg, size);
    print_operand(&oper);
}
