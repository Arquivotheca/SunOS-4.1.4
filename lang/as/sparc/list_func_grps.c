char *sccs_id = "@(#)list_func_grps.c	1.1\t10/31/94";
#include <stdio.h>

#include "structs.h"
#include "lex_tables.h"

char *fg_lookup();
char *fsg_lookup();

main(argc)
	int argc;
{
	register struct opcode *opcp;

	/* The intention is that "list_func_grps -h" turns off the header */
	if (argc == 1)
	{
		print_fields("Opcode", "Func Group", "Func SubGroup");
		print_fields("------", "----------", "-------------");
	}

	for (opcp = &init_opcs[0];  opcp->o_name != NULL;   opcp++)
	{
		if (opcp->o_synonym == NULL)
		{
			print_fields( opcp->o_name,
			      	fg_lookup(opcp->o_func.f_group),
			      	fsg_lookup(opcp->o_func.f_subgroup) );
		}
		else
		{
			print_fields( opcp->o_name,
					"synonym for:",
					opcp->o_synonym );
		}
	}
	
	exit(0);
}

print_fields(f1, f2, f3)
	char *f1, *f2, *f3;
{
	printf("%-8s\t%-8s\t%s\n", f1, f2, f3);
}

struct fg_list
{
	enum functional_group fg_no;
	char *fg_name;
} fg_names[] =
{
	{ FG_error,	"FG_error"	},
	{ FG_NONE,	"FG_NONE"	},
	{ FG_MARKER,	"FG_MARKER"	},
	{ FG_COMMENT,	"FG_COMMENT"	},
	{ FG_LD,	"FG_LD"		},
	{ FG_ST,	"FG_ST"		},
	{ FG_LDST,	"FG_LDST"	},
	{ FG_ARITH,	"FG_ARITH"	},
	{ FG_WR,	"FG_WR"		},
	{ FG_SETHI,	"FG_SETHI"	},
	{ FG_BR,	"FG_BR"		},
	{ FG_TRAP,	"FG_TRAP"	},
	{ FG_CALL,	"FG_CALL"	},
	{ FG_JMP,	"FG_JMP"	},
	{ FG_JMPL,	"FG_JMPL"	},
	{ FG_RETT,	"FG_RETT"	},
	{ FG_IFLUSH,	"FG_IFLUSH"	},
	{ FG_UNIMP,	"FG_UNIMP"	},
	{ FG_RD,	"FG_RD"		},
	{ FG_FLOAT2,	"FG_FLOAT2"	},
	{ FG_FLOAT3,	"FG_FLOAT3"	},
	{ FG_CP2,	"FG_CP2"	},
	{ FG_CP3,	"FG_CP3"	},
	{ FG_RET,	"FG_RET"	},
	{ FG_NOP,	"FG_NOP"	},
	{ FG_LABEL,	"FG_LABEL"	},
	{ FG_SET,	"FG_SET"	},
	{ FG_EQUATE,	"FG_EQUATE"	},
	{ FG_SYM_REF,	"FG_SYM_REF"	},
	{ FG_PS,	"FG_PS"		},
	{ FG_CSWITCH,	"FG_CSWITCH"	},
	{ (enum functional_group)0,
			NULL		}
};
struct fsg_list
{
	enum functional_subgroup fsg_no;
	char *fsg_name;
} fsg_names[] =
{
	{ FSG_error,	"FSG_error"	},
	{ FSG_NONE,	"FSG_NONE"	},
	{ FSG_common,	"FSG_common"	},
	{ FSG_LSB,	"FSG_LSB"	},
	{ FSG_SWAP,	"FSG_SWAP"	},
#ifdef CHIPFABS
	{ FSG_LSW,	"FSG_LSW"	},
#endif
	{ FSG_ADD,	"FSG_ADD"	},
	{ FSG_SUB,	"FSG_SUB"	},
	{ FSG_MULS,	"FSG_MULS"	},
	{ FSG_AND,	"FSG_AND"	},
	{ FSG_ANDN,	"FSG_ANDN"	},
	{ FSG_OR,	"FSG_OR"	},
	{ FSG_ORN,	"FSG_ORN"	},
	{ FSG_XOR,	"FSG_XOR"	},
	{ FSG_XORN,	"FSG_XORN"	},
	{ FSG_SLL,	"FSG_SLL"	},
	{ FSG_SRL,	"FSG_SRL"	},
	{ FSG_SRA,	"FSG_SRA"	},
	{ FSG_TADD,	"FSG_TADD"	},
	{ FSG_TSUB,	"FSG_TSUB"	},
	{ FSG_SAVE,	"FSG_SAVE"	},
	{ FSG_REST,	"FSG_REST"	},
	{ FSG_RDY,	"FSG_RDY"	},
	{ FSG_RDPSR,	"FSG_RDPSR"	},
	{ FSG_RDWIM,	"FSG_RDWIM"	},
	{ FSG_RDTBR,	"FSG_RDTBR"	},
	{ FSG_WRY,	"FSG_WRY"	},
	{ FSG_WRPSR,	"FSG_WRPSR"	},
	{ FSG_WRWIM,	"FSG_WRWIM"	},
	{ FSG_WRTBR,	"FSG_WRTBR"	},
	{ FSG_FCVT,	"FSG_FCVT"	},
	{ FSG_FMOV,	"FSG_FMOV"	},
	{ FSG_FNEG,	"FSG_FNEG"	},
	{ FSG_FABS,	"FSG_FABS"	},
	{ FSG_FCMP,	"FSG_FCMP"	},
	{ FSG_FSQT,	"FSG_FSQT"	},
	{ FSG_FADD,	"FSG_FADD"	},
	{ FSG_FSUB,	"FSG_FSUB"	},
	{ FSG_FMUL,	"FSG_FMUL"	},
	{ FSG_FDIV,	"FSG_FDIV"	},
	{ FSG_RET,	"FSG_RET"	},
	{ FSG_RETL,	"FSG_RETL"	},
	{ FSG_SEG,	"FSG_SEG"	},
	{ FSG_OPTIM,	"FSG_OPTIM"	},
	{ FSG_ASCII,	"FSG_ASCII"	},
	{ FSG_ASCIZ,	"FSG_ASCIZ"	},
	{ FSG_GLOBAL,	"FSG_GLOBAL"	},
	{ FSG_COMMON,	"FSG_COMMON"	},
	{ FSG_RESERVE,	"FSG_RESERVE"	},
	{ FSG_EMPTY,	"FSG_EMPTY"	},
	{ FSG_BOF,	"FSG_BOF"	},
	{ FSG_STAB,	"FSG_STAB"	},
	{ FSG_SGL,	"FSG_SGL"	},
	{ FSG_DBL,	"FSG_DBL"	},
	{ FSG_EXT,	"FSG_EXT"	},
	{ FSG_BYTE,	"FSG_BYTE"	},
	{ FSG_HALF,	"FSG_HALF"	},
	{ FSG_WORD,	"FSG_WORD"	},
	{ FSG_ALIGN,	"FSG_ALIGN"	},
	{ FSG_SKIP,	"FSG_SKIP"	},
	{ FSG_PROC,	"FSG_PROC"	},
	{ FSG_NOALIAS,	"FSG_NOALIAS"	},
	{ FSG_ALIAS,	"FSG_ALIAS"	},
	{ FSG_N,	"FSG_N"		},
	{ FSG_A,	"FSG_A"		},
	{ FSG_E,	"FSG_E"		},
	{ FSG_LE,	"FSG_LE"	},
	{ FSG_L,	"FSG_L"		},
	{ FSG_LEU,	"FSG_LEU"	},
	{ FSG_CS,	"FSG_CS"	},
	{ FSG_NEG,	"FSG_NEG"	},
	{ FSG_VS,	"FSG_VS"	},
	{ FSG_NE,	"FSG_NE"	},
	{ FSG_G,	"FSG_G"		},
	{ FSG_GE,	"FSG_GE"	},
	{ FSG_GU,	"FSG_GU"	},
	{ FSG_CC,	"FSG_CC"	},
	{ FSG_POS,	"FSG_POS"	},
	{ FSG_VC,	"FSG_VC"	},
	{ FSG_FU,	"FSG_FU"	},
	{ FSG_FG,	"FSG_FG"	},
	{ FSG_FUG,	"FSG_FUG"	},
	{ FSG_FL,	"FSG_FL"	},
	{ FSG_FUL,	"FSG_FUL"	},
	{ FSG_FLG,	"FSG_FLG"	},
	{ FSG_FNE,	"FSG_FNE"	},
	{ FSG_FE,	"FSG_FE"	},
	{ FSG_FUE,	"FSG_FUE"	},
	{ FSG_FGE,	"FSG_FGE"	},
	{ FSG_FUGE,	"FSG_FUGE"	},
	{ FSG_FLE,	"FSG_FLE"	},
	{ FSG_FULE,	"FSG_FULE"	},
	{ FSG_FO,	"FSG_FO"	},
	{ FSG_C3,	"FSG_C3"	},
	{ FSG_C2,	"FSG_C2"	},
	{ FSG_C23,	"FSG_C23"	},
	{ FSG_C1,	"FSG_C1"	},
	{ FSG_C13,	"FSG_C13"	},
	{ FSG_C12,	"FSG_C12"	},
	{ FSG_C123,	"FSG_C123"	},
	{ FSG_C0,	"FSG_C0"	},
	{ FSG_C03,	"FSG_C03"	},
	{ FSG_C02,	"FSG_C02"	},
	{ FSG_C023,	"FSG_C023"	},
	{ FSG_C01,	"FSG_C01"	},
	{ FSG_C013,	"FSG_C013"	},
	{ FSG_C012,	"FSG_C012"	},
	{ (enum functional_subgroup)0,
			NULL		}
};

char *
fg_lookup(g)
	enum functional_group g;
{
	register struct fg_list *fgp;

	for (fgp = &fg_names[0];   fgp->fg_name != NULL;   fgp++)
	{
		if (fgp->fg_no == g)	return fgp->fg_name;
	}

	fprintf("[fg_lookup() error: can't find group %d\n", (int)g);
	exit(1);
}

char *
fsg_lookup(sg)
	enum functional_subgroup sg;
{
	register struct fsg_list *fsgp;

	for (fsgp = &fsg_names[0];   fsgp->fsg_name != NULL;   fsgp++)
	{
		if (fsgp->fsg_no == sg)	return fsgp->fsg_name;
	}

	fprintf("[fsg_lookup() error: can't find subgroup %d\n", (int)sg);
	exit(1);
}
