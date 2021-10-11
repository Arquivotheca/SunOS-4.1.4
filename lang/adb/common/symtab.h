/*	@(#)symtab.h 1.1 94/10/31 SMI	*/

/*
 * There is a name conflict (s_name) with the sun-4 symbol table
 * entry from a.out.h.  (A define is used because it's in a union.)
 * So don't try to use that one if you also use this "asym" struct!
 */
#undef s_name
struct asym {
	char	*s_name;
	int	s_flag;
	int	s_type;
	struct	afield *s_f;
	int	s_fcnt, s_falloc;
	int	s_value;
	int	s_fileloc;
	struct	asym *s_link;
} **globals;
int	nglobals, NGLOBALS;

struct afield {
	char	*f_name;
	int	f_type;
	int	f_offset;
	struct	afield *f_link;
} *fields;
int	nfields, NFIELDS;

struct linepc {
	int	l_fileloc;
	int	l_pc;
} *linepc, *linepclast, *pcline;
int	nlinepc, NLINEPC;

/* need a string hash table here */

struct	asym *lookup(), *cursym;
struct	afield *fieldlookup(), *globalfield();

/* flags used to control the symbol table generation */
#define AOUT 1
#define SHLIB 2
