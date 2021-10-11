#ifndef lint
static	char sccsid[] = "@(#)intr_map.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * this module knows which libF77 library functions are
 * implemented as pcc operators.
 */

#include "pcc_ops.h"

static struct intr_descr {
	char *name;
	int pcc_opno;
} intr_descr_tab[] = {
	{ "d_abs", PCC_FABS },		{ "r_abs", PCC_FABS },
	{ "d_cos", PCC_FCOS },		{ "r_cos", PCC_FCOS },
	{ "d_sin", PCC_FSIN },		{ "r_sin", PCC_FSIN },
	{ "d_tan", PCC_FTAN },		{ "r_tan", PCC_FTAN },
	{ "d_acos", PCC_FACOS },	{ "r_acos", PCC_FACOS },
	{ "d_asin", PCC_FASIN },	{ "r_asin", PCC_FASIN },
	{ "d_atan", PCC_FATAN },	{ "r_atan", PCC_FATAN },
	{ "d_cosh", PCC_FCOSH },	{ "r_cosh", PCC_FCOSH },
	{ "d_sinh", PCC_FSINH },	{ "r_sinh", PCC_FSINH },
	{ "d_tanh", PCC_FTANH },	{ "r_tanh", PCC_FTANH },
	{ "d_exp", PCC_FEXP },		{ "r_exp", PCC_FEXP },
	{ "pow_10d", PCC_F10TOX },	{ "pow_10r", PCC_F10TOX },
	{ "pow_2d", PCC_F2TOX },	{ "pow_2r", PCC_F2TOX },
	{ "d_sqr", PCC_FSQR },		{ "r_sqr", PCC_FSQR },
	{ "d_sqrt", PCC_FSQRT },	{ "r_sqrt", PCC_FSQRT },
	{ "d_log", PCC_FLOGN },		{ "r_log", PCC_FLOGN },
	{ "d_lg10", PCC_FLOG10 },	{ "r_lg10", PCC_FLOG10 },
	{ "d_int", PCC_FAINT },		{ "r_int", PCC_FAINT },
	{ "d_nint", PCC_FANINT },	{ "r_nint", PCC_FANINT },

	{ "i_nint", PCC_FNINT },
	{ "PCC_CHK", PCC_CHK },
};

#define N_INTR_DESCR (sizeof(intr_descr_tab) / sizeof(struct intr_descr))

/* must be greater  than N_INTR_DESCR */
#define HASH_TAB_SIZE  256
static struct intr_descr *hash_tab[HASH_TAB_SIZE];


intr_map_init()
{
	register int i, index;
	register struct intr_descr *descr;
	register char *cp;

	for(i=0; i< N_INTR_DESCR; i++ ) {
		descr = &intr_descr_tab[i];
		cp = descr->name;
		index = 0;
		while(*cp) index += *cp++;
		index = index%HASH_TAB_SIZE;
		while(hash_tab[index]) {
			index++;
			index %= HASH_TAB_SIZE;
		}
		hash_tab[index] = descr;
	}
}

is_intrinsic(name)
	register char *name;
{
	register int index, len;
	register struct intr_descr **hashp;
	char name_copy[21];
	register char *cp2;

	if(!name) return -1;
	len = strlen(name);
	if(len > 20) return -1;
	cp2 = name_copy;
	index = 0;
	while(*name && *name != ' ') {
		index += *name;
		*cp2++ = *name++;
	}
	*cp2 = '\0';
	index %=  HASH_TAB_SIZE;
	hashp = &hash_tab[index];
	while(*hashp) {
		if(strcmp(name_copy, (*hashp)->name) == 0) {
			return (*hashp)->pcc_opno;
		}
		hashp ++;
	}
	return -1;
}
