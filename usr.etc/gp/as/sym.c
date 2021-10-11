#ifndef lint
static	char sccsid[] = "@(#)sym.c 1.1 94/10/31 SMI";
#endif

/*
 *	Micro-assembler symbol-table management
 *		sym.c	1.0	20 Feb. 84
 */

#include "micro.h"

SYMBOL syms[NSYM], *cursym = syms;
char   strings[NSTRING], *curstring = strings;
extern sor(), tor(), shf(), bor(), rot(), rom(), etc();

RESERVED *rhash[NHASH];
SYMBOL   *shash[NHASH];

RESERVED words[] = {

	/*   Pseudo-ops                    */
	{ "even",	Pseudo, 1,         },
	{ "odd",	Pseudo, 2,         },
	{ "org",	Pseudo, 3,         },
	{ "debug",	Pseudo, 4,         },
	{ "stop",       Pseudo, 5,         },
	{ "bkpt",       Pseudo, 6,         },
	{ "sccsid",       Pseudo, 7,         },

	/*   debug switch values           */
	{ "verbose",     Switch, 1,        },
	{ "off",         Switch, 2,        },
	{ "concise",     Switch, 3,        },

	/*   Am29116 Opcodes               */

	/*   single-operand instructions   */
	{ "movb",	Aluop, 0, 0xc, sor },
	{ "movw",	Aluop, 1, 0xc, sor },
	{ "compb",	Aluop, 0, 0xd, sor },
	{ "compw",	Aluop, 1, 0xd, sor },
	{ "incb",	Aluop, 0, 0xe, sor },
	{ "incw",	Aluop, 1, 0xe, sor },
	{ "negb",	Aluop, 0, 0xf, sor },
	{ "negw",	Aluop, 1, 0xf, sor },
	/*     two-operand instructions    */
	{ "subb",	Aluop, 0, 0x0, tor },
	{ "subw",	Aluop, 1, 0x0, tor },
	{ "subcb",	Aluop, 0, 0x1, tor },
	{ "subcw",	Aluop, 1, 0x1, tor },
	{ "rsubb",	Aluop, 0, 0x2, tor },
	{ "rsubw",	Aluop, 1, 0x2, tor },
	{ "rsubcb",	Aluop, 0, 0x3, tor },
	{ "rsubcw",	Aluop, 1, 0x3, tor },
	{ "addb",	Aluop, 0, 0x4, tor },
	{ "addw",	Aluop, 1, 0x4, tor },
	{ "addcb",	Aluop, 0, 0x5, tor },
	{ "addcw",	Aluop, 1, 0x5, tor },
	{ "andb",	Aluop, 0, 0x6, tor },
	{ "andw",	Aluop, 1, 0x6, tor },
	{ "nandb",	Aluop, 0, 0x7, tor },
	{ "nandw",	Aluop, 1, 0x7, tor },
	{ "xorb",	Aluop, 0, 0x8, tor },
	{ "xorw",	Aluop, 1, 0x8, tor },
	{ "norb",	Aluop, 0, 0x9, tor },
	{ "norw",	Aluop, 1, 0x9, tor },
	{ "orb",	Aluop, 0, 0xa, tor },
	{ "orw",	Aluop, 1, 0xa, tor },
	{ "xnorb",	Aluop, 0, 0xb, tor },
	{ "xnorw",	Aluop, 1, 0xb, tor },
	/*   single-bit shift instructions */
	{ "sl0b",	Aluop, 0, 0x0, shf },
	{ "sl0w",	Aluop, 1, 0x0, shf },
	{ "sl1b",	Aluop, 0, 0x1, shf },
	{ "sl1w",	Aluop, 1, 0x1, shf },
	{ "slqb",	Aluop, 0, 0x2, shf },
	{ "slqw",	Aluop, 1, 0x2, shf },
	{ "sr0b",	Aluop, 0, 0x4, shf },
	{ "sr0w",	Aluop, 1, 0x4, shf },
	{ "sr1b",	Aluop, 0, 0x5, shf },
	{ "sr1w",	Aluop, 1, 0x5, shf },
	{ "srqb",	Aluop, 0, 0x6, shf },
	{ "srqw",	Aluop, 1, 0x6, shf },
	{ "srcb",	Aluop, 0, 0x7, shf },
	{ "srcw",	Aluop, 1, 0x7, shf },
	{ "srnovb",	Aluop, 0, 0x8, shf },
	{ "srnovw",	Aluop, 1, 0x8, shf },
	/*          bit instructions       */
	{ "bsetb",	Aluop, 0, 0x0, bor },
	{ "bsetw",	Aluop, 1, 0x0, bor },
	{ "bclrb",	Aluop, 0, 0x1, bor },
	{ "bclrw",	Aluop, 1, 0x1, bor },
	{ "btstb",	Aluop, 0, 0x2, bor },
	{ "btstw",	Aluop, 1, 0x2, bor },
	{ "add2nb",	Aluop, 0, 0x3, bor },
	{ "add2nw",	Aluop, 1, 0x3, bor },
	{ "sub2nb",	Aluop, 0, 0x4, bor },
	{ "sub2nw",	Aluop, 1, 0x4, bor },
	{ "mov2nb",	Aluop, 0, 0x5, bor },
	{ "mov2nw",	Aluop, 1, 0x5, bor },
	{ "not2nb",	Aluop, 0, 0x6, bor },
	{ "not2nw",	Aluop, 1, 0x6, bor },
	/*      rotate instructions        */
	{ "rolb",	Aluop, 0, 0x0, rot },
	{ "rolw",	Aluop, 1, 0x0, rot },
	/*   rotate-and-xxxx instructions  */
	{ "romb",	Aluop, 0, 0x0, rom },
	{ "romw",	Aluop, 1, 0x0, rom },
	{ "rocb",	Aluop, 0, 0x1, rom },
	{ "rocw",	Aluop, 1, 0x1, rom },
	/*   other instructions            */
	{ "prib",	Aluop, 0, 0x0, etc },
	{ "priw",	Aluop, 1, 0x0, etc },
	{ "crc",	Aluop, 0, 0x1, etc },
	{ "bcrc",	Aluop, 1, 0x1, etc },
	{ "setsr",	Aluop, 0, 0x2, etc },
	{ "clrsr",	Aluop, 1, 0x2, etc },
	{ "movsrb",	Aluop, 0, 0x3, etc },
	{ "movsrw",	Aluop, 1, 0x3, etc },
	{ "nop",	Aluop, 0, 0x4, etc },

	/* operand reserved words	   */
	{ "acc",	Operand, ACC,      },
	{ "d",		Operand, D,        },
	{ "r",		Operand, RAM,      },
	{ "y",		Operand, Y,        },
	{ "d0e",	Operand, DOE,	   },
	{ "dse",	Operand, DSE,      },
	{ "sr",		Operand, SR,       },
	{ "accsr",	Operand, ACCSR,    },
	{ "i",		Operand, IMM,      },
	{ "n",          Operand, NREG,     },

	/*    Status operands              */
	{ "oncz",	Statopd, ONCZ,     },
	{ "link",	Statopd, LINK,     },
	{ "flag1",	Statopd, FLAG1,    },
	{ "flag2",	Statopd, FLAG2,    },
	{ "flag3",	Statopd, FLAG3,    },

	/*    Status enable completer      */
	{ "s",		Completer, 1,      },

	/*    conditions                   */
	{ "zer",	Cc,   0, 0,        },
	{ "neg",	Cc,   1, 0,        },
	{ "cry",	Cc,   2, 0,        },
	{ "ofl",	Cc,   3, 0,        },
#ifdef VIEW
	{ "fprn",	Cc,   6, 0,        },
	{ "f1f",	Cc,  12, 2,        },
	{ "f1nf",	Cc,   4, 1,        },
	{ "f2e",	Cc,  13, 2,        },
	{ "f2ne",	Cc,   5, 1,        },
	{ "go",		Cc,   7, 0,        },
#endif
#ifdef PAINT
	{ "f2nf",	Cc,   4, 1,        },
	{ "f1ne",	Cc,   5, 1,        },
	{ "zbr",	Cc,   6, 0,        },
	{ "vir",        Cc,   7, 0,        },
	{ "f2f",	Cc,  12, 2,        },
	{ "f1e",	Cc,  13, 2,        },
	{ "zer.3",	Cc,  16, 0,        },
	{ "neg.3",	Cc,  17, 0,        },
	{ "cry.3",	Cc,  18, 0,        },
	{ "ofl.3",	Cc,  19, 0,        },
	{ "f2nf.3",	Cc,  20, 1,        },
	{ "f1ne.3",	Cc,  21, 1,        },
	{ "zbr.3",	Cc,  22, 0,        },
	{ "vir.3",      Cc,  23, 0,        },
	{ "f2f.3",	Cc,  28, 2,        },
	{ "f1e.3",	Cc,  29, 2,        },
	{ "go",		Cc,  -1,-1,        },
#endif

	/*    sources & destinations      */
	{ "am",		Srcdest, 0,  3,    },
#ifdef VIEW
	{ "shmem",	Srcdest, 1,  3,    },
	{ "fpregh",	Srcdest, 2,  3,    },
	{ "fpregl",	Srcdest, 3,  3,    },
	{ "fifo2",	Srcdest, 4,  1,    },
	{ "fpstreg",	Srcdest, 5,  1,    },
	{ "vpprom",	Srcdest, 6,  1,    },
	{ "fl2reg",	Srcdest, 7,  1,    },
	/* integer or identifier,8,  1,    */
	{ "nreg",	Srcdest, 4,  2,    },
	{ "stlreg",	Srcdest, 5,  2,    },
	{ "brreg",	Srcdest, 6,  2,    },
	{ "shmemp",	Srcdest, 7,  2,    },
	{ "vppromp",	Srcdest, 8,  2,    },
	{ "fifo1",	Srcdest, 9,  2,    },
	{ "fpap",	Srcdest,10,  2,    },
	{ "fpbp",	Srcdest,11,  2,    },
	{ "fpdp",	Srcdest,12,  2,    },
	{ "fl1reg",	Srcdest,13,  2,    },

	/*    Weitek opcodes               */
	{ "lmode",	Wop,  -1, 0,       },
	{ "wrapa",	Wop,   0, 0,       },
	{ "wrapr",	Wop,   0, 1,       },
	{ "unrpa",	Wop,   1, 0,       },
	{ "unrpr",	Wop,   1, 1,       },
	{ "floata",	Wop,   2, 0,       },
	{ "floatr",	Wop,   2, 1,       },
	{ "fixa",	Wop,   3, 0,       },
	{ "fixr",	Wop,   3, 1,       },
	{ "adda",	Wop,   4, 0,       },
	{ "addr",	Wop,   4, 1,       },
	{ "suba",	Wop,   5, 0,       },
	{ "subr",	Wop,   5, 1,       },
	{ "rsuba",	Wop,   6, 0,       },
	{ "rsubr",	Wop,   6, 1,       },
	{ "absadda",	Wop,   7, 0,       },
	{ "absaddr",	Wop,   7, 1,       },
	{ "subabsa",	Wop,   8, 0,       },
	{ "subabsr",	Wop,   8, 1,       },
	{ "addabsa",	Wop,   9, 0,       },
	{ "addabsr",	Wop,   9, 1,       },
	{ "maba",	Wop,   0, 0,       },
	{ "mabr",	Wop,   0, 1,       },
	{ "mwaba",	Wop,   1, 0,       },
	{ "mwabr",	Wop,   1, 1,       },
	{ "mawba",	Wop,   2, 0,       },
	{ "mawbr",	Wop,   2, 1,       },
	{ "mwawba",	Wop,   3, 0,       },
	{ "mwawbr",	Wop,   3, 1,       },
#endif
#ifdef PAINT
	{ "scrmem",	Srcdest, 1,  3,    },
	{ "fifo1",	Srcdest, 2,  1,    },
	{ "zbrdreg",	Srcdest, 3,  1,    },
	{ "vrdreg",     Srcdest, 4,  1,    },
	{ "ppprom",	Srcdest, 5,  1,    },
	{ "vstreg",	Srcdest, 6,  1,    },
	{ "mulres",     Srcdest, 7,  1,    },
	{ "fl1reg",     Srcdest, 8,  1,    },
	/* integer or ident    , 9,  1,    */
	{ "nreg",	Srcdest, 2,  2,    },
	{ "stlreg",	Srcdest, 3,  2,    },
	{ "brreg",	Srcdest, 4,  2,    },
	{ "scrmemp",	Srcdest, 5,  2,    },
	{ "pppromp",	Srcdest, 6,  2,    },
	{ "fifo2",	Srcdest, 7,  2,    },
	{ "fl2reg",	Srcdest, 8,  2,    },
	{ "zbwdreg",    Srcdest, 9,  2,    },
	{ "vwdreg",     Srcdest,10,  2,    },
	{ "mulx",       Srcdest,11,  2,    },
	{ "muly",       Srcdest,12,  2,    },
	{ "mulmode",    Srcdest,13,  2,    },
	{ "iidreg",     Srcdest,14,  2,    },
	{ "vhiareg",    Srcdest,15,  2,    },
	{ "vloareg",    Srcdest,16,  2,    },
	{ "vctlreg",    Srcdest,17,  2,    },
	{ "zbhiptr",    Srcdest,18,  2,    },
	{ "zbloptr",    Srcdest,19,  2,    },
#endif

#ifdef VIEW
	/*    Weitek rounding modes       */
	{ "rn",		Wround, 0,         },	
	{ "rz",		Wround, 1,         },
	{ "rp",		Wround, 2,         },
	{ "rm",		Wround, 3,         },

	/*    Weitek infinity modes        */
	{ "ai",		Winf,   0,         },
	{ "pi",		Winf,   1,         },

	/*    Weitek modes                 */
	{ "flow",	Wmode,  0,         },
	{ "pipe",	Wmode,  1,         },

	/*    Weitek codes                 */
	{ "ieee",       Wcode,  0,         },
	{ "fast",       Wcode,  1,         },

	/*    Weitek load flags            */
	{ "lab",        Wload,  1,         },
	{ "la",         Wload,  2,         },

	/*    Weitek unload flags          */
	{ "a",		Wunld,  1,         },
	{ "m",		Wunld,  2,         },

	/*    Weitek store flag            */
	{ "st",		Wstore, 1,         },

	/*    Weitek select flags          */
	{ "hi",		Wselect, 0,        },
	{ "lo",		Wselect, 1,        },
#endif

	/*    Am2910 opcodes               */
	{ "jz",		Seqop,  0, 0,      },
	{ "cjs",	Seqop,	1, 3,      },
	{ "jmap",	Seqop,  2, 2,      },
	{ "cjp",	Seqop,  3, 3,      },
	{ "push",	Seqop,  4, 3,      },
	{ "jsrp",	Seqop,  5, 3,      },
	{ "cjv",	Seqop,  6, 3,      },
	{ "jrp",	Seqop,  7, 3,      },
	{ "rfct",	Seqop,  8, 0,      },
	{ "rpct",	Seqop,  9, 2,      },
	{ "crtn",	Seqop, 10, 3,      },
	{ "cjpp",	Seqop, 11, 3,      },
	{ "ldct",	Seqop, 12, 2,      },
	{ "loop",	Seqop, 13, 1,      },
	{ "cont",	Seqop, 14, 0,      },
	{ "twb",	Seqop, 15, 3,      },

	/* Control field values            */
#ifdef VIEW
	{ "shp0",	Control, 0, 0,     },
	{ "shp",	Control, 1, 0,     },
	{ "ap",		Control, 4, 0,     },
	{ "bp",		Control, 5, 0,     },
	{ "dp",		Control, 6, 2,     },
	{ "abp",	Control, 7, 1,     },
	{ "adp",	Control, 8, 0,     },
	{ "bdp",	Control, 9, 0,     },
	{ "abdp",	Control,10, 0,     },
#endif
#ifdef PAINT
	{ "varegs",	Control, 1,        },
	{ "scrmp",	Control, 3,        },
	{ "zbrd",	Control, 5,        },
	{ "vmerd",	Control, 6,        },
	{ "vmewr",	Control, 7,        },
#endif

	{ (char *)0,	Tnull,  0,   0,   (int (*)())0,   0},
};

