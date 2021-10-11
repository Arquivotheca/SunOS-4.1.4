/*	@(#)micro.h 1.1 94/10/31 SMI; from UCB X.X XX/XX/XX	*/

/*
 *		Micro-assembler global defines
 *		micro.h	1.1	82/09/11	
 */

typedef enum {False=0, True} Boolean;
typedef enum {NOTFP=0,HIGH,LOW} HILO;
typedef enum {NEITHER, NUMBER, ALPHA} SYMTYPE;
typedef enum {	Tnull, Aluop, Cc, Completer, Control, Lls, Lmode, Operand,
		Statopd, Pseudo, Seqop, Srcdest, Wmode,  Wincr, Winf, Wop,
		 Wround, Wstore, Wunld, Wcode, Wload, Wselect, Switch
} Reswd_type;

#define NNODE	16384	/* max number of instructions */
#define NSYM    2048	/* more than enough symbol-table space */
#define NSTRING 8*NSYM  /* really more than enough string space */
#define NHASH 137

#ifdef VIEW
#define FPREGH 2
#define FPREGL 3
#endif

#define AM 0

#ifdef VIEW
#define SHMEM 1
#define VPPROM 6
#define FL2REG 7
#define VPPROMP 8
#define GENERAL 8
#else
#define SCRMEM 1
#define PPPROM 5
#define FL2REG 8
#define PPPROMP 8
#define GENERAL 9
#endif

#define CONTINUE 14

/* instruction descriptor nodes */
typedef struct node {
	char	*filename;	/* name of file in which encountered	*/
	unsigned short word1;	/* 1st word of microcode instruction	*/
	unsigned short word2;	/* 2nd word of microcode instruction	*/
	unsigned short word3;	/* 3rd word of microcode instruction	*/
	unsigned short word4;	/* last byte of microcode instruction	*/
	struct sym *symptr;	/* symbolic general field value		*/
	short	lineno;		/* line number on which encountered	*/
	short	addr;		/* address we assign this instruction	*/
	char    *line;		/* defining text line			*/
	Boolean sccsid:1;	/* part of the sccsid			*/
	Boolean filled:1;	/* instr. has been filled		*/
	Boolean org_pseudo:1;	/* it's an org pseudo-op		*/
	Boolean	dreg_opd:1;	/* dreg field used for 29116 operand	*/
	Boolean has_genl:1;	/* instr. uses var. field for general	*/
	Boolean genl_sym:1;	/* general value is a symbol		*/
	Boolean imm_29116:1;	/* 29116 field taken by immediate from
				   previous instruction			*/
	Boolean imm_sym:1;	/* 29116 field taken by immediate symbol
				   from previous instruction		*/
	Boolean no_promread:1;	/* cannot read from prom		*/
	Boolean no_flag:1;	/* disallow move from flag register	*/
	Boolean status:1;	/* set status update for 29116 inst.
				   with immediate			*/
#ifdef VIEW
	Boolean fp_srcdst:1;	/* instr. has fl. pt. source or dest.	*/
	Boolean fp_used:1;	/* instr. uses var. field for fl. pt.	*/
	Boolean no_fpstore:1;	/* cannot have a Weitek store		*/
	Boolean no_shmread:1;	/* cannot have a shared memory read	*/

	HILO fphilo:2;		/* setting of hi/lo bit or don't care	*/
	Boolean no_shminc:1;	/* cannot inc or dec shmemp unless also
				   writes to shmem			*/
	Boolean no_fpinc:1;	/* cannot inc fpdp without also writing
				   to fpregh or fpregl			*/
#endif
#ifdef PAINT#
	Boolean no_scrmread:1;	/* cannot have a scratchpad memory read	*/
	Boolean no_scrminc:1;	/* cannot inc or dec scrmemp unless also
				   writes to scrmem			*/
	Boolean three_way:1;	/* three-way branch in 2910		*/
#endif
} NODE;

/* user-defined symbol bucket */
typedef struct sym {
	char		*name;
	Boolean		defined:1;
	short		addr;
	struct node	*node;
	struct sym	*next_hash;
} SYMBOL;

/* reserved-word bucket */
typedef struct reswd {
	char	*name;
	Reswd_type
		type;
	short	value1,
		value2;
	int	(*kracker)();
	struct reswd *next_hash;
} RESERVED;

/* operand reserved values -- don't touch, they're indexes! */
#define Z	0
#define D	1
#define RAM	2
#define Y	3
#define DOE	4
#define DSE	5
#define SR	6
#define ACCSR	7
#define IMM	8
#define ACC	9
#define NREG	10
#define SYM	11

/* status operand reserved values -- don't touch, they're indexes! */
#define ONCZ	0
#define LINK	1
#define FLAG1	2
#define FLAG2	3
#define FLAG3	4

/* operand descriptor */
typedef struct opr {
    long   r;
    long   v;
} OPERAND;

extern Boolean dflag;
#define DEBUG if(dflag) printf

extern RESERVED * resw_lookup();
extern SYMBOL * enter();
extern SYMBOL   * lookup();
