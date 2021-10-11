/*	@(#)opt_context.h 1.1 94/10/31 Copyr 1986 SMI	*/

struct name {
	struct symbol *	nm_symbol;
	long int	nm_addend;
};

register_set totally_dead;

struct regtable_entry {
	BITBOOL		valid; /* TRUE if known, false if unknown */
	char		size;  /* number of bytes in a contents-of access */
	BITBOOL		hipart; /* true after sethi %hi(xxx) instruction */
	struct name	v;
	Regno		basereg; /* register part of address in MEMORY case if v.nm_symbol == NULL*/
};

/* a table filled in per-block by regtable_iterate */

#define NREGS	64
#define VALUE	0
#define MEMORY	1

typedef struct regtable_entry regentry[MEMORY+1];
regentry regtable[NREGS];

/* 
 * a table filled in by find_names, emptied out by share_names
 * a table of name references and the nodes that reference them.
 * we delete entries by nulling np
 */

struct namenode{ struct name name; Node * np; int enumeration; };
extern struct namenode *name_array;

/*
 * the form into which these global structures are packed when not in
 * use
 */
 
struct regval {
	Regno		rv_reg;
	int		rv_vm;
	struct regtable_entry
			rv_rte;
};

struct context {
	register_set	ctx_deadregs;
	int		ctx_nnames;
	struct name *	ctx_names;
	int		ctx_nregs;
	struct regval *	ctx_regs;
};
