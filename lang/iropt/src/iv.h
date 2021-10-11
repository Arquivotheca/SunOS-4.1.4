/*	@(#)iv.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __iv__
#define __iv__

typedef struct iv_info {
	BOOLEAN	is_rc:1;
	BOOLEAN	is_iv:1;
	int indx;
	LEAF *leafp;	/* x, the var in question */
	LIST *clist;	/* list of constants for which T(x*c) must be maintained */
	char *afct;	/* the ivs and rcs which can affect the value of x */
	TRIPLE *def_triple;	/* triple which defines the iv */
	TRIPLE *base_on;	/* 0:base, -1:non-base, other:definition */
				/* triple of the base iv which iv bases on */
	LIST * plus_temps;	/* list of "T(x*c) + L1 " temporaries */
	struct iv_info *next;	/* structures are doubly linked */
	struct iv_info *prev;	/* for efficient deletion */
} IV_INFO;

typedef struct iv_hashtab_rec {
	LEAF *x, *c, *t;
	IR_OP op;
	TRIPLE *def_tp;
	LIST * update_triples;	/* triples that update the temp */
	BOOLEAN updated;
	struct iv_hashtab_rec *next;
} IV_HASHTAB_REC; 

# define IV(leafp)	( (IV_INFO *) ((LEAF*) leafp)->visited )
# define IV_HASHTAB_SIZE 256
# define YES 1

extern int	n_iv, n_rc;
extern LIST	*iv_def_list;
extern BOOLEAN	check_definition(/*def*/);
extern void	find_iv(/*loop*/);
extern void	set_iv(/*def_tp, check_base_iv, loop*/);
extern void	set_rc(/*leafp*/);
extern void	iv_init_visited();  /* pre-pass initialization */
extern void	iv_init();	    /* per-loop initialization */
extern void	iv_cleanup();	    /* post-pass storage deallocation */
extern void	do_induction_vars(/*loop*/);
extern TRIPLE	*find_parent(/*def_tp*/); 
extern TYPE	temp_type(/*l,r*/);

#endif
