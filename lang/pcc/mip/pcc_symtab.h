/*	@(#)pcc_symtab.h 1.1 94/10/31 SMI */

/*
 * definition of the pcc symbol table
 */

# ifndef _pcc_symtab_
# define _pcc_symtab_

/* storage classes  */

# define PCC_SNULL 0
# define PCC_AUTO 1
# define PCC_EXTERN 2
# define PCC_STATIC 3
# define PCC_REGISTER 4
# define PCC_EXTDEF 5
# define PCC_LABEL 6
# define PCC_ULABEL 7
# define PCC_MOS 8
# define PCC_PARAM 9
# define PCC_STNAME 10
# define PCC_MOU 11
# define PCC_UNAME 12
# define PCC_TYPEDEF 13
# define PCC_FORTRAN 14
# define PCC_ENAME 15
# define PCC_MOE 16
# define PCC_UFORTRAN 17
# define PCC_USTATIC 18
# define PCC_STP(x)  ( ((int)(x) < (int)(&end)) ? 0 : (struct pcc_symtab*)(x) )

typedef struct pcc_symtab PCC_SYMBOL;

extern end;
extern char *pcc_sname();
extern int pcc_sclass();
extern PCC_TWORD pcc_stype();
extern int pcc_soffset();
extern int pcc_ssize();
extern int pcc_salign();

# endif _pcc_symtab_

