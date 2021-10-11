static char sccs_id[] = "@(#)sym.c	1.1\t10/31/94";
#include <stdio.h>	/* for definition of NULL */
#include "structs.h"
#include "sym.h"
#include "alloc.h"

/* All references to "sym" or "SYM" should be to the SYMBOL table,
 * and likewise "opc" or "OPC" refers to the OPCODE symbol table.
 */

#define SYMHASHTBLSZ	4999	/* hash tbl size- must be prime */
#define OPCHASHTBLSZ	199	/* hash tbl size- must be prime */

typedef unsigned int	HASHIDX;

#define SymHEntry	struct symbol *
#define OpcHEntry	struct opcode *

struct symbol *sym_table[SYMHASHTBLSZ];	/* assume load-time init. to all NULL */
struct opcode *opc_table[OPCHASHTBLSZ];	/* assume load-time init. to all NULL */


/**************************  s y m _ n e w  **********************************/

/* allocate a new symbol-table entry + the char-string for its name */

struct symbol *
sym_new(namep)
register char *namep;
{
	register struct symbol	*sp;
	register unsigned int	 size;
	register char		*newnamep;

	size = sizeof(struct symbol) + strlen(namep) + 1;
	sp = (struct symbol *) as_malloc(size);
	newnamep = ((char *) sp) + sizeof(struct symbol);

	/* Zero/NULL all fields, by default.
	 * bzero() is much faster than zero/NULL'ing them all individually.
	 */
	bzero(sp, sizeof(struct symbol));

	/* Copy the symbol name, and set up the pointer to it. */
	strcpy(newnamep, namep);
	sp->s_symname   = newnamep;

	/* Initialize the fields which are (potentially) non-zero */
	sp->s_segment	= ASSEG_UNKNOWN;
#if SYMIDX_NONE != 0
	sp->s_symidx	= SYMIDX_NONE;
#endif

	/* we know this will never be the case, but for completeness... */
#if NULL != 0
	sp->s_chain	= NULL;		/* not on hash-table chain yet	*/
	sp->s_def_node	= NULL;		/* no defining node yet		*/
	sp->s_ref_list	= NULL;		/* no referencing nodes yet	*/
	sp->s_blockname = NULL;
#endif

	return(sp);
}

/*----------------------------------------------------------------------------
 *	There are 2 symbol tables being maintained here, one for the opcode
 *	mnemonics and one for the usual symbols.  Since the structures referred
 *	to, the hash table sizes used, and practically everything else is
 *	physically different between the two symbol tables, it would be awkward
 *	and inefficient (at least in C) to have them share the same symbol-
 *	table management routines -- even though the symbol tables are
 *	LOGICALLY handled about the same.
 *	
 *	Enter an interesting way of handling this situation: the symbol-table
 *	management routines for BOTH types of symbols are coded (once) in file
 *	"sym_fcns.h", using the preprocessor to code at a "meta" level and hide
 *	the physical differences between the symbol tables (e.g. structure
 *	names, table names, table sizes, routine names, etc).  The parameters
 *	for generating one of the routine sets are #define'd, and the file
 *	of functions is included.  Then this is repeated for the other set of
 *	routines.

 *	So, A SINGLE COPY of the symbol-table source can be maintained,
 *	at the expense of having to read the code at a slightly higher level
 *	of abstaction than usual.
 *----------------------------------------------------------------------------
 */


/*---------------------------------------------------------------------------*/
/*	Generate symbol-table routines for Symbol Table			     */
/*---------------------------------------------------------------------------*/

#define STRUCTNAME	symbol
#define SYMNAME		s_symname
#define CHAIN		s_chain
#undef SYNONYM
#define TABLE		sym_table
#define TBLLEN		SYMHASHTBLSZ
#define FCN_INIT	sym_inittbl
#define FCN_LOOKUP	sym_lookup
#define FCN_HASH	sym_hash
#define FCN_NEW		sym_new
#define FCN_DELETE	sym_delete
#define FCN_FREE	sym_free
#define FCN_ADD		sym_add
#define FCN_NEXT	sym_next
#define FCN_FIRST	sym_first

#include "sym_fcns.h"


/*---------------------------------------------------------------------------*/
/*	undefine all of these, before generating the next set of routines
/*---------------------------------------------------------------------------*/

#undef STRUCTNAME
#undef SYMNAME
#undef CHAIN
#undef SYNONYM
#undef TABLE
#undef TBLLEN
#undef FCN_INIT
#undef FCN_LOOKUP
#undef FCN_HASH
#undef FCN_NEW
#undef FCN_DELETE
#undef FCN_FREE
#undef FCN_ADD
#undef FCN_NEXT
#undef FCN_FIRST


/*---------------------------------------------------------------------------*/
/*	Generate symbol-table routines for Opcode Table			     */
/*---------------------------------------------------------------------------*/

#define STRUCTNAME	opcode
#define SYMNAME		o_name
#define CHAIN		o_chain
#define SYNONYM		o_synonym
#define TABLE		opc_table
#define TBLLEN		OPCHASHTBLSZ
#define FCN_INIT	opc_inittbl
#define FCN_LOOKUP	opc_lookup
#define FCN_HASH	opc_hash
#undef FCN_NEW
#define FCN_DELETE	opc_delete
#define FCN_FREE	opc_free
#undef FCN_ADD
#undef FCN_NEXT
#undef FCN_FIRST

#include "sym_fcns.h"
