/*	@(#)mip.h 1.1 94/10/31 SMI	*/

/*
 * Guy's changes to merge with system V lint (12 Apr 88):
 *
 * Updated "mip.h".  "malloc_with_care" was removed for the reasons
 * mentioned elsewhere; several function declarations were added in
 * response to "lint" complaints.
 */

# include <stdio.h>
# include "ops.h"
# include "types.h"
# include "node.h"

/*
 * local types
 */

# define TNULL (PTR|UNDEF)
# define TERROR MAXSTDTYPE+1

/*
 * local aliases
 */

# define TWORD PCC_TWORD
# define NODE  PCC_NODE
# define sw    pcc_sw
# define dope  pcc_dope
# define talloc pcc_talloc
# define tcheck pcc_tcheck
# define tfree  pcc_tfree
# define freenode pcc_freenode

/*
 * the following operators are used only in pass 1,
 * and in fact are defined in cgram.y.  They appear
 * here because some are used in mkdope(), though they
 * are not part of the pcc intermediate code.
 */

# define TYPE 33
# define CLASS 34
# define STRUCT 35
# define RETURN 36
# define IF 38
# define ELSE 39
# define SWITCH 40
# define BREAK 41
# define CONTINUE 42
# define WHILE 43
# define DO 44
# define FOR 45
# define DEFAULT 46
# define CASE 47
# define SIZEOF 48
# define ENUM 49
# define LP 50
# define RP 51
# define LC 52
# define RC 53
# define LB 54
# define RB 55
# define SM 57
# define DOT 68
# define STREF 69
# define ASM 112

/* conversion operators, rewritten before pass 2 */

# define PMCONV 106
# define PVCONV	107
# define CAST   111

/*
 * various flags and temporary operators
 */

# define NOLAB (-1)	/* break, continue, or return out of context */
# define CCODES   96	/* result of computation is in condition codes */
# define FREE     97	/* indicates a free tree node */
# define FCCODES 113	/* result of computation is in condition codes */

/*
 * miscellaneous macros
 */

/* advance x to a multiple of y */
# define SETOFF(x,y)   if( (x)%(y) != 0 ) (x) = ( ((x)/(y) + 1) * (y))

/* can y bits be added to x without overflowing z */
# define NOFIT(x,y,z)   ( ( (x)%(z) + (y) ) > (z) )

/*
 * storage allocation for strings, tree nodes
 */

# define TREESZ 512
extern char *hash();
extern char *savestr();
extern char *tstr();
extern void freetstr();

extern char *malloc();
extern char *realloc();
extern char *calloc();

extern char *strcpy();
extern char *strncpy();
extern char *strcat();

extern void print_d();
extern void print_x();
extern void print_str();
extern void print_str_nl();
extern void print_str_d();
extern void print_str_d_nl();
extern void print_str_str();
extern void print_str_str_nl();
extern void print_label();

extern void adrput();
extern void upput();

extern void pcc_tfree();

/*
 * common defined variables
 */

extern int nerrors;  /* number of errors seen so far */
extern char *opst[]; /* table of operator names */

# define NIL (NODE *)0

/* flags for conditional compilation of debugging code */

# ifdef BUG2
# define BUG1
# endif
# ifdef BUG3
# define BUG2
# define BUG1
# endif
# ifdef BUG4
# define BUG1
# define BUG2
# define BUG3
# endif

# define BUFSTDERR


