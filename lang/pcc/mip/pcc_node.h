/*	@(#)pcc_node.h 1.1 94/10/31 SMI	*/

# ifndef _pcc_node_
# define _pcc_node_

# include "machdep.h"
# include "pcc_types.h"

typedef union pcc_node PCC_NODE;

union pcc_node {

	struct {
		int op;
		PCC_TWORD type;
		int rall;
		SUTYPE su;
		char *name;
		int flags;
		PCC_NODE *left;
		PCC_NODE *right;
	}in; /* interior node */

	struct {
		int op;
		PCC_TWORD type;
		int rall;
		SUTYPE su;
		char *name;
		int flags;
		CONSZ lval;
		int rval;
	}tn; /* terminal node */

	struct {
		int op;
		PCC_TWORD type;
		int rall;
		SUTYPE su;
		int label;  /* for use with branching */
		int reversed;
	}bn; /* branch node */

	struct {
		int op;
		PCC_TWORD type;
		int rall;
		SUTYPE su;
		int stsize;  /* sizes of structure objects */
		int stalign;  /* alignment of structure objects */
	}stn; /* structure node */

	struct {
		int op;
		PCC_TWORD type;
		int cdim;
		int csiz;
		int bfwidth;	/* bit field width */
		int bfoffset;	/* bit field offset */
	}fn; /* front-end node */

	struct {
		/* this structure is used when a floating point constant
		   is being computed */
		int op;
		PCC_TWORD type;
		int rall;
		SUTYPE su;
		double dval;
	}fpn; /* floating point node */

};

# endif  _pcc_node_
