/*	@(#)aliaser.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef __aliaser__
#define __aliaser__

typedef enum {AERR, WRST, ADDR, CPY, STAR} ALIAS_RULES;

/*  Structure for recording pointer effect of each triple */

typedef struct pt_effect {
        ALIAS_RULES srctyp;  /* rule for column */
        int     source;      /* columns of bit vector */
        ALIAS_RULES typ;     /* rule for row          */
        int     object;      /* pointer or rows of bit vector */
};

extern void	find_aliases();

#endif
