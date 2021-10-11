#ifndef lint
static	char sccsid[] = "@(#)ir_debug.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

# include "ir_misc.h"
# include <stdio.h>
# include <ctype.h>

/* export */
int ir_debug = 0;

/* from ir_misc.c */
extern char *regname(/*n*/);

/* static strings -- for debugging info */
static char *segcontent_names[] = {
    "REG_SEG", "STG_SEG"
};

static char *segbuiltin_names[] = {
    "USER_SEG", "BUILTIN_SEG"
};

static char *segstgtype_names[] = {
    "LCLSTG_SEG", "EXTSTG_SEG"
};

#if TARGET == MC68000
    static char *segclass_names[] = {
	"ARG_SEG", "BSS_SEG", "DATA_SEG", "AUTO_SEG", "TEMP_SEG",
	"DREG_SEG", "AREG_SEG", "FREG_SEG", "HEAP_SEG"
    };
    static char *baseoff_fmt = "%s@(%d)";
#endif

#if TARGET == SPARC || TARGET == I386
    static char *segclass_names[] = {
	"ARG_SEG", "BSS_SEG", "DATA_SEG", "AUTO_SEG", "TEMP_SEG",
	"DREG_SEG", "FREG_SEG", "HEAP_SEG"
    };
    static char *baseoff_fmt = "[%s%+d]";
#endif

static struct {
    char *tname; /* type name */
    int size;	 /* size, in bits */
    int align;	 /* alignment, in bits */
} ir_type_info [] = {
    "undef",     0, 0,
    "farg",      0, 0,
    "char",      SZCHAR,   ALCHAR,
    "short",     SZSHORT,  ALSHORT,
    "int",       SZINT,    ALINT,
    "long",      SZLONG,   ALLONG,
    "float",     SZFLOAT,  ALFLOAT,
    "double",    SZDOUBLE, ALDOUBLE,
    "strty",     0, 0,
    "unionty",   0, 0,
    "enumty",    0, 0,
    "moety",     0, 0,
    "uchar",     SZCHAR,   ALCHAR,
    "ushort",    SZSHORT,  ALSHORT,
    "unsigned",  SZINT,    ALINT,
    "ulong",     SZLONG,   ALLONG,
    "void",      0, 0,
    /* END PCC TYPES */
    "bool",      0, 0,
    "extendedf", 0, 0,
    "complex",   SZFLOAT*2,  ALFLOAT,
    "dcomplex",  SZDOUBLE*2, ALDOUBLE,
    "string",    0, 0,
    "labelno",   SZINT,    ALINT,
    "bitfield",  0, 0,
    0 
};

static void
print_type(type)
    TYPE type;
{
    PCC_TWORD t;
    int size, align;

    t = type.tword;
    for(;; t = PCC_DECREF(t)) {
	if( PCC_ISPTR(t) ) {
	    printf( "PTR " );
	} else if( PCC_ISFTN(t) ) {
	    printf( "FTN " );
	} else if( PCC_ISARY(t) ) {
	    printf( "ARY " );
	} else {
	    if ((unsigned)t > IR_LASTTYPE) {
		printf( "t=0x%x ", t );
	    } else {
		printf( "%s ", ir_type_info[t].tname);
	    }
	    break;
	}
    }
    if (t == IR_BITFIELD) {
	printf("<%d:%d> ", type.align, type.size);
    } else {
	if (PCC_ISPTR(type.tword)) {
	    size = SZPOINT/SZCHAR;
	    align = ALPOINT/SZCHAR;
	} else if ((unsigned)type.tword <= IR_LASTTYPE) {
	    size = ir_type_info[type.tword].size/SZCHAR;
	    align = ir_type_info[type.tword].align/SZCHAR;
	} else {
	    size = 0;
	    align = 0;
	}
        if (type.size != size) {
	    printf("size %d ", type.size);
	}
        if (type.align != align) {
	    printf("align %d ", type.align);
	}
    }
}

void
print_address(ap)
    ADDRESS *ap;
{
    SEGMENT *sp;

    sp=ap->seg;
    if (sp->descr.content == REG_SEG) {
	printf("%s ", regname(firstreg(sp->descr.class) + ap->offset));
    } else if (sp->base == -1) {
	printf("%s+%d ", sp->name, ap->offset);
    } else {
	printf(baseoff_fmt, regname(sp->base), sp->offset+ap->offset);
    }
    if(sp->descr.class == DATA_SEG) {
	printf(" label %d ",ap->labelno);
    }
}

/*
 * return an aligned copy of an int
 * at a misaligned address 
 */
static int
l_align(bp)
    register char *bp;
{
    int x;

    if (((unsigned)bp % sizeof(int)) == 0) {
	return (*(int*)bp);
    }
    bcopy(bp, (char*)&x, sizeof(int));
    return(x);
}