extern SYMBOL syms[];

init_symtab()
{   int i;

    for (i = 0; i < NSYM; i++) {
	syms[i].defined = False;
    }
}

int
hashval( s ) 
    char *s;
{
    register int val = 0;

    while (*s) {
	val = (val<<1) + *s++;
    }
    val %= NHASH;
    if (val < 0) val += NHASH;
    return val;
}

void
resw_hash()
{
    RESERVED *rp, **pp;

    rp = words;
    while (rp->name) {
	pp = rhash + hashval(rp->name);
	rp->next_hash = *pp;
	*pp = rp;
	rp++;
    }
}

RESERVED *
resw_lookup( s )
    char *s;
{
    register RESERVED *rp;

    rp = rhash[hashval(s)];
    while (rp && strcmp(s,rp->name))
	rp = rp->next_hash;
    return rp;
}

SYMBOL *
lookup( s )
    char *s;
{
    register SYMBOL *sp;

    sp = shash[hashval(s)];
    while ( sp && strcmp( s, sp->name ))
	sp = sp->next_hash;
    return sp;
}

SYMBOL *
enter( s )
    char *s;
{
    register SYMBOL *sp;
    SYMBOL **pp;
    register char *cp;
    int leng;

    leng = strlen(s);
    cp = curstring;
    if ( curstring+leng > &strings[NSTRING-2])
	fatal( "out of string space");
    while( *cp++ = *s++)
	;
    if (cursym >= &syms[NSYM])
	fatal( "out of symbol space");
    sp = cursym++;
    pp = shash + hashval(sp->name = curstring);
    curstring = cp;
    sp->next_hash = *pp;
    *pp = sp;
    sp->node = 0;
    sp->addr = 0;
    return sp;
}

SYMBOL *
firstsym()
{
    return (cursym == syms)? (SYMBOL *)0 : syms;
}
SYMBOL *
nextsym( sp )
    SYMBOL *sp;
{
    sp++;
    return (sp==cursym)? (SYMBOL *)0 : sp;
}
