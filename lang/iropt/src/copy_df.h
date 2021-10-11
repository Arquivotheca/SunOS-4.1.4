/*	@(#)copy_df.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __copy_df__
#define __copy_df__

# define COPY_HASH_SIZE 256

typedef struct copy {
	int copyno;
	BOOLEAN visited;
	struct leaf *left, *right;
	struct list *define_at;
} COPY;

typedef struct copy_ref {
	SITE site;	/* where it defined */
} COPY_REF;

extern int	ncopies;
extern LIST	*copy_hash_tab[COPY_HASH_SIZE];

extern void	entable_copies();
extern BOOLEAN	interesting_copy(/* tp */);
extern void	build_copy_kill_gen();
extern void	build_copy_in_out();

#endif