static void
print_constant(lp)
    LEAF *lp;
{
    int i;
    char c;
    char *cp;

    if(IR_ISPTRFTN(lp->type.tword)) {
	printf("%s",lp->val.const.c.cp);
	if (lp->unknown_control_flow)
	    printf(" [unk]");
	if (lp->makes_regs_inconsistent)
	    printf(" [regs?]");
    } else {
	switch(PCC_BTYPE(lp->type.tword)) {
	case PCC_CHAR:
	case PCC_UCHAR:
	case IR_STRING:
	    putchar('\'');
	    for(cp=lp->val.const.c.cp; c = (*cp++) ; ) {
		if (isprint(c) || isspace(c)) {
		    putchar(c);
		} else {
		    printf("\\%03o",(unsigned char)c); break;
		}
	    }
	    putchar('\'');
	    break;
	case PCC_SHORT:
	case PCC_INT:
	case PCC_LONG:
	    printf("%d",lp->val.const.c.i);
	    break;
	case PCC_USHORT:
	case PCC_UNSIGNED:
	case PCC_ULONG:
	    printf("0x%x",lp->val.const.c.i);
	    break;
	case PCC_FLOAT:
	    if(lp->val.const.isbinary == TRUE) {
		printf("0x%X", l_align(lp->val.const.c.bytep[0]));
	    } else {
		printf("%s",lp->val.const.c.fp[0]);
	    }
	    break;
	case PCC_DOUBLE:
	    if(lp->val.const.isbinary == TRUE) {
		printf("0x%X,0x%X", l_align(lp->val.const.c.bytep[0]),
		    l_align(lp->val.const.c.bytep[0]+4));
	    } else {
		printf("%s",lp->val.const.c.fp[0]);
	    }
	    break;
	default:
	    printf("unknown leaf constant type");
	    break;
	}
    }
}

void
print_leaf(lp)
    LEAF *lp;
{
    register LIST *listp;

    printf("L[%d] %s ", lp->leafno, (lp->pass1_id ? lp->pass1_id : "\"\""));
    print_type(lp->type);
    switch(lp->class) {
    case ADDR_CONST_LEAF:
	printf("ADDR_CONST ");
	print_address(&lp->val.addr);
	break;

    case VAR_LEAF:
	printf("VAR ");
	print_address(&lp->val.addr);
	if (lp->elvarno != -1)
	    printf(" var# = %d ", lp->elvarno);
	if (lp->pointerno != -1)
	    printf(" ptr# = %d ", lp->pointerno);
	if (lp->no_reg)
	    printf(" [no_reg] ");
	break;

    case CONST_LEAF:
	printf(" CONST ");
	print_constant(lp);
	break;

    default:
	printf(" unknown leaf class");
	break;
    }
    if(lp->overlap) {
	printf(" overlap: ");
	LFOR(listp,lp->overlap) {
	    printf("L[%d] ", LCAST(listp,LEAF)->leafno);
	}
    }
    if(lp->neighbors) {
	printf(" neighbors: ");
	LFOR(listp,lp->neighbors) {
	    printf("L[%d] ", LCAST(listp,LEAF)->leafno);
	}
    }
    printf("\n");
}

void
print_triple(tp, indent)
    register TRIPLE *tp;
    int indent;
{
    register LIST *lp;
    register TRIPLE *tlp;
    register LEAF *leafp;
    int i;

    for(i=0;i<indent;i++){
	printf("\t");
    }

    printf("T[%d] ", tp->tripleno );

    if( ISOP(tp->op,VALUE_OP)) {
	print_type(tp->type);
    } else {
	printf("\t");
    }
    printf("%s ",op_descr[(int)tp->op].name);
    if(tp->op == IR_PASS) {
	printf(" %s\n",tp->left->leaf.val.const.c.cp);
    } else if(tp->op == IR_GOTO) {
	TRIPLE *labelref = (TRIPLE*) tp->right;
	if(labelref) {
	    if(labelref->left->operand.tag == ISBLOCK) {
		printf(" B[%d]\n",((BLOCK*)labelref->left)->blockno);
	    } else {
		leafp = (LEAF*)labelref->left;
		printf(" L[%d]=",leafp->leafno);
		print_constant(leafp);
		printf("\n");
	    }
	} else {
	    printf("exit\n");
	}
    } else {
	if(tp->left) {
	    switch(tp->left->operand.tag) {
	    case ISTRIPLE:
		printf(" T[%d]", tp->left->triple.tripleno );
		break;
	    case ISLEAF:
		leafp = (LEAF*)tp->left;
		printf(" L[%d]=", leafp->leafno);
		switch(leafp->class) {
		case VAR_LEAF:
		case ADDR_CONST_LEAF:
		    print_address(&leafp->val.addr);
		    break;
		case CONST_LEAF:
		    print_constant(leafp);
		    break;
		}
		break;
	    case ISBLOCK:
		printf(" B[%d]", ((BLOCK*)tp->left)->blockno);
		break;
	    default:
		printf(" print_triple: bad left operand");
		break;
	    }
	}
	if(tp->right) {
	    switch(tp->right->operand.tag) {
	    case ISLEAF:
		leafp = (LEAF*)tp->right;
		printf(" L[%d]=", leafp->leafno);
		switch(leafp->class) {
		case VAR_LEAF:
		case ADDR_CONST_LEAF:
		    print_address(&leafp->val.addr);
		    break;
		case CONST_LEAF:
		    print_constant(leafp);
		    break;
		}
		break;
	    case ISTRIPLE:
		if(ISOP(tp->op,BRANCH_OP)) {
		    printf(" labels:");
		    TFOR(tlp, (TRIPLE*) tp->right){
			if(tlp->left->operand.tag == ISBLOCK) {
			    printf(" (B[%d] if ",((BLOCK*)tlp->left)->blockno);
			} else {
			    printf(" (L[%d] if ",((LEAF*)tlp->left)->leafno);
			}
			print_constant((LEAF*)tlp->right);
			printf(")");
		    }
		} else if(tp->op == IR_SCALL || tp->op == IR_FCALL) {
		    if(tp->right) {
			printf(" args :\n");
			TFOR(tlp,tp->right->triple.tnext) {
			    print_triple(tlp,indent+2);
			}
		    }
		} else {
		    printf(" T[%d]", tp->right->triple.tripleno );
		}
		break;

	    default:
		printf(" print_triple: bad right operand");
		break;
	    }
	}
	if( tp->can_access ) {
	    printf(" can_access:");
	    LFOR(lp,tp->can_access) {
		printf(" L[%d]", LCAST(lp,LEAF)->leafno);
	    }
	}
	printf("\n");
    }
}

void
print_block(p)
    register BLOCK *p;
{
    register LIST *lp;
    register TRIPLE *tlp;
    int i=0;

    printf("B[%d] %s label %d next %d",
	p->blockno, (p->entryname ? p->entryname : ""),
	p->labelno, (p->next ? p->next->blockno : -1 ));

    printf("\ntriples:");
    if(p->last_triple) {
	TFOR(tlp,p->last_triple->tnext) {
	    if( (++i)%35 == 0)
		printf("\n");
	    printf("%d ",   tlp->tripleno);
	}
    }
    printf("\n");
}

void
print_segment(sp)
    register SEGMENT *sp;
{
    printf("%s",sp->name);
    printf("(%s,", segclass_names[(int)sp->descr.class]);
    printf("%s,", segcontent_names[(int)sp->descr.content]);
    printf("%s,", segbuiltin_names[(int)sp->descr.builtin]);
    printf("%s)", segstgtype_names[(int)sp->descr.external]);
    if (sp->base != NOBASEREG) {
	printf(baseoff_fmt, regname(sp->base), sp->offset);
    }
    if (sp->len != 0) {
	printf(" length %d",sp->len);
    }
    if (sp->align != 0) {
	printf(" align %d", sp->align);
    }
    printf("\n");
}

void
npr(np)
    IR_NODE *np;
{
    if (np != NULL) {
	switch(np->operand.tag) {
	case ISBLOCK:
	    print_block((BLOCK*)np);
	    break;
	case ISLEAF:
	    print_leaf((LEAF*)np);
	    break;
	case ISTRIPLE:
	    print_triple((TRIPLE*)np, 0);
	    break;
	}
    }
}

void
dump_segments()
{
    register SEGMENT *sp;

    printf("\nSEGMENTS\n");
    for(sp=first_seg; sp; sp = sp->next_seg) {
	print_segment(sp);
    }
}

void
dump_leaves() 
{
    register LEAF *lp;
    printf("\nLEAVES\n");
    for(lp = first_leaf; lp; lp = lp->next_leaf) {
	print_leaf(lp);
    }
}

void
dump_triples()
{
    BLOCK *bp;
    register TRIPLE *first, *tp;

    printf("\nTRIPLES:\n");
    for(bp=first_block; bp; bp=bp->next) {
	first = bp->last_triple;
	if(first != TNULL) {
	    first = first->tnext;
	    TFOR(tp, first) {
		print_triple(tp,0);
	    }
	}
    }
}

void
dump_blocks() 
{
    register BLOCK *bp;

    printf("\nBLOCKS\n");
    for(bp=first_block;bp;bp=bp->next) {
	print_block(bp);
    }
}

/* print a LIST of NODEs */

void
lpr(start)
    LIST *start;
{
    LIST *l;

    l=start;
    if (l) {
	do {
	    printf("(%x) ",l);
	    npr((IR_NODE*)l->datap);
	    l=l->next;
	} while(l && l != start);
    }
}
